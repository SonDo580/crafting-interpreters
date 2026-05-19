#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
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
    Token name;      // variable name
    int depth;       // scope depth of the block where the variable was declared
    bool isCaptured; // True if the local is captured by any later nested function
} Local;

typedef struct
{
    uint8_t index; // enclosing function's local slot OR enclosing function's upvalue index
    bool isLocal;  // True -> enclosing function's local; False -> enclosing function's upvalue
} Upvalue;

typedef enum
{
    TYPE_FUNCTION,
    TYPE_SCRIPT, // implicit function for top-level code
} FunctionType;

typedef struct Compiler
{
    struct Compiler *enclosing; // Compiler of the enclosing function
    ObjFunction *function;
    FunctionType type;
    Local locals[UINT8_COUNT]; // all locals that are in current function's scope; same order as declaration order
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth; // number of blocks surrounding currently compiling code

    // instruction operand to encode local/upvalue is 1 byte -> limit array size to UINT8_COUNT
} Compiler;

Parser parser;
Compiler *current = NULL;

static Chunk *currentChunk()
{
    return &current->function->chunk;
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

static void emitLoop(int loopStart)
{
    // Jump back to before the condition
    emitByte(OP_LOOP);

    // loopStart: offset in bytecode right before condition expression
    // (+2 to adjust for the jump offset)
    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX)
        error("Loop body too large.");

    // Insert jump offset (big-endian uint16)
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

// Return offset of the emitted instruction
static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    // placeholder (16-bit): how much to offset ip
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitReturn()
{
    emitByte(OP_NIL); // functions implicitly return nil by default
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX)
    {
        error("Too many constants in 1 chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

// Replace placeholder with the calculated jump offset
static void patchJump(int offset)
{
    // jump: how much to offset ip
    // offset: offset of the emitted jump instruction
    // (-2 to adjust for the jump offset itself)
    int jump = currentChunk()->count - offset - 2;
    if (jump > UINT16_MAX)
    {
        error("Too much code to jump over.");
    }

    // Insert jump offset (big-endian uint16)
    currentChunk()->code[offset] = (jump >> 8) & 0xff; // higher byte
    currentChunk()->code[offset + 1] = jump & 0xff;    // lower byte
}
static void initCompiler(Compiler *compiler, FunctionType type)
{
    compiler->enclosing = current; // capture current Compiler

    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction(); // implicit function in case of top-level code

    current = compiler; // set new Compiler as current

    if (type != TYPE_SCRIPT)
    { // initCompiler() is called right after parsing function name -> use previous token
        current->function->name = copyString(parser.previous.start,
                                             parser.previous.length);
    }

    // Claim slot 0 (for VM, to store function being called)
    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    local->name.start = "";
    local->name.length = 0;
}

static ObjFunction *endCompiler()
{
    emitReturn();
    ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
    {
        disassembleChunk(currentChunk(), function->name != NULL
                                             ? function->name->chars
                                             : "<script>");
    }
#endif

    current = current->enclosing; // restore previous Compiler
    return function;
}

static void beginScope()
{
    current->scopeDepth++;
}

static void endScope()
{
    current->scopeDepth--;

    // Free the stack slots for locals
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth >
               current->scopeDepth)
    {
        if (current->locals[current->localCount - 1].isCaptured)
        { // Hoist onto the heap if being closed-over
            emitByte(OP_CLOSE_UPVALUE);
        }
        else
        { // Otherwise just discard it
            emitByte(OP_POP);
        }
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
// which is offset from the base of current call frame in VM stack;
// Return -1 -> not found -> assumed to be global.
static int resolveLocal(Compiler *compiler, Token *name)
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
            return i;
        }
    }

    return -1;
}

// Get or add upvalue to current (function + compiler);
// Return existing index of index after added
static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal)
{
    int upvalueCount = compiler->function->upvalueCount;

    // Reuse resolved upvalue if possible
    for (int i = 0; i < upvalueCount; i++)
    {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal)
        {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT)
    {
        error("Too many closure variables in function.");
        return 0;
    }

    // Add new upvalue
    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

// Looks for a local variable declared in any of the surrounding functions;
// Return an "upvalue index" if found;
// Otherwise return -1 -> assumed to be global.
// (upvalue index: index in 'upvalues' array of current compiler)
static int resolveUpvalue(Compiler *compiler, Token *name)
{
    if (compiler->enclosing == NULL) // reached outermost function
        return -1;

    // Resolve as local variable in enclosing function
    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1)
    {
        // Mark the local variable as being captured
        // (hoist to the heap when enclosing function's stack is discarded)
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    // Resolve as upvalue of any enclosing function (recursive).
    // All (function + compiler)s on the chain will add the resolved value.
    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1)
    {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

// Add variable to compiler's list of variables in current scope
static void addLocal(Token name)
{
    if (current->localCount == UINT8_COUNT)
    {
        error("Too many local variables in function.");
        return;
    }

    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1; // uninitialized
    local->isCaptured = false;
}

// Declare local variable
static void declareVariable()
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

    addLocal(*name);
}

static uint8_t argumentList()
{
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            expression();
            if (argCount == 255)
            {
                error("Can't have more than 255 arguments");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static void and_(bool canAssign)
{
    // if left value (current top of stack) is falsy:
    // - skip right operand
    // - left value is result of the entire expression
    //
    // if left value is truthy:
    // - discard that value (pop)
    // - evaluate right operand which becomes result of the entire expression

    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
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

static void call(bool canAssign)
{
    // Each argument expression generates code that leaves it value on the stack
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void dot(bool canAssign)
{
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);

    if (canAssign && match(TOKEN_EQUAL))
    {
        expression();
        emitBytes(OP_SET_PROPERTY, name);
    }
    else
    {
        emitBytes(OP_GET_PROPERTY, name);
    }
}

static void access(bool canAssign)
{
    expression(); // field expression
    consume(TOKEN_RIGHT_SQUARE, "Expect ']' after field expression");

    if (canAssign && match(TOKEN_EQUAL))
    {
        expression(); // assigned value
        emitByte(OP_SET_DYNAMIC_PROPERTY);
    }
    else
    {
        emitByte(OP_GET_DYNAMIC_PROPERTY);
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

static void or_(bool canAssign)
{
    // if left value (current top of stack) is falsy:
    // - discard that value
    // - evaluate right operand, which becomes result of the entire expression
    //
    // if left value is truthy:
    // - skip right operand
    // - left value is result of the entire expression

    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);

    emitByte(OP_POP);
    parsePrecedence(PREC_OR);

    patchJump(endJump);
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
    int arg = resolveLocal(current, &name);
    if (arg != -1)
    { // local variable
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else if ((arg = resolveUpvalue(current, &name)) != -1)
    { // upvalue
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    }
    else
    { // global variable
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL))
    { // assignment
        expression();
        emitBytes(setOp, (uint8_t)arg);
    }
    else
    { // variable access
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
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_SQUARE] = {NULL, access, PREC_CALL},
    [TOKEN_RIGHT_SQUARE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, dot, PREC_CALL},
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
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
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
    { // no rules to consume the '=' now
        error("Invalid assignment target.");
        // Example: a * b = c * d
    }
}

static uint8_t parseVariable(const char *errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0)
        return 0; // dummy (locals aren't looked up by names)

    return identifierConstant(&parser.previous);
}

// Mark (local) variable as initialized
// (replace sentinel value -1 with real scope depth)
static void markInitialized()
{
    if (current->scopeDepth == 0)
        // a top-level function declaration also calls markInitialized()
        // -> no local variable to mark initialized
        //    (the function is bound to a global variable)
        return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global)
{
    if (current->scopeDepth > 0)
    {
        // VM has executed code for local variable's initializer,
        // and the value is sitting right on top of the stack.
        // -> the temporaries simply become local variables
        markInitialized();
        return;
    }

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

static void function(FunctionType type)
{
    // Create a separate compiler to compile the function
    // (all emitted bytecode goes to the function's chunk)
    Compiler compiler;
    initCompiler(&compiler, type); // init and set as current compiler
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN))
    { // parameters
        do
        {
            current->function->arity++;
            if (current->function->arity > 255)
            {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant); // so that function body can refer to parameters
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    consume(TOKEN_LEFT_BRACE, "Expect '(' before function body.");
    block(); // body

    ObjFunction *function = endCompiler();

    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    // Upvalues that the closure captured
    for (int i = 0; i < function->upvalueCount; i++)
    {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void classDeclaration()
{
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant); // -> body can reference the containing class

    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
}

static void funDeclaration()
{
    // Bind the function (first-class value) to a global/local variable
    uint8_t global = parseVariable("Expect function name");
    markInitialized(); // so that the function can refer to its own name in its body (recursion)
    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration()
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

    uint8_t global = parseVariable("Expect variable name.");

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

    defineVariable(global);
}

static void expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP); // discard evaluated result
}

static void forStatement()
{
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    // (optional) initializer
    if (match(TOKEN_SEMICOLON))
    {
        // no initializer
    }
    else if (match(TOKEN_VAR))
    {
        varDeclaration();
    }
    else
    {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;

    // (optional) condition
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON))
    {
        expression(); // condition
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if condition is falsy
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // discard condition value
    }

    // (optional) increment clause
    if (!match(TOKEN_RIGHT_PAREN))
    {
        // Jump over the increment to execute the body first
        int bodyJump = emitJump(OP_JUMP);

        int incrementStart = currentChunk()->count;
        expression();     // increment clause
        emitByte(OP_POP); // discard value (only need side effect)
        consume(TOKEN_RIGHT_PAREN, "Expect '(' after for clauses.");

        // Jump back to top of 'for' loop
        emitLoop(loopStart);

        loopStart = incrementStart; // to jump back to increment code after executing body
        patchJump(bodyJump);
    }

    statement(); // body

    // Jump back to increment code
    // (or top of 'for' loop if there's no increment)
    emitLoop(loopStart);

    // Patch the exit jump if there's a condition clause
    if (exitJump != -1)
    {
        patchJump(exitJump);
        emitByte(OP_POP); // discard condition value
    }

    endScope();
}

static void ifStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression(); // condition
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    // jump over 'then' branch if condition is false
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // condition is truthy -> pop before code of 'then' branch
    statement();      // 'then' branch

    // jump over 'else' branch after executing 'then' branch
    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP); // condition is falsy -> pop before code of 'else' branch

    if (match(TOKEN_ELSE))
        statement(); // 'else' branch
    patchJump(elseJump);
}

static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void returnStatement()
{
    if (current->type == TYPE_SCRIPT)
    {
        error("Can't return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON))
    {
        emitReturn(); // implicitly return nil
    }
    else
    {
        expression(); // return value
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

static void whileStatement()
{
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression(); // condition
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    // skip body if condition is falsy
    int exitJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP); // discard condition value
    statement();      // body
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP); // discard condition value
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
    if (match(TOKEN_CLASS))
    {
        classDeclaration();
    }
    else if (match(TOKEN_FUN))
    {
        funDeclaration();
    }
    else if (match(TOKEN_VAR))
    {
        varDeclaration();
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
    else if (match(TOKEN_FOR))
    {
        forStatement();
    }
    else if (match(TOKEN_IF))
    {
        ifStatement();
    }
    else if (match(TOKEN_RETURN))
    {
        returnStatement();
    }
    else if (match(TOKEN_WHILE))
    {
        whileStatement();
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

ObjFunction *compile(const char *source)
{
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance(); // get the first token to 'parser.current'

    while (!match(TOKEN_EOF))
    {
        declaration();
    }

    ObjFunction *function = endCompiler();
    return parser.hadError ? NULL : function;
}

void markCompilerRoots()
{
    Compiler *compiler = current;
    while (compiler != NULL)
    {
        markObject((Obj *)compiler->function);
        compiler = compiler->enclosing;
    }
}