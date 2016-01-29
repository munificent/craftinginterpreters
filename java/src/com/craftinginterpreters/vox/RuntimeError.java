package com.craftinginterpreters.vox;

class RuntimeError extends RuntimeException {
  RuntimeError(String message) {
    super(message);
  }
}
