//>= Variables
package com.craftinginterpreters.vox;

import java.util.HashMap;
import java.util.Map;

class Environment {
  private final Environment enclosing;
  private final Map<String, Object> values = new HashMap<>();

  Environment(Environment enclosing) {
    this.enclosing = enclosing;
  }

  Object get(Token name) {
    return get(name.text, name.line);
  }

  Object get(String name, int line) {
    if (values.containsKey(name)) {
      return values.get(name);
    }

    if (enclosing != null) return enclosing.get(name, line);

    throw new RuntimeError(
        "Undefined variable '" + name + "'.", line);
  }

//>= Closures
  Object getAt(int distance, Token name) {
    Environment environment = this;
    for (int i = 0; i < distance; i++) {
      environment = environment.enclosing;
    }

    return environment.values.get(name.text);
  }

//>= Variables
  void set(Token name, Object value) {
    if (values.containsKey(name.text)) {
      values.put(name.text, value);
      return;
    }

    if (enclosing != null) {
      enclosing.set(name, value);
      return;
    }

    throw new RuntimeError(
        "Undefined variable '" + name.text + "'.", name);
  }

//>= Closures
  void setAt(int distance, Token name, Object value) {
    Environment environment = this;
    for (int i = 0; i < distance; i++) {
      environment = environment.enclosing;
    }

    environment.values.put(name.text, value);
  }

//>= Variables
  void declare(Token name) {
    // Note: Can't just use define(name, null). That will
    // overwrite a previously defined global value.
    if (!values.containsKey(name.text)) {
      values.put(name.text, null);
    }
  }

  void define(String name, Object value) {
    values.put(name, value);
  }

  Environment beginScope() {
    return new Environment(this);
  }
}
