#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct
{
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

// Precedence levels, from lowest to highest
typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

// pointer to function that takes 1 bool argument and returns nothing
// ('canAssign' is only used in some parse functions but we need a common signature)
typedef void (*ParseFn)(bool canAssign);

typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct
{
    Token name;   // variable name
    int depth;    // scope depth of the block where the variable was declared
    bool isConst; // isConst = true -> single-assignment variable
} Local;

typedef struct
{
    // all locals that are in scope (all scopes)
    // - same order as declaration order
    // - instruction operand to encode local is 1 byte -> limit number to UINT8_COUNT
    Local locals[UINT8_COUNT];
    int localCount; // how many locals are in scope (all scopes)
    int scopeDepth; // number of blocks surrounding currently compiling code
} Compiler;

Parser parser;
Compiler *current = NULL;
Chunk *compilingChunk;

static Chunk *currentChunk()
{
    return compilingChunk;
}

static void errorAt(Token *token, const char *message)
{
    if (parser.panicMode)
        return; // suppress any detected error while on panic mode
    parser.panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
        // nothing
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char *message)
{
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char *message)
{
    errorAt(&parser.current, message);
}

// Save current token; Ask Scanner for next token and store it
static void advance()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR)
            break;

        errorAtCurrent(parser.current.start);
    }
}

// Consume current token; Report error if it doesn't have expected type
static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type)
    {
        advance();
        return;
    }

    errorAtCurrent(message);
}

// Return True if current token has the given type
static bool check(TokenType type)
{
    return parser.current.type == type;
}

// Consume current token and return True if type match
static bool match(TokenType type)
{
    if (!check(type))
        return false;
    advance();
    return true;
}

// Append a byte to the chunk
static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn()
{
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value)
{
    int constantIndex = addConstant(currentChunk(), value);
    if (constantIndex > UINT8_MAX)
    {
        error("Too many constants in 1 chunk.");
        return 0;
    }

    return (uint8_t)constantIndex;
}

static void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void initCompiler(Compiler *compiler)
{
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

static void endCompiler()
{
    emitReturn();

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
    {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void beginScope()
{
    current->scopeDepth++;
}

static void endScope()
{
    current->scopeDepth--;

    // Discard local variables when popping a scope
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth >
               current->scopeDepth)
    {
        emitByte(OP_POP);
        current->localCount--;
    }
}

static void expression();
static void declaration();
static void statement();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

// Add the lexeme as a string to the chunk's constant table;
// Return its index in constant table
static uint8_t identifierConstant(Token *name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token *a, Token *b)
{
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

// Return the variable index in 'locals' array,
// which is the same as its VM stack slot;
// Return -1 -> not found -> assumed to be global;
static int resolveLocal(Compiler *compiler, Token *name, bool *isConst)
{
    // Walk backward to find the last declared variable with the identifier
    // (inner local variables shadow outer ones with the same name)
    for (int i = compiler->localCount - 1; i >= 0; i--)
    {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name))
        {
            if (local->depth == -1) // uninitialized (example: var a = a;)
            {
                error("Can't read local variable in its own initializer.");
            }

            *isConst = local->isConst;
            return i;
        }
    }

    *isConst = false;
    return -1;
}

// Add variable to compiler's list of variables in current scope
static void addLocal(Token name, bool isConst)
{
    if (current->localCount == UINT8_COUNT)
    {
        error("Too many local variables in function.");
        return;
    }

    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1; // uninitialized
    local->isConst = isConst;
}

// Declare local variable
static void declareVariable(bool isConst)
{
    if (current->scopeDepth == 0)
        // Global variables are late bound (lookup value when executing)
        // -> compiler doesn't keep track of which declarations for them it has seen.
        return;

    Token *name = &parser.previous;

    // Don't allow redeclaring local variable (same name in the same scope)
    // Still allow shadowing (same name in different scopes)
    for (int i = current->localCount - 1; i >= 0; i--)
    {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth)
        { // reached outer scope; also skip just-added uninitialized local (depth = -1)
            break;
        }

        if (identifiersEqual(name, &local->name))
        {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name, isConst);
}

static void binary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;

    // Compile the right operand
    // . Use 1 higher level of precedence, since binary operators are left-associative
    // . Example: 1 + 2 + 3 is parsed like (1 + 2) + 3
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    // Emit the operator instruction
    switch (operatorType)
    {
    case TOKEN_BANG_EQUAL:
        emitBytes(OP_EQUAL, OP_NOT); // a != b <-> !(a == b)
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emitBytes(OP_LESS, OP_NOT); // a >= b <-> !(a < b)
        break;
    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emitBytes(OP_GREATER, OP_NOT); // a <= b <-> !(a > b)
        break;
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    default:
        return; // unreachable
    }
}

