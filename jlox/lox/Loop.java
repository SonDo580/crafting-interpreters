package jlox.lox;

/**
 * Represent the a loop that may be nested within other loops.
 */
public class Loop {
    final Loop enclosing;

    /*
     * Construct the outer most loop.
     * This instance doesn't represent an actual loop.
     */
    Loop() {
        this.enclosing = null;
    }

    /* Construct a new loop enclosed by another loop */
    Loop(Loop enclosing) {
        this.enclosing = enclosing;
    }
}
