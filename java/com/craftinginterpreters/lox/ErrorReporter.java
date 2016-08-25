//>= Framework
package com.craftinginterpreters.lox;

class ErrorReporter {
  boolean hadError = false;
  boolean hadRuntimeError = false;

  void reset() {
    hadError = false;
    hadRuntimeError = false;
  }

  void error(int line, String message) {
    report(line, "", message);
  }

  // TODO: Stack trace.
  void runtimeError(int line, String message) {
    System.err.println(message + "\n[line " + line + "]");
    hadRuntimeError = true;
  }

//>= Scanning
  void error(Token token, String message) {
    if (token.type == TokenType.EOF) {
      report(token.line, " at end", message);
    } else {
      report(token.line, " at '" + token.text + "'", message);
    }
  }

//>= Framework
  private void report(int line, String location, String message) {
    System.err.println("[line " + line + "] Error" + location +
        ": " + message);
    hadError = true;
  }
}
