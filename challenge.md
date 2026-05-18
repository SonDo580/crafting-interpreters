# Challenge 25.1

Wrapping every ObjFunction in an ObjClosure introduces a level of indirection that has a performance cost. That cost isn’t necessary for functions that do not close over any variables, but it does let the runtime treat all calls uniformly.

Change clox to only wrap functions in ObjClosures that need upvalues. How does the code complexity and performance compare to always wrapping functions? Take care to benchmark programs that do and do not use closures. How should you weight the importance of each benchmark? If one gets slower and one faster, how do you decide what trade-off to make to choose an implementation strategy?

## Idea

- Basic:
  - Compiler: in function(), emit OP_CONSTANT if `upvalueCount` is 0 (Value is function); otherwise emit OP_CLOSURE (Value is closure).
  - VM: Check the type and cast to the correct object on related operations.
- "Breaking" changes:
  - intermediate functions should not add the upvalue when an inner function closed over a variable in an outer function.
    -> we cannot use `index` as upvalue index of enclosing function if that enclosing function don't explicitly reference the variable.
    -> need to know the exact function to hop to, and reference its local variable in that case.
- Implement breaking changes:
  - each item following OP_CLOSURE needs an extra field `hop`, which encodes how many frames to move out from current frame (where the closed variable lives);
  - remove `isLocal` and keep `index` (can only be local variable in that outer function; we could mix but I chose not to do so).
  - Compiler:
    - don't add upvalues to intermediate functions.
    - calculate number of hops based on Compiler linked-list.
  - VM:
    - hop to the exact frame to extract local variable.
