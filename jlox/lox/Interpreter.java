package jlox.lox;

class Interpreter implements Expr.Visitor<Object> {
    /**
     * Evaluate and print an expression.
     * Report runtime error.
     */
    void interpret(Expr expression) {
        try {
            Object value = evaluate(expression);
            System.out.println(stringify(value));
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    /* Evaluate an expression */
    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }

    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
    }

    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return evaluate(expr.expression);
    }

    @Override
    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = evaluate(expr.right);

        switch (expr.operator.type) {
            case BANG:
                return !isTruthy(right);
            case MINUS:
                checkNumberOperand(expr.operator, right);
                return -(double) right;
            default:
                break;
        }

        return null;
    }

    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left = evaluate(expr.left);
        Object right = evaluate(expr.right);

        switch (expr.operator.type) {
            // Arithmetic
            case MINUS:
                checkNumberOperand(expr.operator, left, right);
                return (double) left - (double) right;
            case SLASH:
                checkNumberOperand(expr.operator, left, right);
                return (double) left / (double) right;
            case STAR:
                checkNumberOperand(expr.operator, left, right);
                return (double) left * (double) right;

            // Addition | Concatenation
            case PLUS:
                if (areNumbers(left, right)) {
                    return (double) left + (double) right;
                }
                if (areStrings(left, right)) {
                    return (String) left + (String) right;
                }
                throw new RuntimeError(expr.operator, "Operands must be 2 numbers or 2 strings.");

            // Comparison
            case GREATER:
                if (areNumbers(left, right)) {
                    return (double) left > (double) right;
                }
                if (areStrings(left, right)) {
                    return compareString((String) left, (String) right) > 0;
                }
                throw new RuntimeError(expr.operator, "Operands must be 2 numbers or 2 strings.");

            case GREATER_EQUAL:
                if (areNumbers(left, right)) {
                    return (double) left >= (double) right;
                }
                if (areStrings(left, right)) {
                    return compareString((String) left, (String) right) >= 0;
                }
                throw new RuntimeError(expr.operator, "Operands must be 2 numbers or 2 strings.");

            case LESS:
                if (areNumbers(left, right)) {
                    return (double) left < (double) right;
                }
                if (areStrings(left, right)) {
                    return compareString((String) left, (String) right) < 0;
                }
                throw new RuntimeError(expr.operator, "Operands must be 2 numbers or 2 strings.");

            case LESS_EQUAL:
                if (areNumbers(left, right)) {
                    return (double) left <= (double) right;
                }
                if (areStrings(left, right)) {
                    return compareString((String) left, (String) right) <= 0;
                }
                throw new RuntimeError(expr.operator, "Operands must be 2 numbers or 2 strings.");

            // Equality
            case BANG_EQUAL:
                return !isEqual(left, right);
            case EQUAL_EQUAL:
                return isEqual(left, right);

            default:
                break;
        }

        return null;
    }

    /**
     * Check the truthiness of a Lox value.
     * (only 'nil' and 'false' are falsy)
     */
    private boolean isTruthy(Object object) {
        if (object == null) {
            return false;
        }
        if (object instanceof Boolean) {
            return (boolean) object;
        }
        return true;
    }

    /**
     * Check if 2 values are equal.
     * Handle nil/null specially to avoid NullPointerException.
     */
    private boolean isEqual(Object a, Object b) {
        if (a == null && b == null) {
            return true;
        }
        if (a == null) {
            return false;
        }
        return a.equals(b);
    }

    /* Ensures the operand of a unary operation is number */
    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double) {
            return;
        }
        throw new RuntimeError(operator, "Operand must be a number.");
    }

    /* Check if the operands of a binary operation are numbers */
    private boolean areNumbers(Object left, Object right) {
        return left instanceof Double && right instanceof Double;
    }

    /* Check if the operands of a binary operation are strings */
    private boolean areStrings(Object left, Object right) {
        return left instanceof String && right instanceof String;
    }

    /* Ensures the operands of a binary operation are numbers */
    private void checkNumberOperand(Token operator, Object left, Object right) {
        if (areNumbers(left, right)) {
            return;
        }
        throw new RuntimeError(operator, "Operands must be a numbers.");
    }

    /*
     * Return -1 if 'left' string is 'less' than 'right' string.
     * Return 1 if 'left' string is 'greater' than 'right' string.
     * Return 0 if 2 strings are equal.
     */
    private int compareString(String left, String right) {
        int leftLength = left.length();
        int rightLength = right.length();
        int length = leftLength < rightLength ? leftLength : rightLength;

        for (int i = 0; i < length; i++) {
            if (left.charAt(i) < right.charAt(i)) {
                return -1;
            } else if (left.charAt(i) > right.charAt(i)) {
                return 1;
            }
        }

        if (leftLength > length) {
            return 1;
        } else if (rightLength > length) {
            return -1;
        }
        return 0;
    }

    /* String representation of a Lox value */
    private String stringify(Object object) {
        if (object == null) {
            return "nil";
        }

        String text = object.toString();
        if (object instanceof Double) {
            if (text.endsWith(".0")) {
                text = text.substring(0, text.length() - 2);
            }
        }

        return text;
    }
}