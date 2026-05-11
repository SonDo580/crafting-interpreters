#

## Idea

- Modify `identifierConstant()`:
  - don't always add a new constant.
  - lookup index of an existing constant that points to the same string.
- Tradeoff:
  - increase compile time since we have to loop through the chunk's constants (bad if there're lots of variable access).

## Verify

- The operand of `OP_GET_GLOBAL` should be the same (index) for the same variable name

```
var a = 2; print a + a; print 2;

0000    1 OP_CONSTANT         1 '2'
0002    | OP_DEFINE_GLOBAL    0 'a'
0004    | OP_GET_GLOBAL       0 'a'
0006    | OP_GET_GLOBAL       0 'a'
0008    | OP_ADD
0010    | OP_CONSTANT         2 '2'
0012    | OP_PRINT
0013    2 OP_RETURN
```
