#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type *)allocateObject(sizeof(type), objectType)

static Obj *allocateObject(size_t size, ObjType type)
{
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = type;

    // insert to all-objects linked-list
    object->next = vm.objects;
    vm.objects = object;

    return object;
}

// Create string object
static ObjString *allocateString(char *chars, int length, bool constant)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->constant = constant;
    return string;
}

// Create string object ("own" the given string, don't need copy)
ObjString *takeString(char *chars, int length)
{
    return allocateString(chars, length, false);
}

// Create string object that points to source string
ObjString *takeConstantString(const char *chars, int length)
{
    return allocateString((char*)chars, length, true);
}

void printObject(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING:
        // must specify length since constant strings don't have '\0' terminator
        printf("%.*s", AS_STRING(value)->length, AS_CSTRING(value));
        break;
    }
}