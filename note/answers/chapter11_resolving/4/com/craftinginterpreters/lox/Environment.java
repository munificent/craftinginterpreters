package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.List;

class Environment {
  final Environment enclosing;
  private final List<Object> values = new ArrayList<>();

  Environment() {
    enclosing = null;
  }

  Environment(Environment enclosing) {
    this.enclosing = enclosing;
  }

  void define(Object value) {
    values.add(value);
  }

  Object getAt(int distance, int slot) {
    Environment environment = this;
    for (int i = 0; i < distance; i++) {
      environment = environment.enclosing;
    }

    return environment.values.get(slot);
  }

  void assignAt(int distance, int slot, Object value) {
    Environment environment = this;
    for (int i = 0; i < distance; i++) {
      environment = environment.enclosing;
    }

    environment.values.set(slot, value);
  }
  @Override
  public String toString() {
    String result = values.toString();
    if (enclosing != null) {
      result += " -> " + enclosing.toString();
    }

    return result;
  }
}
