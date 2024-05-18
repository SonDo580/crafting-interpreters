package jlox.lox;

import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.Set;

class LoxClass implements LoxCallable {
    final String name;
    final List<LoxClass> superClasses;
    final Map<String, LoxFunction> methods;

    LoxClass(String name, List<LoxClass> superClasses, Map<String, LoxFunction> methods) {
        this.superClasses = superClasses;
        this.name = name;
        this.methods = methods;
    }

    /*
     * Look up a method on current class.
     * Look up on the super classes if not found.
     */
    LoxFunction findMethod(String name) {
        if (methods.containsKey(name)) {
            return methods.get(name);
        }

        if (!superClasses.isEmpty()) {
            return LoxClass.findMethod(superClasses, name);
        }

        return null;
    }

    /*
     * Look up a method on current class.
     * Don't look up on the super classes.
     */
    LoxFunction findSelfMethod(String name) {
        if (methods.containsKey(name)) {
            return methods.get(name);
        }

        return null;
    }

    /*
     * Look up method on a list of Lox classes.
     * Check the classes in the inheritance order.
     * Check all classes in each level like breadth-first search.
     * Use a Set to store the names of the classes that have been checked.
     */
    static LoxFunction findMethod(List<LoxClass> klasses, String name) {
        Set<String> checked = new HashSet<>();
        Queue<LoxClass> queue = new LinkedList<>(klasses);

        while (!queue.isEmpty()) {
            LoxClass klass = queue.poll();

            // Skip if the class has been checked
            if (checked.contains(klass.name)) {
                continue;
            }

            // Look up method just in the current class
            LoxFunction method = klass.findSelfMethod(name);
            if (method != null) {
                return method;
            }

            // Mark the class as checked
            checked.add(klass.name);

            // Add super classes of current class to the queue
            if (!klass.superClasses.isEmpty()) {
                queue.addAll(klass.superClasses);
            }
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
