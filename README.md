## Challenge 4.4:

Add support to Lox’s scanner for C-style /\* ... \*/ block comments. Make sure to handle newlines in them. Consider allowing them to nest. Is adding support for nesting more work than you expected? Why?

## Answer

1. Handling block comments (normally)

- Similar to handling a string literals

2. Handling nested block comments

- This is more complicated. We need to keep track of the current nesting level.
- Keep checking when the nesting level is greater than 0
- Encountering a '/\*' => entering a new level. Encountering a '\*/' => exit the current level
- When we reach the end of the source string and the nesting level is not 0 => the block is unterminated
