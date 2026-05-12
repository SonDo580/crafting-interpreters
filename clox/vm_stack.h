#ifndef clox_vm_stack_h
#define clox_vm_stack_h

#include "value.h"

#define STACK_MAX (1 << 16)

typedef struct
{
    int capacity;
    int count;
    Value *items;
} Stack;

void initStack(Stack *stack);
void freeStack(Stack *stack);
void push(Stack *stack, Value value);
Value pop(Stack *stack);
Value peek(Stack *stack, int distance);
Value peekAt(Stack *stack, int index);
void setAt(Stack *stack, int index, Value value);
void printStack(Stack *stack);

#endif