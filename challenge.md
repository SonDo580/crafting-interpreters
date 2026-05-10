# Challenge 19.1

Each string requires two separate dynamic allocations - one for the `ObjString` and a second for the character array. Accessing the characters from a value requires two pointer indirections, which can be bad for performance. A more efficient solution relies on a technique called [flexible array members](https://en.wikipedia.org/wiki/Flexible_array_member). Use that to store the `ObjString` and its character array in a single contiguous allocation.
