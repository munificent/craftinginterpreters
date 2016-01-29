package com.craftinginterpreters.vox;

class ErrorReporter {
  boolean hadError = false;

  void error(int line, String message) {
    System.err.println("[line " + line + "] Error: " + message);
    hadError = true;
  }
}
