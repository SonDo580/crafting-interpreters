# Challenge 14.2

Because `OP_CONSTANT` uses only a single byte for its operand, a chunk may only contain up to 256 different constants. That’s small enough that people writing real-world code will hit that limit. We could use two or more bytes to store the operand, but that makes every constant instruction take up more space. Most chunks won’t need that many unique constants, so that wastes space and sacrifices some locality in the common case to support the rare case.

To balance those two competing aims, many instruction sets feature multiple instructions that perform the same operation but with operands of different sizes. Leave our existing one-byte `OP_CONSTANT` instruction alone, and define a second `OP_CONSTANT_LONG` instruction. It stores the operand as a 24-bit number, which should be plenty.

Implement this function:

```
void writeConstant(Chunk* chunk, Value value, int line) {
  // Implement me...
}
```

It adds `value` to `chunk`’s constant array and then writes an appropriate instruction to load the constant. Also add support to the disassembler for `OP_CONSTANT_LONG` instructions.

Defining two instructions seems to be the best of both worlds. What sacrifices, if any, does it force on us?

## Idea

- Call `addConstant()` and receive `constantIndex`
- If `constantIndex < 2^8`, use `OP_CONSTANT` instruction
- If `2^8 <= constantIndex < 2^16`, use `OP_CONSTANT_LONG` instruction. Use Little Endian when putting the constant into the byte array.
