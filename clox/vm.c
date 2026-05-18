#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

VM vm;

// Return elapsed time since program started running, in seconds
static Value clockNative(int argCount, Value *args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void resetStack()
{
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

static void runtimeError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // Stack trace: print out functions that were executing,
    // and where the execution was in each function
    for (int i = vm.frameCount - 1; i >= 0; i--)
    {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->closure->function;
        // (-1 since the interpreter advances past an instruction before executing it)
        size_t instruction = (frame->ip - 1) - function->chunk.code;
        int line = function->chunk.lines[instruction];

        fprintf(stderr, "[line %d] in ", line);
        if (function->name == NULL)
        {
            fprintf(stderr, "script\n");
        }
        else
        {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    resetStack();
}

// - function: pointer to a C function
// - name: name it will be known as in Lox
static void defineNative(const char *name, NativeFn function)
{
    // copyString() and newNative() dynamically allocate memory
    // -> can trigger garbage collection
    // -> push on stack so GC doesn't free them
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM()
{
    resetStack();
    vm.objects = NULL;

    initTable(&vm.globals);
    initTable(&vm.strings);

    defineNative("clock", clockNative);
}

void freeVM()
{
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
}

void push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop()
{
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance)
{
    return vm.stackTop[-1 - distance];
}

// Initialize the next CallFrame on the stack
static bool call(ObjClosure *closure, int argCount)
{
    if (argCount != closure->function->arity)
    {
        runtimeError("Expect %d arguments but got %d.",
                     closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX)
    {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = (vm.stackTop - 1) - argCount;
    // frame->slots points to current function, parameters start from slot 1
    return true;
}

static bool callValue(Value callee, int argCount)
{
    if (IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee))
        {
        case OBJ_CLOSURE:
            return call(AS_CLOSURE(callee), argCount);
        case OBJ_NATIVE:
            NativeFn native = AS_NATIVE(callee);
            Value result = native(argCount, vm.stackTop - argCount);
            vm.stackTop -= argCount + 1;
            push(result);
            return true;
        default:
            break; // non-callable object type
        }
    }

    runtimeError("Can only call functions and classes.");
    return false;
}

// Create upvalue from local variable of closure's enclosing function;
// MUST use existing open upvalue if point to the same slot;
// (multiple closures accessing the same variable should end up in the same location)
static ObjUpvalue *captureUpvalue(Value *local)
{
    // Find existing open upvalue that points to the same local slot
    // OR where to insert the new upvalue
    // (order by decreasing stack slot index)
    ObjUpvalue *prevUpvalue = NULL;
    ObjUpvalue *upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local)
    {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local)
    { // Found existing open upvalue that points to the same local slot
        return upvalue;
    }

    // Create and insert the new upvalue
    ObjUpvalue *createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL)
    {
        vm.openUpvalues = createdUpvalue;
    }
    else
    {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

// Close every open upvalue with slot >= last (from 'last' towards top of stack);
// Move the locals from stack to heap
static void closeUpvalues(Value *last)
{
    while (vm.openUpvalues != NULL &&
           vm.openUpvalues->location >= last)
    {
        ObjUpvalue *upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location; // copy local variable's value to 'closed' (on heap)
        upvalue->location = &upvalue->closed; // point 'location' to 'closed' (get/set upvalue instructions use 'location')
        vm.openUpvalues = upvalue->next;      // remove from 'openUpvalues' linked-list
    }
}

static bool isFalsy(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate()
{
    ObjString *b = AS_STRING(pop());
    ObjString *a = AS_STRING(pop());

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);
    push(OBJ_VAL(result));
}

static InterpretResult run()
{
    CallFrame *frame = &vm.frames[vm.frameCount - 1]; // current frame

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
     (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1])) // byte order: big-endian

#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op)                        \
    do                                                  \
    {                                                   \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) \
        {                                               \
            runtimeError("Operands must be numbers.");  \
            return INTERPRET_RUNTIME_ERROR;             \
        }                                               \
        double b = AS_NUMBER(pop());                    \
        double a = AS_NUMBER(pop());                    \
        push(valueType(a op b));                        \
    } while (false)
    /*
    do {...} while (false) <-> {...}, but allow semicolon at the end.

    if (cond)
        MACRO(); // using {...} would result in compile error
    else
        other();
    */

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        // show stack content
        printf("         ");
        for (Value *slot = vm.stack; slot < vm.stackTop; slot++)
        {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");

        disassembleInstruction(
            &frame->closure->function->chunk,
            (int)(frame->ip - frame->closure->function->chunk.code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_CONSTANT:
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        case OP_NIL:
            push(NIL_VAL);
            break;
        case OP_TRUE:
            push(BOOL_VAL(true));
            break;
        case OP_FALSE:
            push(BOOL_VAL(false));
            break;
        case OP_POP:
            pop();
            break;
        case OP_GET_LOCAL:
        {
            uint8_t slot = READ_BYTE(); // relative to base of current frame
            push(frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL:
        {
            uint8_t slot = READ_BYTE(); // relative to base of current frame
            frame->slots[slot] = peek(0);
            // don't pop (assignment is an expression)
            break;
        }
        case OP_GET_GLOBAL:
        {

            ObjString *name = READ_STRING();
            Value value;
            if (!tableGet(&vm.globals, name, &value))
            {
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OP_DEFINE_GLOBAL:
        {
            ObjString *name = READ_STRING();
            tableSet(&vm.globals, name, peek(0));
            pop();
            break;
            // don't pop the value until after we add it to the hash table
            // (ensure VM can still find the value if garbage collection
            //  is triggered during the adding process)
        }
        case OP_SET_GLOBAL:
        {
            ObjString *name = READ_STRING();
            if (tableSet(&vm.globals, name, peek(0)))
            { // variable hasn't been defined -> runtime error
                // Must delete the added entry since REPL keeps running after runtime error
                tableDelete(&vm.globals, name);
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            // don't pop (assignment is an expression)
            break;
        }
        case OP_GET_UPVALUE:
        {
            uint8_t slot = READ_BYTE();
            push(*frame->closure->upvalues[slot]->location);
            break;
        }
        case OP_SET_UPVALUE:
        {
            uint8_t slot = READ_BYTE();
            *frame->closure->upvalues[slot]->location = peek(0);
            // don't pop (assignment is an expression)
            break;
        }
        case OP_EQUAL:
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(valuesEqual(a, b)));
            break;
        case OP_GREATER:
            BINARY_OP(BOOL_VAL, >);
            break;
        case OP_LESS:
            BINARY_OP(BOOL_VAL, <);
            break;
        case OP_ADD:
            if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
            {
                concatenate();
            }
            else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
            {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            }
            else
            {
                runtimeError("Operands must be 2 numbers or 2 strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        case OP_SUBTRACT:
            BINARY_OP(NUMBER_VAL, -);
            break;
        case OP_MULTIPLY:
            BINARY_OP(NUMBER_VAL, *);
            break;
        case OP_DIVIDE:
            BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_NOT:
            push(BOOL_VAL(isFalsy(pop())));
            break;
        case OP_NEGATE:
            if (!IS_NUMBER(peek(0)))
            {
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            break;
        case OP_PRINT:
            printValue(pop());
            printf("\n");
            break;
        case OP_JUMP:
        {
            uint16_t offset = READ_SHORT();
            frame->ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE:
        {
            uint16_t offset = READ_SHORT();
            if (isFalsy(peek(0)))
                frame->ip += offset;
            // don't pop condition value (used for short-circuiting by logical operators)
            break;
        }
        case OP_LOOP:
        {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset; // jump backward to loopStart
            break;
        }
        case OP_CALL:
        {
            // - To perform a call, we need a CallFrame initialized with
            //   the function being called and stack slots that it can use
            // - The top of caller's stack window contains the function being called
            //   follow by the arguments in order.
            //   The bottom of the callee's stack window overlaps so that the parameter slots
            //   exactly line up with the arguments.

            int argCount = READ_BYTE();
            // find function: count past the argument slots from top of stack
            if (!callValue(peek(argCount), argCount))
            {
                return INTERPRET_RUNTIME_ERROR;
            }

            // if callValue() is successful, there will be a new CallFrame
            // (native function calls don't create new CallFrame)
            frame = &vm.frames[vm.frameCount - 1]; // update run()'s cached pointer to current frame
            break;
        }
        case OP_CLOSURE:
        {
            ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
            ObjClosure *closure = newClosure(function);
            push(OBJ_VAL(closure));

            // Fill the upvalue array
            // (Note that current function is enclosing function of newly created closure)
            for (int i = 0; i < closure->upvalueCount; i++)
            {
                uint8_t isLocal = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (isLocal)
                { // upvalue is enclosing function's local
                    closure->upvalues[i] = captureUpvalue(frame->slots + index);
                }
                else
                { // upvalue is enclosing function's upvalue
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }

            break;
        }
        case OP_CLOSE_UPVALUE:
            // the variable needed hoisting is currently on top of stack
            closeUpvalues(vm.stackTop - 1);
            pop();
            break;
        case OP_RETURN:
            Value result = pop();        // save returned value
            closeUpvalues(frame->slots); // close all open upvalues owned by the returning function

            vm.frameCount--; // discard CallFrame of the returning function
            if (vm.frameCount == 0)
            { // finish executing top-level code
                pop();
                return INTERPRET_OK;
            }

            vm.stackTop = frame->slots;            // discard the called function's stack window
            push(result);                          // push returned value to top of stack
            frame = &vm.frames[vm.frameCount - 1]; // update run()'s cached pointer to current frame
            break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char *source)
{
    ObjFunction *function = compile(source); // top-level function
    if (function == NULL)
        return INTERPRET_COMPILE_ERROR;

    // newClosure() allocate memory dynamically -> can trigger garbage collection
    // -> push 'function' (no references) to stack so it is not freed,
    //    pop it after the closure has been created.
    push(OBJ_VAL(function));
    ObjClosure *closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure)); // put top-level closure at stack slot 0
    call(closure, 0);       // set up initial CallFrame

    return run();
}