static void literal(bool canAssign)
{
    switch (parser.previous.type)
    {
    case TOKEN_FALSE:
        emitByte(OP_FALSE);
        break;
    case TOKEN_NIL:
        emitByte(OP_NIL);
        break;
    case TOKEN_TRUE:
        emitByte(OP_TRUE);
        break;
    default:
        return; // unreachable
    }
}

static void grouping(bool canAssign)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign)
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign)
{
    // exclude the leading and trailing quotation marks
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                    parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    bool isConst;
    int arg = resolveLocal(current, &name, &isConst);
    if (arg != -1)
    { // local variable
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else
    { // global variable
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    // Look for and consume the '=' only if
    // currently in the context of a low-precedence expression
    if (canAssign && match(TOKEN_EQUAL))
    { // setter or assignment
        if (isConst)
            error("Assign to `const` variable.");

        expression();
        emitBytes(setOp, (uint8_t)arg);
    }
    else
    { // getter or variable access
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign)
{
    namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;

    // Compile the operand
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction
    switch (operatorType)
    {
    case TOKEN_BANG:
        emitByte(OP_NOT);
        break;
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    default:
        return; // unreachable
    }
}

// Given a token type, this table lets us find:
// - the function to compile a prefix expression starting with a token of that type
// - the function to compile an infix expression whose left operand is
//   followed by a token of that type
// - the precedence of an infix expression that uses that token as operator
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

// Parse any expression at 'precedence' level or higher
static void parsePrecedence(Precedence precedence)
{
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL)
    {
        error("Expect expressions.");
        return;
    }

    // Since assignment is the lowest-precedence expression,
    // assignment is only allowed when parsing assignment expression
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence)
    {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL))
    {
        error("Invalid assignment target.");
        // Example: a * b = c * d
    }
}

static uint8_t parseVariable(bool isConst, const char *errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable(isConst);
    if (current->scopeDepth > 0)
        return 0; // dummy (locals aren't looked up by names)

    return identifierConstant(&parser.previous);
}

// Mark variable as initialized
// (replace sentinel value -1 with real scope depth)
static void markInitialized()
{
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global, bool isConst)
{
    if (current->scopeDepth > 0)
    {
        // VM has executed code for local variable's initializer,
        // and the value is sitting right on top of the stack.
        // -> the temporaries simply become local variables
        markInitialized();
        return;
    }

    if (isConst)
        emitBytes(OP_DEFINE_CONST_GLOBAL, global);
    else
        emitBytes(OP_DEFINE_GLOBAL, global);
}

static ParseRule *getRule(TokenType type)
{
    return &rules[type];
}

static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void varDeclaration(bool isConst)
{
    // Phases:
    // - Before compiling the initializer, mark the variable as uninitialized
    //   (declare: variable is added to scope)
    // - After compiling the initializer, mark the variable as initialized
    //   (define: variable becomes available to use)
    //
    // Purpose:
    // - Disallow local variables that refer to the same name in their initializer.
    //
    // Example:
    // {
    //   var a = "outer";
    //   {
    //     var a = a;
    //   }
    // }

    uint8_t global = parseVariable(isConst, "Expect variable name.");

    if (match(TOKEN_EQUAL))
    {
        expression();
    }
    else
    {
        emitByte(OP_NIL); // initialized to nil by default
    }
    consume(TOKEN_SEMICOLON,
            "Expect ';' after variable declaration.");

    defineVariable(global, isConst);
}

static void expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP); // discard evaluated result
}

static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

// Skip tokens until reaching statement boundary;
// Used to recover from error and continue parsing
// - purpose: report as much errors as possible.
// - compile() still returns False (hadError = True),
//   so VM doesn't execute code.
static void synchronize()
{
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF)
    {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;

        switch (parser.current.type)
        {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;

        default:; // do nothing
        }

        advance();
    }
}

static void declaration()
{
    if (match(TOKEN_VAR))
    {
        varDeclaration(false);
    }
    else if (match(TOKEN_CONST))
    {
        varDeclaration(true);
    }
    else
    {
        statement();
    }

    if (parser.panicMode)
        synchronize();
}

static void statement()
{
    if (match(TOKEN_PRINT))
    {
        printStatement();
    }
    else if (match(TOKEN_LEFT_BRACE))
    {
        beginScope();
        block();
        endScope();
    }
    else
    {
        expressionStatement();
    }
}

bool compile(const char *source, Chunk *chunk)
{
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance(); // get the first token to 'parser.current'

    while (!match(TOKEN_EOF))
    {
        declaration();
    }

    endCompiler();
    return !parser.hadError;
}