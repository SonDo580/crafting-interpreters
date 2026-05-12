# Challenge 22.4

Extend `clox` to allow more than 256 local variables to be in scope at a time

## Idea

- Local variables (indices) are pushed on stack -> to support more than 256 local variables in scope at a time, we need to grow the stack (currently limit to 256 slots)
- Implement:
  - let the stack be a dynamic array, with max size 65536 _(2^16, pretty big for a stack)_, which is also maximum number of local variables in scope
  - to index into that array, `OP_GET_LOCAL` and `OP_SET_LOCAL` operand needs 2 bytes _(we may use a separate instruction for large index, but I don't do that to simplify things)_.
