#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

// represent an ongoing function call
typedef struct
{
    ObjFunction *function; // function being called
    Value *slots;          // 1st slot on VM's stack this function can use (base pointer / frame pointer)
} CallFrame;

typedef struct
{
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    Value stack[STACK_MAX];
    Value *stackTop; // 1 past last item
    Obj *objects;    // all allocated objects
    Table globals;   // global variables
    Table strings;   // all (unique) strings created
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
void push(Value value);
Value pop();

#endif