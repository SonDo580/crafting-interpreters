#include <stdio.h>

#include "memory.h"
#include "line.h"

void initLineArray(LineArray *array)
{
    array->count = 0;
    array->capacity = 0;
    array->entries = NULL;
}

void writeLineArray(LineArray *array, int lineNumber)
{
    if (array->count > 0 && array->entries[array->count - 2] == lineNumber)
    {
        array->entries[array->count - 1]++;
        return;
    }

    if (array->count == array->capacity)
    {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->entries = GROW_ARRAY(int, array->entries,
                                    oldCapacity, array->capacity);
    }

    array->entries[array->count] = lineNumber;
    array->entries[array->count + 1] = 1;
    array->count += 2;
}

void freeLineArray(LineArray *array)
{
    FREE_ARRAY(int, array->entries, array->capacity);
    initLineArray(array);
}

// Return the line number that an instruction occurs
int getLine(LineArray *array, int instructionIndex)
{
    if (array->count == 0)
    {
        fprintf(stderr, "Instruction not found\n");
        return -1;
    }

    int i = 0;
    int sum = 0;

    while (i < array->count - 1)
    {
        sum += array->entries[i + 1];
        if (sum > instructionIndex)
        {
            return array->entries[i];
        }
        i += 2;
    }

    fprintf(stderr, "Instruction not found\n");
    return -1;
}