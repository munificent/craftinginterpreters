package com.craftinginterpreters.vox;

import java.util.HashMap;
import java.util.Map;

class Environment {
  final Environment outer;
  final private Map<String, Object> variables = new HashMap<>();

  Environment(Environment outer) {
    this.outer = outer;
  }

  void define(Token name, Object value) {
    // TODO: Error on duplicate definition in non-global scope.
    variables.put(name.text, value);
  }

  void assign(String name, Object value) {
    variables.put(name, value);
  }

  Object get(String name) {
    return variables.get(name);
  }

  Environment find(Token name) {
    Environment env = this;
    while (env != null) {
      if (env.variables.containsKey(name.text)) return env;
      env = env.outer;
    }

    throw new RuntimeError("Undefined variable '" + name.text + "'.", name);
  }
}
