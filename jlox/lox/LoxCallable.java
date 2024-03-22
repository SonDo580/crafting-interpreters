package jlox.lox;

import java.util.List;

interface LoxCallable {
    // Number of arguments
    int arity();

    Object call(Interpreter interpreter, List<Object> arguments);
}
