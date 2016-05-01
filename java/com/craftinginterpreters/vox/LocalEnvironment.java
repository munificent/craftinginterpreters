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
  public Object get(String name, int line) {
    if (this.name.equals(name)) return value;

    return previous.get(name, line);
  }

  // TODO: Move into Environment?
  @Override
  public Object get(Token name) {
    return get(name.text, name.line);
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
  public Environment declare(Token name) {
    return new LocalEnvironment(this, name.text, null);
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
