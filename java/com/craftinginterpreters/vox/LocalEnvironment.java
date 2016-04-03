package com.craftinginterpreters.vox;

class LocalEnvironment implements Environment {
  private final Environment previous;
  private final String name;
  private Object value;

  LocalEnvironment(Environment previous, String name, Object value) {
    this.previous = previous;
    this.name = name;
    this.value = value;
  }

  @Override
  public Object get(Token name) {
    if (this.name.equals(name.text)) return value;

    return previous.get(name);
  }

  @Override
  public void set(Token name, Object value) {
    if (this.name.equals(name.text)) {
      this.value = value;
      return;
    }

    previous.set(name, value);
  }

  @Override
  public Environment define(String name, Object value) {
    return new LocalEnvironment(this, name, value);
  }

  @Override
  public Environment enterScope() {
    return this;
  }
}
