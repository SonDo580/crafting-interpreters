# Challenge 27.3

Conversely, Lox offers no way to remove a field from an instance. You can set a field’s value to `nil`, but the entry in the hash table is still there. How do other languages handle this? Choose and implement a strategy for Lox.

## Answer

- Javascript (allow): `delete user.name`
- Python (allow): `del user.name` OR `delattr(user, "name")`

## Implementation

- Use `delete instance.field` syntax.
- Handle instruction `OP_DELETE_PROPERTY`: remove entry from `fields` table if exists, raise runtime error if not found.
- Parse delete statement: emit code as getter, then replace the latest `OP_GET_PROPERTY` instruction with `OP_DELETE_PROPERTY`.
