# Challenge 23.2

In jlox, we had a challenge to add support for `break` statements. This time, let’s do `continue`:

```
continueStmt   → "continue" ";"
```

A `continue` statement jumps directly to the top of the nearest enclosing loop, skipping the rest of the loop body. Inside a `for` loop, a continue jumps to the increment clause, if there is one. It’s a compile-time error to have a `continue` statement not enclosed in a loop.

Make sure to think about scope. What should happen to local variables declared inside the body of the loop or in blocks nested inside the loop when a continue is executed?

## Idea

- The `statement()` parse function needs an extra parameter `loopStart`, which points to top of the nearest enclosing loop.
  - Propagate that `loopStart` to statement parse functions that can contain statements (`declaration()`, `ifStatement()`, `block()`)
  - `whileStatement()`, `forStatement()` will propagate their own `loopStart` to their body statement -> don't need to accept `loopStart`
- Handle `continue`:
  - `continueStatement()` needs `loopStart` to know where to jump to
  - When encounter `continue`, end scope before jump (to discard local variables in current scope)
- To detect not-inside-loop: Let `loopStart = -1` initially.
