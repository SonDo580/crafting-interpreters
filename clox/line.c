#include <stdio.h>

#include "memory.h"
#include "line.h"

void initLineArray(LineArray *array)
{
    array->capacity = 0;
    array->count = 0;
    array->lines_rle = NULL;
}

void writeLineArray(LineArray *array, int line)
{
    // item is still on the same line -> increase item count
    if (array->count > 0 && array->lines_rle[array->count - 2] == line)
    {
        array->lines_rle[array->count - 1]++;
        return;
    }

    // grow the array if needed
    if (array->count == array->capacity)
    {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->lines_rle = GROW_ARRAY(int, array->lines_rle,
                                      oldCapacity, array->capacity);
    }

    // add new line entry and set count to 1
    array->lines_rle[array->count] = line;
    array->lines_rle[array->count + 1] = 1; // count is always multiple of 2 -> no out-of-bound
    array->count += 2;
}

void freeLineArray(LineArray *array)
{
    FREE_ARRAY(int, array->lines_rle, array->capacity);
    initLineArray(array);
}

// Return the line number that an instruction occur
int getLine(LineArray *array, int offset)
{
    int i = 0;
    int count = 0;

    while (i < array->count - 1)
    {
        count += array->lines_rle[i + 1];
        if (count > offset)
        {
            return array->lines_rle[i];
        }
        i += 2;
    }

    fprintf(stderr, "Instruction not found\n");
    return -1;
}