package com.craftinginterpreters.vox;

class ErrorReporter {
  boolean hadError = false;

  void error(int line, String message) {
    report(line, "", message);
  }

  void error(Token token, String message) {
    if (token.type == TokenType.EOF) {
      report(token.line, " at end", message);
    } else {
      report(token.line, " at '" + token.text + "'", message);
    }
  }

  private void report(int line, String location, String message) {
    System.err.println("[line " + line + "] Error" + location + ": " + message);
    hadError = true;
  }
}
