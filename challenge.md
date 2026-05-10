# Challenge 18.1

We could reduce our binary operators even further than we did here. Which other instructions can you eliminate, and how would the compiler cope with their absence?

## Answer

```
a - b <-> a + (-b)
-> replace OP_SUBTRACT with OP_ADD and OP_NEGATE
```
