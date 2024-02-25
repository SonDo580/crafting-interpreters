/** Lox's expression grammar (low-to-high precedence)
 * expression     → comma
 * comma          → equality ( (",") equality )* ;
 * equality       → comparison ( ( "!=" | "==" ) comparison )* ;
 * comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
 * term           → factor ( ( "-" | "+" ) factor )* ;
 * factor         → unary ( ( "/" | "*" ) unary )* ;
 * unary          → ( "!" | "-" ) unary
                    | primary ;
 * primary        → NUMBER | STRING | "true" | "false" | "nil"
                    | "(" expression ")" ;
 */

package jlox.lox;

import java.util.List;

import static jlox.lox.TokenType.*;

/**
 * Parse different components of Lox's grammar.
 * Use recursive-descent (top-down parser)
 */
class Parser {
    /* A custom runtime exception to indicate parsing error */
    private static class ParseError extends RuntimeException {
    }

    private final List<Token> tokens;
    private int current = 0; // current token index

    /* The Parser takes a list of tokens as input */
    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    /* Start the parsing process. Return 'null' if there's a syntax error */
    Expr parse() {
        try {
            return expression();
        } catch (ParseError error) {
            return null;
        }
    }

    private Expr expression() {
        return comma();
    }

    private Expr comma() {
        Expr expr = equality();

        while (match(COMMA)) {
            Token operator = previous();
            Expr right = equality();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr equality() {
        Expr expr = comparison();

        while (match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr comparison() {
        Expr expr = term();

        while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
            Token operator = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr term() {
        Expr expr = factor();

        while (match(PLUS, MINUS)) {
            Token operator = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr factor() {
        Expr expr = unary();

        while (match(SLASH, STAR)) {
            Token operator = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr unary() {
        if (match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary();
            return new Expr.Unary(operator, right);
        }

        return primary();
    }

    private Expr primary() {
        if (match(FALSE)) {
            return new Expr.Literal(false);
        }
        if (match(TRUE)) {
            return new Expr.Literal(true);
        }
        if (match(NIL)) {
            return new Expr.Literal(null);
        }

        if (match(NUMBER, STRING)) {
            return new Expr.Literal(previous().literal);
        }

        if (match(LEFT_PAREN)) {
            Expr expr = expression();
            consume(RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }

        throw error(peek(), "Expect expression.");
    }

    /*
     * Check if current token matches any of the given types.
     * Consume the token if true.
     */
    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }

        return false;
    }

    /*
     * Consume the token if it matches the expected type.
     * Otherwise, report an error.
     */
    private Token consume(TokenType type, String message) {
        if (check(type)) {
            return advance();
        }

        // Stop parsing if there's an error
        throw error(peek(), message);
    }

    /* Check if current token matches the expected type */
    private boolean check(TokenType type) {
        if (isAtEnd()) {
            return false;
        }
        return peek().type == type;
    }

    /* Consume and return current token */
    private Token advance() {
        if (!isAtEnd()) {
            current++;
        }
        return previous();
    }

    /* Check for EOF (end of file) token */
    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    /* Return current token */
    private Token peek() {
        return tokens.get(current);
    }

    /* Return previous token */
    private Token previous() {
        return tokens.get(current - 1);
    }

    /* Generate a ParseError */
    private ParseError error(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }

    /**
     * Skip tokens until the start of the next statement.
     * Used to recover from error and continue parsing.
     */
    private void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == SEMICOLON) {
                return;
            }

            switch (peek().type) {
                case CLASS:
                case FUN:
                case VAR:
                case FOR:
                case IF:
                case WHILE:
                case PRINT:
                case RETURN:
                    return;
                default:
                    break;
            }

            advance();
        }
    }
}
