#ifndef clox_vm_stack_h
#define clox_vm_stack_h

#include "value.h"

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
void printStack(Stack *stack);

#endif