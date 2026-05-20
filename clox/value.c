#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

void initValueArray(ValueArray *array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray *array, Value value)
{
    if (array->capacity == array->count)
    {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values,
                                   oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray *array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value)
{
#ifdef NAN_BOXING
    if (IS_BOOL(value))
    {
        printf(AS_BOOL(value) ? "true" : "false");
    }
    else if (IS_NIL(value))
    {
        printf("nil");
    }
    else if (IS_NUMBER(value))
    {
        printf("%g", AS_NUMBER(value));
    }
    else if (IS_OBJ(value))
    {
        printObject(value);
    }
#else
    switch (value.type)
    {
    case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        printf("nil");
        break;
    case VAL_NUMBER:
        printf("%g", AS_NUMBER(value));
        break;
    case VAL_OBJ:
        printObject(value);
        break;
    }
#endif
}

bool valuesEqual(Value a, Value b)
{
#ifdef NAN_BOXING
    if (IS_NUMBER(a) && IS_NUMBER(b))
    {
        // - In case a and b bit patterns are the same and represent NaN
        //   a == b will return True for uint64_t (Value) comparison.
        // - To be fully compliant with IEEE 754 (NaN != NaN),
        //   convert them to C values then compare.
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    return a == b;
#else
    if (a.type != b.type)
        return false;

    switch (a.type)
    {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
        return true;
    case VAL_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:
        // For strings: pointer equality <-> value equality, since "string interning" is used
        return AS_OBJ(a) == AS_OBJ(b);
    default:
        return false; // unreachable
    }

    // Why don't just 'memcmp()' 2 Value structs:
    // - a Value contains unused bits due to padding and different-sized union fields,
    //   and C gives no guarantee about what is in those
#endif
}
