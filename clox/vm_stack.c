#include <stdio.h>
#include <stdlib.h>

#include "vm_stack.h"
#include "memory.h"

void initStack(Stack *stack)
{
    stack->capacity = 0;
    stack->count = 0;
    stack->items = NULL;
}

void freeStack(Stack *stack)
{
    FREE_ARRAY(Value, stack->items, stack->capacity);
    initStack(stack);
}

void push(Stack *stack, Value value)
{
    if (stack->capacity == stack->count)
    {
        int oldCapacity = stack->capacity;
        stack->capacity = GROW_CAPACITY(oldCapacity);
        if (stack->capacity > STACK_MAX)
        {
            fprintf(stderr, "Stack overflow.");
            exit(EXIT_FAILURE);
        }
        stack->items = GROW_ARRAY(Value, stack->items,
                                  oldCapacity, stack->capacity);
    }

    stack->items[stack->count++] = value;
}

Value pop(Stack *stack)
{
    if (stack->count == 0)
    {
        fprintf(stderr, "pop empty stack");
        exit(EXIT_FAILURE);
    }
    return stack->items[--stack->count];
}

Value peek(Stack *stack, int distance)
{
    if (stack->count - distance <= 0)
    {
        fprintf(stderr, "peek invalid distance");
        exit(EXIT_FAILURE);
    }
    return stack->items[stack->count - 1 - distance];
}

Value peekAt(Stack *stack, int index)
{
    if (index < 0 || index >= stack->count)
    {
        fprintf(stderr, "invalid index");
        exit(EXIT_FAILURE);
    }
    return stack->items[index];
}

void setAt(Stack *stack, int index, Value value)
{
    if (index < 0 || index >= stack->count)
    {
        fprintf(stderr, "invalid index");
        exit(EXIT_FAILURE);
    }
    stack->items[index] = value;
}

void printStack(Stack *stack)
{
    printf("         ");
    for (int i = 0; i < stack->count; i++)
    {
        printf("[ ");
        printValue(stack->items[i]);
        printf(" ]");
    }
    printf("\n");
}