package com.craftinginterpreters.lox;

public final class ReplStatus {
    private static ReplStatus shared;
    private boolean isRepl;
    private boolean insideFunBody;
    private Object current_expr;
    private ReplStatus() { }

    // Initializer
    public static ReplStatus getInstance() {
        if (shared == null)
            shared = new ReplStatus();
        return shared;
    }

    public void push(Object expr) {
        current_expr = expr;
        if (!insideFunBody)
            print(expr);
    }
    public void print(Object expr) {
        if (isRepl) System.out.println(expr);
    }

    // Setters
    public void setRepl() { isRepl = true; }
    public void confirmFunBody(boolean inout) { insideFunBody = inout; }



}
