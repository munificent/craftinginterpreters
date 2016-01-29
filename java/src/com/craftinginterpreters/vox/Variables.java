package com.craftinginterpreters.vox;

class Variables {
  private final Variables previous;
  private final int depth; // TODO: Remove?
  private final String name;
  private Object value;

  Variables(Variables previous, int depth, String name, Object value) {
    this.previous = previous;
    this.depth = depth;
    this.name = name;
    this.value = value;
  }

  Variables define(int depth, String name, Object value) {
    // TODO: Error if already defined.
    return new Variables(this, depth, name, value);
  }

  Object lookUp(String name) {
    if (this.name.equals(name)) return value;
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
