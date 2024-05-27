#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;

// The constant pool is a dynamic array of values
// An instruction to load a constant looks up the value by index
typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);

#endif