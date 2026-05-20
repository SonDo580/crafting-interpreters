#ifndef clox_value_h
#define clox_value_h

#include <string.h>

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

/*
- "Problem": On a 64-bit machine, Value type takes up 16 bytes
  . type tag: 4 bytes
  . padding: 4 bytes (to keep the union aligned to 8 byte boundary)
  . union: 8 bytes (largest field is pointer or double)
- If we can cut that down:
   . the VM could pack more values into the same amount of memory
   . more values fit in a cache line -> fewer cache misses

- 64-bit, double precision, IEEE floating-point number:
  . 63: sign bit
  . 52 -> 62: 11 exponent bits
  . 0 -> 51: 52 mantissa (fraction, significand) bits
- When all exponent bits are set, the value represents NaN (not a number).
  NaNs where the highest mantissa bit (51) is 0 are signaling NaNs,
  and other are quiet NaNs.
  . Signaling NaNs represent errors and may cause aborting a program.
  . Quiet NaNs are safer to use.
- For each quiet NaN, we have 51 bits remaining bits to use:
  . - 11 exponent bits (62 -> 52)
  . - highest mantissa bit (51)
  . - bit for Intel's "QNaN Floating-Point Indefinite" (50)
  . -> remain: (0 -> 49) + sign bit (63)
=> A 64-bit double has enough room to store a numeric floating-point values
   and has room for another 51 bits of data that we can use.

- Technically, pointers on a 64-bit architecture are 64-bits,
  but most widely used chips today only ever use the low 48 bits,
  -> 48 bits to represent pointers (address range)

- NaN boxing relies on low-level details of how chip represents FP numbers and pointers
  -> select Value representation at compile time with NAN_BOXING flag
     (use tagged union if not set)
*/

#define SIGN_BIT ((uint64_t)0x8000000000000000)
// 1 0 ... 0 (63 0's)

#define QNAN ((uint64_t)0x7ffc000000000000)
// 0  | 1  ... 1  | 1  1  0  ... 0
// 63   62     52   51 50 49     0

#define TAG_NIL 1   // 01
#define TAG_FALSE 2 // 10
#define TAG_TRUE 3  // 11

typedef uint64_t Value;

// Check Lox value type
#define IS_BOOL(value) (((value) | 1) == TRUE_VAL)
#define IS_NIL(value) ((value) == NIL_VAL)
#define IS_NUMBER(value) (((value) & QNAN) != QNAN)
#define IS_OBJ(value) \
  (((value) & (SIGN_BIT | QNAN)) == (SIGN_BIT | QNAN))

// Lox value -> C value
#define AS_BOOL(value) ((value) == TRUE_VAL)
#define AS_NUMBER(value) valueToNum(value)
#define AS_OBJ(value) \
  ((Obj *)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

// C value -> Lox value
#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
#define NUMBER_VAL(num) numToValue(num)
#define OBJ_VAL(obj) \
  (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj)) // Obj* -> Value

static inline double valueToNum(Value value)
{
  // Efficient since most compilers optimize type punning
  // (see notes in numToValue())
  double num;
  memcpy(&num, &value, sizeof(Value));
  return num;
}

static inline Value numToValue(double num)
{
  // Efficient since most compilers optimize type punning
  Value value;
  memcpy(&value, &num, sizeof(double));
  return value;

  /*
  - Without optimization:
    . get the double from FP register and store the bytes on RAM at 'num' address.
    . copy the bytes from 'num' address to 'value' address.
    . load the bytes on RAM at 'value' address into the integer register for return value.
  - But most compilers recognize this pattern
    (type punning: re-interpret the exact raw bits of double as uint64_t)
    and optimize away the memcpy() entirely.
    . emits a single instruction that copies the bits
      from the FP register to the integer register.
      (the bits don't move through RAM at all)
  */
}

#else

typedef enum
{
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ,
} ValueType;

typedef struct
{
  ValueType type;
  union
  {
    bool boolean;
    double number;
    Obj *obj;
  } as;
} Value;

// check Lox Value's type
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

// Unpack Lox Value to C value
#define AS_OBJ(value) ((value).as.obj)
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

// Convert C value to Lox Value
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj *)object}})

#endif

// The constant pool
// (load-constant instruction looks up the value by index)
typedef struct
{
  int capacity;
  int count;
  Value *values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);

#endif