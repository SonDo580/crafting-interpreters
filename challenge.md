# Challenge 20.1

In clox, we happen to only need keys that are strings, so the hash table we built is hardcoded for that key type. If we exposed hash tables to Lox users as a first-class collection, it would be useful to support different kinds of keys.

Add support for keys of the other primitive types: numbers, Booleans, and nil. Later, clox will support user-defined classes. If we want to support keys that are instances of those classes, what kind of complexity does that add?

## Primitive types as keys

- Just use equality check (same type, same value)
- Treat primitive values as their hash values (`0`, `false`, `nil` have the same hash value 0, but that's acceptable)

## Class instances as keys (not implement)

- Use pointer equality check by default -> each instance is a unique key
- Let user define `hash()` method for a class to change that behavior -> compare hash value if `hash()` method exists (must be instances of the same class).
