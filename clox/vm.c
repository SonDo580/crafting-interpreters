#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm; // only need 1 VM instance

void initVM()
{
    initStack(&vm.stack);
}

void freeVM()
{
    freeStack(&vm.stack);
    initVM();
}

static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)              \
    do                             \
    {                              \
        double b = pop(&vm.stack); \
        double a = pop(&vm.stack); \
        push(&vm.stack, a op b);   \
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
        case OP_ADD:
            BINARY_OP(+);
            break;
        case OP_SUBTRACT:
            BINARY_OP(-);
            break;
        case OP_MULTIPLY:
            BINARY_OP(*);
            break;
        case OP_DIVIDE:
            BINARY_OP(/);
            break;
        case OP_NEGATE:
            push(&vm.stack, -pop(&vm.stack));
            break;
        case OP_RETURN:
            printValue(pop(&vm.stack));
            printf("\n");
            return INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(Chunk *chunk)
{
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}
