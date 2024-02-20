package jlox;

enum TokenType {
    LEFT_PAREN, // (
    RIGHT_PAREN, // )
    LEFT_BRACE, // {
    RIGHT_BRACE, // }
    COMMA, // ,
    SEMICOLON, // ;
    DOT, // .
    PLUS, // +
    MINUS, // -
    STAR, // *
    SLASH, // /

    BANG, // !
    BANG_EQUAL, // !=
    EQUAL, // =
    EQUAL_EQUAL, // ==
    GREATER, // >
    GREATER_EQUAL, // >=
    LESS, // <
    LESS_EQUAL, // <=

    // Literals
    IDENTIFIER,
    STRING,
    NUMBER,

    // Keywords
    CLASS,
    SUPER,
    THIS,
    FUN,
    RETURN,
    AND,
    OR,
    TRUE,
    FALSE,
    NIL,
    VAR,
    IF,
    ELSE,
    FOR,
    WHILE,
    PRINT,

    EOF
}
