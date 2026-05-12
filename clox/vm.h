#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"
#include "value.h"
#include "vm_stack.h"

typedef struct
{
    Chunk *chunk;
    uint8_t *ip; // instruction pointer
    Stack stack;
    Obj *objects;  // all allocated objects
    Table globals; // global variables
    Table strings; // all (unique) strings created
} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char *source);

#endif