//> Classes not-yet
package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

class LoxInstance {
  private LoxClass klass;
  final Map<String, Object> fields = new HashMap<>();

  LoxInstance(LoxClass klass) {
    this.klass = klass;
  }

  Object getProperty(Token name) {
    if (fields.containsKey(name.text)) {
      return fields.get(name.text);
    }

    LoxFunction method = klass.findMethod(this, name.text);
    if (method != null) return method;

    throw new RuntimeError(name,
        "Undefined property '" + name.text + "'.");
  }

  @Override
  public String toString() {
    return klass.name + " instance";
  }
}
