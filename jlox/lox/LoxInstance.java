package jlox.lox;

import java.util.HashMap;
import java.util.Map;

class LoxInstance {
    private LoxClass klass;
    private final Map<String, Object> fields = new HashMap<>();

    LoxInstance(LoxClass klass) {
        this.klass = klass;
    }

    /**
     * Look up property on the instance.
     * Return a field / method, or throw an error if not found.
     */
    Object get(Token name) {
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
        fields.put(name.lexeme, value);
    }

    @Override
    public String toString() {
        return klass.name + " instance";
    }
}
