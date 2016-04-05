package com.craftinginterpreters.vox;

class ErrorReporter {
  boolean hadError = false;

  void error(Token token, String message) {
    System.err.println("[line " + token.line +
        "] Error on '" +
        token.text + "': " + message);
    hadError = true;
  }
}
