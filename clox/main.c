#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char *argv[])
{
    Chunk chunk;
    initChunk(&chunk);

    // exhaust indices representable with OP_CONSTANT instruction (2^8)
    for (int i = 0; i < 256; i++)
    {
        writeConstant(&chunk, i * 0.5, 1);
    }

    // this will use OP_CONSTANT_LONG instruction
    writeConstant(&chunk, 1.2, 2);

    writeChunk(&chunk, OP_RETURN, 2);

    disassembleChunk(&chunk, "test chunk");

    freeChunk(&chunk);
    return 0;
}