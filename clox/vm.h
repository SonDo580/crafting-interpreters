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
    Value callable; // function or closure being called
    uint8_t *ip;     // instruction pointer (to current function's code)
    Value *slots;    // 1st slot on VM's stack this function can use (base pointer / frame pointer)

    // Instead of storing return address in callee's frame, caller stores its own 'ip'
    // When we return from a function, VM will jump to 'ip' of caller's CallFrame and resume
} CallFrame;

typedef struct
{
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    Value stack[STACK_MAX];
    Value *stackTop;          // 1 past last item
    Table globals;            // global variables
    Table strings;            // all (unique) strings created
    ObjUpvalue *openUpvalues; // all open upvalues (still on stack)
    Obj *objects;             // all allocated objects
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