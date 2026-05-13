#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

typedef enum
{
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
} ObjType;

struct Obj
{
    ObjType type;
    struct Obj *next; // next Obj in all-objects linked-list
};

typedef struct
{
    Obj obj;   // must be 1st field
    int arity; // number of parameters
    Chunk chunk;
    ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct
{
    Obj obj;           // must be 1st field
    NativeFn function; // pointer to the C function that implement the native behavior
} ObjNative;

struct ObjString
{
    Obj obj; // must be 1st field
    int length;
    char *chars;
    uint32_t hash; // calculate once (Lox strings are immutable)
};

ObjFunction *newFunction();
ObjNative *newNative(NativeFn function);
ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
    // Why not just put this in the macro:
    // - Because the body uses 'value' twice.
    // - If a macro uses a parameter more than once,
    //   that expression gets evaluated multiple times
    //   (bad if expresion has side effects).
}

#endif
