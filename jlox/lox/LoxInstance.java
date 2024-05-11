package jlox.lox;

import java.util.HashMap;
import java.util.Map;

class LoxInstance {
    private LoxClass klass;
    private final Map<String, Object> fields = new HashMap<>();

    /* klass can be null.
     * For LoxClass instance initialization.
     */
    LoxInstance(LoxClass klass) {
        this.klass = klass;
    }

    /* For specifying class after initialization. */
    void setClass(LoxClass klass) {
        this.klass = klass;
    }

    /* Check if the instance is the class itself.
     * For static methods feature.
     */
    private boolean isClass() {
        return this == this.klass;
    }

    /**
     * Look up property on the instance.
     * Return a field / method, or throw an error if not found.
     */
    Object get(Token name) {
        // Look for static method if the callee is the class itself
        if (isClass()) {
            return klass.findStaticMethod(name.lexeme);
        }

        // Look for field
        if (fields.containsKey(name.lexeme)) {
            return fields.get(name.lexeme);
        }

        // If field is not found, look for method on the instance's class
        LoxFunction method = klass.findMethod(name.lexeme);
        if (method != null) {
            // Bind the method to current class instance
            return method.bind(this);
        }

        throw new RuntimeError(name, "Undefined property '" + name.lexeme + "'.");
    }

    /* Set a property on the instance */
    void set(Token name, Object value) {
        // Don't support static fields
        if (isClass()) {
            throw new RuntimeError(name, "Cannot set property of class.");
        }

        fields.put(name.lexeme, value);
    }

    @Override
    public String toString() {
        if (isClass()) {
            return klass.name;
        }
        return klass.name + " instance";
    }
}
