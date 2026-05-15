# Challenge 24.1

Reading and writing the `ip` field is one of the most frequent operations inside the bytecode loop. Right now, we access it through a pointer to the current CallFrame. That requires a pointer indirection which may force the CPU to bypass the cache and hit main memory. That can be a real performance sink.

Ideally, we’d keep the `ip` in a native CPU register. C doesn’t let us require that without dropping into inline assembly, but we can structure the code to encourage the compiler to make that optimization. If we store the `ip` directly in a C local variable and mark it `register`, there’s a good chance the C compiler will accede to our polite request.

This does mean we need to be careful to load and store the local `ip` back into the correct `CallFrame` when starting and ending function calls. Implement this optimization. Write a couple of benchmarks and see how it affects the performance. Do you think the extra code complexity is worth it?

## Idea

- Don't store `ip` field in CallFrame, store it in a local variable and mark it `register`.
- When calling a function:
  - push current `ip` (of caller) onto the stack, right after the arguments
  - reset `ip` to 0 when executing the callee.
- When returning
  - save the stored caller's `ip` before discarding callee's frame.
  - go back to caller's frame and load the saved `ip`.
