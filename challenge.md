# Challenge 15.4

To interpret `OP_NEGATE`, we pop the operand, negate the value, and then push the result. That’s a simple implementation, but it increments and decrements `stackTop` unnecessarily, since the stack ends up the same height in the end. It might be faster to simply negate the value in place on the stack and leave `stackTop` alone. Try that and see if you can measure a performance difference.

Are there other instructions where you can do a similar optimization?
