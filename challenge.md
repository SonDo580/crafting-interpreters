# Challenge 27.2

Fields are accessed at runtime by their string name. But that name must always appear directly in the source code as an identifier token. A user program cannot imperatively build a string value and then use that as the name of a field. Do you think they should be able to? Devise a language feature that enables that and implement it.

## Answer

- Yes, we should support that
- How other languages do it:
  - Javascript uses bracket notation: `user["user" + "name"]`
  - Python uses `getattr()`: `getattr(user, "user" + "name")`

## Implementation

- Take the JS approach - use bracket notation.
- The precedence of `[` should be the same as `.` (`PREC_CALL`)
- Evaluate the expression inside the bracket, require it to be a string.
- Need 2 extra instructions (`OP_GET_DYNAMIC_PROPERTY` and `OP_SET_DYNAMIC_PROPERTY`) that use field Value on stack instead of reading from the chunk's constants table.
