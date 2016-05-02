package com.craftinginterpreters.vox;

class ErrorReporter {
  boolean hadError = false;

  void error(Token token, String message) {
    switch (token.type) {
      case ERROR:
        System.err.println("[line " + token.line + "] Error: " + message);
        break;

      case EOF:
        System.err.println("[line " + token.line + "] Error at end: " + message);
        break;

      default:
        System.err.println("[line " + token.line + "] Error on '" +
            token.text + "': " + message);
    }

    hadError = true;
  }
}
