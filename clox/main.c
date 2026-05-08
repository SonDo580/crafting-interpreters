#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char *argv[])
{
    Chunk chunk;
    initChunk(&chunk);

    int constantIndex = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, constantIndex, 1);

    writeChunk(&chunk, OP_RETURN, 1);

    constantIndex = addConstant(&chunk, 2.4);
    writeChunk(&chunk, OP_CONSTANT, 2);
    writeChunk(&chunk, constantIndex, 2);

    writeChunk(&chunk, OP_RETURN, 2);
    writeChunk(&chunk, OP_RETURN, 3);

    disassembleChunk(&chunk, "test chunk");

    freeChunk(&chunk);
    return 0;
}