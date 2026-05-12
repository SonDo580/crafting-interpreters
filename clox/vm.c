#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

VM vm;

static void runtimeError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // Get offset of the instruction being executed
    // (the interpreter advances past each instruction before executing it)
    size_t instruction = (vm.ip - 1) - vm.chunk->code;

    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    initStack(&vm.stack);
}

void initVM()
{
    initStack(&vm.stack);
    vm.objects = NULL;
    initTable(&vm.globals);
    initTable(&vm.strings);
}

void freeVM()
{
    freeStack(&vm.stack);
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
}

static bool isFalsy(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate()
{
    ObjString *b = AS_STRING(pop(&vm.stack));
    ObjString *a = AS_STRING(pop(&vm.stack));

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);
    push(&vm.stack, OBJ_VAL(result));
}

static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_LONG() readLong((vm.ip += 2) - 2)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op)                       \
    do                                                 \
    {                                                  \
        if (!IS_NUMBER(peek(&vm.stack, 0)) ||          \
            !IS_NUMBER(peek(&vm.stack, 1)))            \
        {                                              \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR;            \
        }                                              \
        double b = AS_NUMBER(pop(&vm.stack));          \
        double a = AS_NUMBER(pop(&vm.stack));          \
        push(&vm.stack, valueType(a op b));            \
    } while (false)
    /*
    do {...} while (false) <-> {...}, but allow semicolon at the end.

    if (cond)
        MACRO(); // using {...} would result in compile error
    else
        other();
    */

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printStack(&vm.stack);
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_CONSTANT:
            Value constant = READ_CONSTANT();
            push(&vm.stack, constant);
            break;
        case OP_NIL:
            push(&vm.stack, NIL_VAL);
            break;
        case OP_TRUE:
            push(&vm.stack, BOOL_VAL(true));
            break;
        case OP_FALSE:
            push(&vm.stack, BOOL_VAL(false));
            break;
        case OP_POP:
            pop(&vm.stack);
            break;
        case OP_GET_LOCAL:
        {
            uint16_t slot = READ_LONG();
            push(&vm.stack, peekAt(&vm.stack, slot));
            break;
        }
        case OP_SET_LOCAL:
        {
            uint16_t slot = READ_LONG();
            setAt(&vm.stack, slot, peek(&vm.stack, 0));
            // don't pop (assignment is an expression)
            break;
        }
        case OP_GET_GLOBAL:
        {

            ObjString *name = READ_STRING();
            Value value;
            if (!tableGet(&vm.globals, name, &value))
            {
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(&vm.stack, value);
            break;
        }
        case OP_DEFINE_GLOBAL:
        {
            ObjString *name = READ_STRING();
            tableSet(&vm.globals, name, peek(&vm.stack, 0));
            pop(&vm.stack);
            break;
            // don't pop the value until after we add it to the hash table
            // (ensure VM can still find the value if garbage collection
            //  is triggered during the adding process)
        }
        case OP_SET_GLOBAL:
        {
            ObjString *name = READ_STRING();
            if (tableSet(&vm.globals, name, peek(&vm.stack, 0)))
            { // variable hasn't been defined -> runtime error
                // Must delete the added entry since REPL keeps running after runtime error
                tableDelete(&vm.globals, name);
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            // don't pop (assignment is an expression)
            break;
        }
        case OP_EQUAL:
            Value b = pop(&vm.stack);
            Value a = pop(&vm.stack);
            push(&vm.stack, BOOL_VAL(valuesEqual(a, b)));
            break;
        case OP_GREATER:
            BINARY_OP(BOOL_VAL, >);
            break;
        case OP_LESS:
            BINARY_OP(BOOL_VAL, <);
            break;
        case OP_ADD:
            if (IS_STRING(peek(&vm.stack, 0)) &&
                IS_STRING(peek(&vm.stack, 1)))
            {
                concatenate();
            }
            else if (IS_NUMBER(peek(&vm.stack, 0)) &&
                     IS_NUMBER(peek(&vm.stack, 1)))
            {
                double b = AS_NUMBER(pop(&vm.stack));
                double a = AS_NUMBER(pop(&vm.stack));
                push(&vm.stack, NUMBER_VAL(a + b));
            }
            else
            {
                runtimeError("Operands must be 2 numbers or 2 strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        case OP_SUBTRACT:
            BINARY_OP(NUMBER_VAL, -);
            break;
        case OP_MULTIPLY:
            BINARY_OP(NUMBER_VAL, *);
            break;
        case OP_DIVIDE:
            BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_NOT:
            push(&vm.stack, BOOL_VAL(isFalsy(pop(&vm.stack))));
            break;
        case OP_NEGATE:
            if (!IS_NUMBER(peek(&vm.stack, 0)))
            {
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(&vm.stack, NUMBER_VAL(-AS_NUMBER(pop(&vm.stack))));
            break;
        case OP_PRINT:
            printValue(pop(&vm.stack));
            printf("\n");
            break;
        case OP_RETURN:
            // Exit the interpreter
            return INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_LONG
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char *source)
{
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk))
    {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}
