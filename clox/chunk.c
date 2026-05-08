#include <stdlib.h>
#include <stdio.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line)
{
    if (chunk->capacity == chunk->count)
    {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
                                 oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines,
                                  oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

// Return the index where the constant was appended
int addConstant(Chunk *chunk, Value value)
{
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

// Add value to constant array then write the appropriate instruction
void writeConstant(Chunk *chunk, Value value, int line)
{
    int constantIndex = addConstant(chunk, value);
    if (constantIndex < (1 << 8))
    {
        writeChunk(chunk, OP_CONSTANT, line);
        writeChunk(chunk, constantIndex, line);
    }
    else if (constantIndex < (1 << 16))
    {
        writeChunk(chunk, OP_CONSTANT_LONG, line);

        // use Little Endian (low-order byte at lower address)
        writeChunk(chunk, (uint8_t)constantIndex, line); // truncate to low-order byte
        writeChunk(chunk, (constantIndex >> 8), line);
    }
    else
    {
        fprintf(stderr, "too many constants\n");
        exit(EXIT_FAILURE);
    }
}