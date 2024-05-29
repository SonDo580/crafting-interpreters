#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "value.h"
#include "line.h"

void disassembleChunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;)
    {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t constantIndex = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constantIndex);
    printValue(chunk->constants.values[constantIndex]);
    printf("'\n");
    return offset + 2;
}

static int simpleInstruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

// Return value: the offset of the next instruction
int disassembleInstruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);
    int currentLine = getLine(&chunk->lines, offset);
    if (currentLine == -1)
    {
        exit(EXIT_FAILURE);
    }

    if (offset > 0 &&
        currentLine == getLine(&chunk->lines, offset - 1))
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", currentLine);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
    case OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}