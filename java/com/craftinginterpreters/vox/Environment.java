package com.craftinginterpreters.vox;

abstract class Environment {
  abstract Object get(String name, int line);

  public Object get(Token name) {
    return get(name.text, name.line);
  }

  abstract void set(Token name, Object value);

  abstract Environment declare(Token name);
  abstract Environment define(String name, Object value);
  abstract Environment enterScope();
}
