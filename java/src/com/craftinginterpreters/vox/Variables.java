package com.craftinginterpreters.vox;

class Variables {
  private final Variables previous;
  private final String name;
  private Object value;

  Variables(Variables previous, String name, Object value) {
    this.previous = previous;
    this.name = name;
    this.value = value;
  }

  Variables define(String name, Object value) {
    // TODO: Error if already defined.
    return new Variables(this, name, value);
  }

  Object lookUp(String name) {
    if (this.name.equals(name)) return value;

    if (previous == null) {
      throw new RuntimeError("Variable '" + name + "' is not defined.");
    }

    return previous.lookUp(name);
  }

  void assign(String name, Object value) {
    if (this.name.equals(name)) {
      this.value = value;
    } else {
      previous.assign(name, value);
    }
  }

}
