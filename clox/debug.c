#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassembleChunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;)
    {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(int type, const char *name, Chunk *chunk, int offset)
{
    int constantIndex = (int)(chunk->code[offset + 1]);
    int nextOffset = offset + 2;
    if (type == OP_CONSTANT_LONG)
    {
        // use LittleEndian (low-order byte at lower address)
        constantIndex += (int)(chunk->code[offset + 2]) << 8;
        nextOffset = offset + 3;
    }
    
    printf("%-16s %5d '", name, constantIndex);
    printValue(chunk->constants.values[constantIndex]);
    printf("'\n");
    return nextOffset;
}

static int simpleInstruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

// Return the offset of the next instruction
int disassembleInstruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
    case OP_CONSTANT:
        return constantInstruction(OP_CONSTANT, "OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_LONG:
        return constantInstruction(OP_CONSTANT_LONG, "OP_CONSTANT_LONG", chunk, offset);
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}