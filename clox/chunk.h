#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"
#include "line.h"

// Each instruction has a 1-byte operation code
typedef enum
{
    OP_CONSTANT,
    OP_RETURN
} OpCode;

// A dynamic array to store instructions
typedef struct
{
    int count;
    int capacity;
    uint8_t *code;
    LineArray lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);

#endif