# Crafting Interpreters

- Develop 2 interpreters for the Lox language: `jlox` in Java and `clox` in C
- Book: [Crafting Interpreters](https://craftinginterpreters.com/contents.html)
- Lox samples: [test files](https://github.com/munificent/craftinginterpreters/tree/master/test)

## Extensions

Checkout `challenge...` branches

- ... -> 13: extend JLox
- 14 -> ...: extend CLox

## JLox

### Compile

```bash
bash build_jlox.sh
```

### Run

```bash
# interactive mode
java jlox.lox.Lox

# execute code from file
java jlox.lox.Lox <script_path>
```

### Debug (VSCode)

```json
// launch.json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "java",
      "name": "Debug JLox",
      "request": "launch",
      "mainClass": "jlox.lox.Lox",
      "args": "" // use Lox program path to execute code from file
    }
  ]
}
```

## CLox

### Compile

```bash
cd clox
make clean
make
```

### Run

```bash
# REPL
./clox
# REPL with navigation and history support (without code change)
rlwrap ./clox

# execute code from file
./clox <script_path>
```

### Debug

```bash
gdb clox
```
