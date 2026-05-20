# Challenge 28.1

The hash table lookup to find a class’s `init()` method is constant time, but still fairly slow. Implement something faster. Write a benchmark and measure the performance difference.

## Idea

- Store `init()` method in a **separate field**, instead of putting it in the same hash table for methods.
- `init()` can only be invoked internally by the VM when creating object.
