## Challenge 6.2

Add support for the C-style conditional or “ternary” operator `?:`. What precedence level is allowed between the `?` and `:`? Is the whole operator left-associative or right-associative?

## Answer:

1. **My implementation**

The ternary operator is right-associative.

- expression → ternary
- ternary → equality ( "?" expression ":" expression )?
- equality → comparison ( ( "!=" | "==" ) comparison )\*
- ...

2. **Example**

```
a ? b : c ? d : e
```

is evaluated as

```
a ? b : (c ? d : e)
```
