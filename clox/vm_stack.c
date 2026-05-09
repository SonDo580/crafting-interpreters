#include <stdlib.h>
#include <stdio.h>

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

// - Expand: double capacity when count == capacity
// - Shrink: not implement (can halve capacity when count == capacity / 4)

void push(Stack *stack, Value value)
{
    if (stack->capacity == stack->count)
    {
        int oldCapacity = stack->capacity;
        stack->capacity = GROW_CAPACITY(oldCapacity);
        stack->items = GROW_ARRAY(Value, stack->items,
                                  oldCapacity, stack->capacity);
    }

    stack->items[stack->count] = value;
    stack->count++;
}

Value pop(Stack *stack)
{
    if (stack->count == 0)
    {
        fprintf(stderr, "pop empty stack\n");
        exit(EXIT_FAILURE);
    }
    return stack->items[--(stack->count)];
}

// show stack content
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