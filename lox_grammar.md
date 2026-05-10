## Lox's grammar

```
program        → statement* EOF
declaration    → classDecl
                 | funDecl
                 | varDecl
                 | statement
classDecl      → "class" IDENTIFIER ( "<" IDENTIFIER )?
                  "{" function* "}"
funDecl        → "fun" function
function       → IDENTIFIER "(" parameters? ")" block
parameters     → IDENTIFIER ( "," IDENTIFIER )*
varDecl        → "var" IDENTIFIER ( "=" expression )? ";"
statement      → exprStmt
                 | forStmt
                 | ifStmt
                 | printStmt
                 | returnStmt
                 | whileStmt
                 | block
returnStmt     → "return" expression? ";"
forStmt        → "for" "(" ( varDecl | exprStmt | ";" )
                 expression? ";"
                 expression? ")" statement
whileStmt      → "while" "(" expression ")" statement
ifStmt         → "if" "(" expression ")" statement
                 ( "else" statement )?
block          → "{" declaration* "}"
exprStmt       → expression ";"
printStmt      → "print" expression ";"
```

## Lox's expression grammar (increasing precedence)

```
expression     → ternary
ternary        → assignment ("?" expression ":" expression)?
assignment     → ( call ".")? IDENTIFIER "=" assignment
                 | logic_or
logic_or       → logic_and ( "or" logic_and )*
logic_and      → equality ( "and" equality )*
equality       → comparison ( ( "!=" | "==" ) comparison )*
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )*
term           → factor ( ( "-" | "+" ) factor )*
factor         → unary ( ( "/" | "*" ) unary )*
unary          → ( "!" | "-" ) unary
                 | call
call           → primary ( "(" arguments? ")" | "." IDENTIFIER)*
arguments      → expression ( "," expression )*
primary        → "true" | "false" | "nil"
                 | NUMBER | STRING
                 | "(" expression ")"
                 | IDENTIFIER
                 | "super" "." IDENTIFIER
```
