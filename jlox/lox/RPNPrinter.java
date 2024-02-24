package jlox.lox;

/**
 * RPN: Reverse Polish Notation
 * Takes an expression, converts it to RPN, and returns the resulting string
 */
public class RPNPrinter implements Expr.Visitor<String> {
    /* Example usage */
    public static void main(String[] args) {
        RPNPrinter printer = new RPNPrinter();

        // expression: (1 + 2) * (4 - 3)
        Expr expression = new Expr.Binary(
                new Expr.Grouping(new Expr.Binary(
                        new Expr.Literal(1),
                        new Token(TokenType.PLUS, "+", null, 1),
                        new Expr.Literal(2))),
                new Token(TokenType.STAR, "*", null, 1),
                new Expr.Grouping(new Expr.Binary(
                        new Expr.Literal(4),
                        new Token(TokenType.MINUS, "-", null, 1),
                        new Expr.Literal(3))));

        // expected output: 1 2 + 4 3 - *
        System.out.println(printer.print(expression));
    }

    /* Start the printing process */
    String print(Expr expr) {
        return expr.accept(this);
    }

    @Override
    public String visitBinaryExpr(Expr.Binary expr) {
        return print(expr.left) + print(expr.right) + expr.operator.lexeme + " ";
    }

    @Override
    public String visitGroupingExpr(Expr.Grouping expr) {
        return print(expr.expression);
    }

    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null) {
            return "nil ";
        }
        return expr.value.toString() + " ";
    }

    @Override
    public String visitUnaryExpr(Expr.Unary expr) {
        return print(expr.right) + expr.operator.lexeme + " ";
    }
}
