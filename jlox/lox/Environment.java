package jlox.lox;

import java.util.HashMap;
import java.util.Map;

/**
 * Represent the environment in which variables are stored.
 */
public class Environment {
    final Environment enclosing;
    private final Map<String, Object> values = new HashMap<>();

    /* Construct the global environment (with no enclosing environment) */
    Environment() {
        enclosing = null;
    }

    /* Construct a new environment enclosed by another environment */
    Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    /* Define or redefine a variable in the current environment */
    void define(String name, Object value) {
        values.put(name, value);
    }

    /*
     * Return the environment that is a certain distance away
     * in the enclosing environment chain
     */
    Environment ancestor(int distance) {
        Environment environment = this;
        for (int i = 0; i < distance; i++) {
            environment = environment.enclosing;
        }

        return environment;
    }

    /* Retrieve value of a variable in ancestor(distance) */
    Object getAt(int distance, String name) {
        return ancestor(distance).values.get(name);
    }

    /* Assign value to a variable in ancestor(distance) */
    void assignAt(int distance, Token name, Object value) {
        ancestor(distance).values.put(name.lexeme, value);
    }

    /*
     * Look up the value of a variable.
     * Recursively search the enclosing environments if needed.
     */
    Object get(Token name) {
        if (values.containsKey(name.lexeme)) {
            return values.get(name.lexeme);
        }

        if (enclosing != null) {
            return enclosing.get(name);
        }

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'");
    }

    /*
     * Assign value to an existing variable.
     * Recursively search the enclosing environments if needed.
     */
    void assign(Token name, Object value) {
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }

        if (enclosing != null) {
            enclosing.assign(name, value);
            return;
        }

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'");
    }
}
