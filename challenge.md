# Challenge 28.3

When interpreting an `OP_INVOKE` instruction, the VM has to do two hash table lookups. First, it looks for a field that could shadow a method, and only if that fails does it look for a method. The former check is rarely useful—most fields do not contain functions. But it is necessary because the language says fields and methods are accessed using the same syntax, and fields shadow methods.

That is a language choice that affects the performance of our implementation. Was it the right choice? If Lox were your language, what would you do?

## Answer

- Was it the right choice:
  - pros: easier for user to learn the syntax.
  - cons: lower VM performance because of the double lookup.
- What would I do:
  - use a different syntax for method invocation: `instance:method()`
