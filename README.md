# Challenge 5.3

In reverse Polish notation (RPN), the operands to an arithmetic operator are both placed before the operator, so `1 + 2` becomes `1 2 +`. Evaluation proceeds from left to right. Numbers are pushed onto an implicit stack. An arithmetic operator pops the top two numbers, performs the operation, and pushes the result. Thus, this:

```
(1 + 2) * (4 - 3)
```

in RPN becomes:

```
1 2 + 4 3 - *
```

Define a visitor class for our syntax tree classes that takes an expression, converts it to RPN, and returns the resulting string.
