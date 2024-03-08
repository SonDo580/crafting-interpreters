## Challenge 7.1

Allowing comparisons on types other than numbers could be useful. The operators might have a reasonable interpretation for strings. Even comparisons among mixed types, like `3 < "pancake"` could be handy to enable things like ordered collections of heterogeneous types. Or it could simply lead to bugs and confusion.

Would you extend Lox to support comparing other types? If so, which pairs of types do you allow and how do you define their ordering? Justify your choices and compare them to other languages.

## Answer

- I will support comparing 2 strings.
- Start by comparing the first characters of each one. If they're equal, compare the second characters, and so on.
- The character that comes first in the alphabet is 'smaller'.
- A substring will be evaluated as smaller.
