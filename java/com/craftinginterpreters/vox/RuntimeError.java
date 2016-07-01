//>= Interpreting ASTs
package com.craftinginterpreters.vox;

class RuntimeError extends RuntimeException {
  private final int line;

  RuntimeError(String message, Token token) {
    super(message);
    line = token.line;
  }

  RuntimeError(String message, int line) {
    super(message);
    this.line = line;
  }

  @Override
  public String toString() {
    return getMessage() + "\n[line " + line + "]";
  }
}
