package com.craftinginterpreters.vox;

class LocalEnvironment extends Environment {
  private final Environment previous;
  private final String name;
  private Object value;

  LocalEnvironment(Environment previous, String name, Object value) {
    this.previous = previous;
    this.name = name;
    this.value = value;
  }

  @Override
  Object get(String name, int line) {
    if (this.name.equals(name)) return value;

    return previous.get(name, line);
  }

  @Override
  void set(Token name, Object value) {
    if (this.name.equals(name.text)) {
      this.value = value;
      return;
    }

    previous.set(name, value);
  }

  @Override
  Environment declare(Token name) {
    return new LocalEnvironment(this, name.text, null);
  }

  @Override
  Environment define(String name, Object value) {
    return new LocalEnvironment(this, name, value);
  }

  @Override
  Environment enterScope() {
    return this;
  }
}
