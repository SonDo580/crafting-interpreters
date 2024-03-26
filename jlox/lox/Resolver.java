package jlox.lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

/**
 * Walk through the AST to bind variable and function declarations
 * to their respective scopes before interpretation.
 */
class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
    private final Interpreter interpreter;

    // The Boolean value indicates whether the variable
    // has been initialized within the current scope.
    private final Stack<Map<String, Boolean>> scopes = new Stack<>();

    // Currently inside a function or not
    private FunctionType currentFunction = FunctionType.NONE;

    private enum FunctionType {
        NONE,
        FUNCTION
    }

    Resolver(Interpreter interpreter) {
        this.interpreter = interpreter;
    }

    /* Resolve list of statements */
    void resolve(List<Stmt> statements) {
        for (Stmt statement : statements) {
            resolve(statement);
        }
    }

    /* Resolve a single statement */
    private void resolve(Stmt stmt) {
        stmt.accept(this);
    }

    /* Resolve a single expression */
    private void resolve(Expr expr) {
        expr.accept(this);
    }

    /* Resolve a function */
    private void resolveFunction(Stmt.Function stmt, FunctionType type) {
        // Save current function context before resolving the new function
        FunctionType enclosingFunction = currentFunction;
        currentFunction = type;

        beginScope();
        for (Token param : stmt.params) {
            declare(param);

            // Mark the parameters as initialized immediately.
            // So the parameters are usable inside the function body.
            define(param);
        }

        resolve(stmt.body);
        endScope();

        // Restore the previous function context
        currentFunction = enclosingFunction;
    }

    /* Enter a new scope */
    private void beginScope() {
        scopes.push(new HashMap<String, Boolean>());
    }

    /* Exit the current scope */
    private void endScope() {
        scopes.pop();
    }

    /* Declare a variable in the current scope */
    private void declare(Token name) {
        if (scopes.isEmpty()) {
            return;
        }

        Map<String, Boolean> scope = scopes.peek();

        // Don't allow re-declaration
        if (scope.containsKey(name.lexeme)) {
            Lox.error(name, "Already a variable with this name in this scope.");
        }

        // Add the variable to current scope, mark as not initialized
        scope.put(name.lexeme, false);
    }

    /* Define a variable in the current scope */
    private void define(Token name) {
        if (scopes.isEmpty()) {
            return;
        }

        // Mark the variable as initialized
        scopes.peek().put(name.lexeme, true);
    }

    /**
     * Traverse the stack of scopes (from innermost to outermost).
     * Checks each scope to see if it contains the variable being resolved.
     * Save the number of "hops" between the current scope and the
     * scope where the variable is defined to the interpreter.
     */
    private void resolveLocal(Expr expr, Token name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            if (scopes.get(i).containsKey(name.lexeme)) {
                interpreter.resolve(expr, scopes.size() - 1 - i);
                return;
            }
        }
    }

    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        beginScope();
        resolve(stmt.statements);
        endScope();
        return null;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        declare(stmt.name);

        // Mark the function as initialized immediately.
        // This lets a function refer to itself inside its own body (recursion).
        define(stmt.name);

        resolveFunction(stmt, FunctionType.FUNCTION);
        return null;
    }

    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        resolve(stmt.condition);
        resolve(stmt.thenBranch);
        if (stmt.elseBranch != null) {
            resolve(stmt.elseBranch);
        }
        return null;
    }

    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        if (currentFunction == FunctionType.NONE) {
            Lox.error(stmt.keyword, "Can't return from top-level code.");
        }

        if (stmt.value != null) {
            resolve(stmt.value);
        }

        return null;
    }

    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        // Resolve the initializer before defining the variable.
        // To handle variable being accessed in its own initializer.
        declare(stmt.name);
        if (stmt.initializer != null) {
            resolve(stmt.initializer);
        }
        define(stmt.name);
        return null;
    }

    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        resolve(stmt.condition);
        resolve(stmt.body);
        return null;
    }

    @Override
    public Void visitVariableExpr(Expr.Variable expr) {
        if (!scopes.isEmpty() && scopes.peek().get(expr.name.lexeme) == Boolean.FALSE) {
            Lox.error(expr.name, "Can't read local variable in its own initializer.");
        }

        resolveLocal(expr, expr.name);
        return null;
    }

    @Override
    public Void visitAssignExpr(Expr.Assign expr) {
        resolve(expr.value);
        resolveLocal(expr, expr.name);
        return null;
    }

    @Override
    public Void visitBinaryExpr(Expr.Binary expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitCallExpr(Expr.Call expr) {
        resolve(expr.callee);

        for (Expr argument : expr.arguments) {
            resolve(argument);
        }

        return null;
    }

    @Override
    public Void visitGroupingExpr(Expr.Grouping expr) {
        resolve(expr.expression);
        return null;
    }

    @Override
    public Void visitLiteralExpr(Expr.Literal expr) {
        return null;
    }

    @Override
    public Void visitLogicalExpr(Expr.Logical expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitUnaryExpr(Expr.Unary expr) {
        resolve(expr.right);
        return null;
    }
}
