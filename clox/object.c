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
static ObjString *allocateString(char *chars, int length)
{
    size_t size = sizeof(ObjString) + (length + 1) * sizeof(char);
    ObjString *string = (ObjString *)allocateObject(size, OBJ_STRING);
    string->length = length;
    memcpy(string->chars, chars, length + 1);
    return string;
}

// Create string object ("own" the given string, don't need copy)
ObjString *takeString(char *chars, int length)
{
    return allocateString(chars, length);
}

// Extract string value from the lexeme & Create string object
ObjString *copyString(const char *chars, int length)
{
    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length);
}

void printObject(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    }
}