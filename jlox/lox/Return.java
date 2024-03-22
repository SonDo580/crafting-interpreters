package jlox.lox;

/**
 * Custom exception for early return from function
 */
class Return extends RuntimeException {
    final Object value; // return value

    Return(Object value) {
        // don't have a message or a cause
        // disable stack trace generation and suppression
        super(null, null, false, false);

        this.value = value;
    }
}
