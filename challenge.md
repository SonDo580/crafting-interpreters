# Challenge 17.3

You might be wondering about complex “mixfix” expressions that have more than two operands separated by tokens. C’s conditional or “ternary” operator, `?:`, is a widely known one.

Add support for that operator to the compiler. You don’t have to generate any bytecode, just show how you would hook it up to the parser and handle the operands.

## Note

- Ternary operator is right-associative
  - `a ? b : c ? d : e` <-> `a ? b : (c ? d : e)`
