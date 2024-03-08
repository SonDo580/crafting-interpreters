## Challenge 7.3

What happens right now if you divide a number by zero? What do you think should happen? Justify your choice. How do other languages you know handle division by zero, and why do they make the choices they do?

Change the implementation in `visitBinaryExpr()` to detect and report a runtime error for this case.

## Answer

- With the current implementation, the result is 'Infinity'
- We should throw a RuntimeError (division by 0)
