package jlox.lox;

import java.util.List;
import java.util.Map;

class LoxClass extends LoxInstance implements LoxCallable {
    final String name;
    final Map<String, LoxFunction> methods;
    final Map<String, LoxFunction> staticMethods;

    LoxClass(String name, Map<String, LoxFunction> methods, Map<String, LoxFunction> staticMethods) {
        super(null);
        this.name = name;
        this.methods = methods;
        this.staticMethods = staticMethods;
        super.setClass(this);
    }

    /* Look up a static method on current class */
    LoxFunction findStaticMethod(String name) {
        if (staticMethods.containsKey(name)) {
            return staticMethods.get(name);
        }

        return null;
    }

    /* Look up a method on current class */
    LoxFunction findMethod(String name) {
        if (methods.containsKey(name)) {
            return methods.get(name);
        }

        return null;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        LoxInstance instance = new LoxInstance(this);

        // Handle user-defined constructor
        LoxFunction initializer = findMethod("init");
        if (initializer != null) {
            // We bind the init() method before calling it,
            // so it has access to 'this' inside its body
            initializer.bind(instance).call(interpreter, arguments);
        }

        return instance;
    }

    @Override
    public int arity() {
        LoxFunction initializer = findMethod("init");
        if (initializer == null) {
            return 0;
        }
        return initializer.arity();
    }

    @Override
    public String toString() {
        return name;
    }
}
