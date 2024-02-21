package jlox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static jlox.TokenType.*;

/**
 * The Scanner is responsible for converting Lox source code
 * into a list of Tokens that the Parser can work with
 */
class Scanner {
    private final String source;
    private final List<Token> tokens = new ArrayList<>();
    private int start = 0; // where the current token begins
    private int current = 0; // current position in source code
    private int line = 1; // current line number

    // A map to identify keywords and their corresponding token types
    private static final Map<String, TokenType> keywords;
    static {
        keywords = new HashMap<>();
        keywords.put("and", AND);
        keywords.put("class", CLASS);
        keywords.put("else", ELSE);
        keywords.put("false", FALSE);
        keywords.put("for", FOR);
        keywords.put("fun", FUN);
        keywords.put("if", IF);
        keywords.put("nil", NIL);
        keywords.put("or", OR);
        keywords.put("print", PRINT);
        keywords.put("return", RETURN);
        keywords.put("super", SUPER);
        keywords.put("this", THIS);
        keywords.put("true", TRUE);
        keywords.put("var", VAR);
        keywords.put("while", WHILE);
    }

    /* Initialize a Scanner object with the provided source code */
    Scanner(String source) {
        this.source = source;
    }

    /* Scan the source code and convert it into a list of tokens */
    List<Token> scanTokens() {
        while (!isAtEnd()) {
            start = current;
            scanToken();
        }
        // Add an EOF token to signify the end of the file
        tokens.add(new Token(EOF, "", null, line));
        return tokens;
    }

    /* Determines whether the end of the file has been reached */
    private boolean isAtEnd() {
        return current >= source.length();
    }

    /* Scans and categorizes the next token from source code */
    private void scanToken() {
        char c = advance();
        switch (c) {
            // Single-character tokens
            case '(':
                addToken(LEFT_PAREN);
                break;
            case ')':
                addToken(RIGHT_PAREN);
                break;
            case '{':
                addToken(LEFT_BRACE);
                break;
            case '}':
                addToken(RIGHT_BRACE);
                break;
            case ',':
                addToken(COMMA);
                break;
            case '.':
                addToken(DOT);
                break;
            case '-':
                addToken(MINUS);
                break;
            case '+':
                addToken(PLUS);
                break;
            case ';':
                addToken(SEMICOLON);
                break;
            case '*':
                addToken(STAR);
                break;

            // 1 or 2-character tokens
            // Check the next token and consume it upon a match
            case '!':
                addToken(match('=') ? BANG_EQUAL : BANG);
                break;
            case '=':
                addToken(match('=') ? EQUAL_EQUAL : EQUAL);
                break;
            case '<':
                addToken(match('=') ? LESS_EQUAL : LESS);
                break;
            case '>':
                addToken(match('=') ? GREATER_EQUAL : GREATER);
                break;

            case '/':
                if (match('/')) {
                    // Inline comment
                    // keep consuming characters until the end of the line
                    while (peek() != '\n' && !isAtEnd()) {
                        advance();
                    }
                } else if (match('*')) {
                    // Block comment
                    blockComment();
                } else {
                    // Division operator
                    addToken(SLASH);
                }
                break;

            case ' ':
            case '\r':
            case '\t':
                // Whitespaces are ignored
                break;

            case '\n':
                line++;
                break;

            // Handle string literals
            case '"':
                string();
                break;

            default:
                if (isDigit(c)) {
                    // Handle number literals
                    number();
                } else if (isAlpha(c)) {
                    // Handle reserved keywords and identifiers
                    keyword_identifier();
                } else {
                    Lox.error(line, "Unexpected character.");
                }
                break;
        }
    }

    /*
     * Consumes and returns the current character.
     * Advances to the next character.
     */
    private char advance() {
        return source.charAt(current++);
    }

    /* Returns the current character without consuming it */
    private char peek() {
        if (isAtEnd()) {
            return '\0';
        }
        return source.charAt(current);
    }

    /* Returns the next character without consuming the current character */
    private char peekNext() {
        if (current + 1 >= source.length()) {
            return '\0';
        }
        return source.charAt(current + 1);
    }

    /*
     * Checks if the current character matches the expected character.
     * Consumes it if it does.
     */
    private boolean match(char expected) {
        if (isAtEnd() || source.charAt(current) != expected) {
            return false;
        }
        current++;
        return true;
    }

    /* Adds a token of the specified type without a literal value */
    private void addToken(TokenType type) {
        addToken(type, null);
    }

    /* Adds a token of the specified type with a literal value */
    private void addToken(TokenType type, Object literal) {
        String lexeme = source.substring(start, current);
        tokens.add(new Token(type, lexeme, literal, line));
    }

    /* Handle block comments */
    private void blockComment() {
        // Keep consuming character until a star (*) is found
        while (peek() != '*' && !isAtEnd()) {
            if (peek() == '\n') {
                line++;
            }
            advance();
        }

        if (isAtEnd()) {
            Lox.error(line, "Unterminated block comment.");
        }

        // Comsume the star
        advance();

        // Check for closing slash
        if (peek() != '/') {
            blockComment();
        } else {
            // Consume closing slash
            advance();
        }
    }

    /* Handle string literals */
    private void string() {
        // Keep consuming characters until the closing quote is found
        while (peek() != '"' && !isAtEnd()) {
            // allow multi-line strings
            if (peek() == '\n') {
                line++;
            }
            advance();
        }

        if (isAtEnd()) {
            Lox.error(line, "Unterminated string.");
            return;
        }

        // Consume the closing quote
        advance();

        // Get the string value without surrounding quotes
        String value = source.substring(start + 1, current - 1);
        addToken(STRING, value);
    }

    /* Handle number literals */
    private void number() {
        while (isDigit(peek())) {
            advance();
        }

        // Look for fractional part
        if (peek() == '.' && isDigit(peekNext())) {
            // Consume the dot
            advance();

            while (isDigit(peek())) {
                advance();
            }
        }

        // Convert the lexeme to numeric value
        Double value = Double.parseDouble(source.substring(start, current));
        addToken(NUMBER, value);
    }

    /* Handle reserved keywords and identifiers */
    private void keyword_identifier() {
        while (isAlphaNumeric(peek())) {
            advance();
        }
        // Check if the lexeme is a keyword or an identifier
        String text = source.substring(start, current);
        TokenType type = keywords.get(text);
        if (type == null) {
            type = IDENTIFIER;
        }
        addToken(IDENTIFIER);
    }

    private boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    private boolean isAlpha(char c) {
        return (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                c == '_';
    }

    private boolean isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }
}
