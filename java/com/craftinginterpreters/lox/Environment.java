//> Statements and State 99
package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

class Environment {
  final Environment enclosing;
  private final Map<String, Object> values = new HashMap<>();

  Environment() {
    enclosing = null;
  }

  Environment(Environment enclosing) {
    this.enclosing = enclosing;
  }

  void declare(Token name) {
    // Note: Can't just use define(name, null). That will overwrite a
    // previously defined global value.
    if (!values.containsKey(name.text)) {
      values.put(name.text, null);
    }
  }

  Object get(Token name) {
    if (values.containsKey(name.text)) {
      return values.get(name.text);
    }

    if (enclosing != null) return enclosing.get(name);

    throw new RuntimeError(name,
        "Undefined variable '" + name.text + "'.");
  }

  void assign(Token name, Object value) {
    if (values.containsKey(name.text)) {
      values.put(name.text, value);
      return;
    }

    if (enclosing != null) {
      enclosing.assign(name, value);
      return;
    }

    throw new RuntimeError(name,
        "Undefined variable '" + name.text + "'.");
  }

  void define(String name, Object value) {
    values.put(name, value);
  }

//> Resolving and Binding 99
  Object getAt(int distance, String name) {
    Environment environment = this;
    for (int i = 0; i < distance; i++) {
      environment = environment.enclosing;
    }

    return environment.values.get(name);
  }

  void assignAt(int distance, Token name, Object value) {
    Environment environment = this;
    for (int i = 0; i < distance; i++) {
      environment = environment.enclosing;
    }

    environment.values.put(name.text, value);
  }
//< Resolving and Binding 99

  Environment enterScope() {
    return new Environment(this);
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
