# Crafting Interpreters

- Develop 2 interpreters for the Lox language: `jlox` in Java and `clox` in C
- Book: [Crafting Interpreters](https://craftinginterpreters.com/contents.html)
- Lox samples: [test files](https://github.com/munificent/craftinginterpreters/tree/master/test)

## Note

- Extend the base interpreter: checkout `challenge...` branches
- Exit code: use the convention defined in the UNIX [sysexits.h](https://man.freebsd.org/cgi/man.cgi?query=sysexits&apropos=0&sektion=0&manpath=FreeBSD+4.3-RELEASE&format=html) header

# JLox

## Compile

```bash
bash build_jlox.sh
```

## Run

```bash
# interactive mode
java jlox.lox.Lox

# execute code from file
java jlox.lox.Lox <script_path>
```

## Debug (VSCode)

- `launch.json`

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "java",
      "name": "Debug JLox",
      "request": "launch",
      "mainClass": "jlox.lox.Lox",
      "args": "", // use Lox program path to execute code from file
      "preLaunchTask": "build-jlox" // comment out if don't need rebuild
    }
  ]
}
```

- `tasks.json`

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "build-jlox",
      "type": "shell",
      "command": "bash build_jlox.sh",
      "group": {
        "kind": "build"
      },
      "problemMatcher": []
    }
  ]
}
```
