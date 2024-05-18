package jlox.lox;

import java.util.List;

class LoxFunction implements LoxCallable {
    private final Stmt.Function declaration;

    // The environment at the time the function was defined.
    // Allows the function to access variables from its defining scope.
    private final Environment closure;

    // Whether the function is an initializer or not
    private final boolean isInitializer;

    LoxFunction(Stmt.Function declaration, Environment closure, boolean isInitializer) {
        this.isInitializer = isInitializer;
        this.declaration = declaration;
        this.closure = closure;
    }

    /* Bind the method to a given instance */
    LoxFunction bind(LoxInstance instance) {
        // Create a new environment nestled inside the method’s original closure
        // Declare “this” as a variable and bind it to the given instance
        Environment environment = new Environment(closure);
        environment.define("this", instance);
        return new LoxFunction(declaration, environment, isInitializer);
    }

    @Override
    public int arity() {
        return declaration.params.size();
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        // Create a new environment for the function call
        // with the function's closure as its parent
        Environment environment = new Environment(closure);

        for (int i = 0; i < declaration.params.size(); i++) {
            environment.define(declaration.params.get(i).lexeme, arguments.get(i));
        }

        try {
            interpreter.executeBlock(declaration.body, environment);
        } catch (Return returnValue) {
            // In initializer: empty return => return 'this'
            if (isInitializer) {
                return closure.getAt(0, "this");
            }

            return returnValue.value;
        }

        // Always return 'this' for an initializer
        if (isInitializer) {
            return closure.getAt(0, "this");
        }

        return null;
    }

    @Override
    public String toString() {
        return "<fn " + declaration.name.lexeme + ">";
    }
}
