# Challenge 22.3

Many languages make a distinction between variables that can be reassigned and those that can’t. In Java, the `final` modifier prevents you from assigning to a variable. In JavaScript, a variable declared with `let` can be assigned, but one declared using `const` can’t. Swift treats `let` as single-assignment and uses `var` for assignable variables. Scala and Kotlin use `val` and `var`.

Pick a keyword for a single-assignment variable form to add to Lox. Justify your choice, then implement it. An attempt to assign to a variable declared using your new keyword should cause a compile error.

## Idea

- Let `const` declare a single-assignment variable.
- To support that:
  - Local variables: add an extra field `isConst` to `Local` struct.
  - Global variables: ?
- Raise compile error if a `const` variable is set.