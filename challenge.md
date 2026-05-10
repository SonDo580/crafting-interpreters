# Challenge 19.2

When we create the ObjString for each string literal, we copy the characters onto the heap. That way, when the string is later freed, we know it is safe to free the characters too.

This is a simpler approach but wastes some memory, which might be a problem on very constrained devices. Instead, we could keep track of which ObjStrings own their character array and which are “constant strings” that just point back to the original source string or some other non-freeable location. Add support for this.

## Idea

- Add extra field to `ObjString` struct to indicate “constant strings” that point back to the original source string.
- When creating `ObjString`, don't copy the characters.
- When freeing memory, don't free the character array for those objects.
- Note that the constant strings don't have '\0' terminator, so remember to check length.
