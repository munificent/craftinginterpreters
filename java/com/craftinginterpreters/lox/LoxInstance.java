//> Classes lox-instance
package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

class LoxInstance {
  private LoxClass klass;
  final Map<String, Object> fields = new HashMap<>();

  LoxInstance(LoxClass klass) {
    this.klass = klass;
  }
//> lox-instance-get-property
  Object getProperty(Token name) {
    if (fields.containsKey(name.lexeme)) {
      return fields.get(name.lexeme);
    }

    LoxFunction method = klass.findMethod(this, name.lexeme);
    if (method != null) return method;

    throw new RuntimeError(name,
        "Undefined property '" + name.lexeme + "'.");
  }
//< lox-instance-get-property

  @Override
  public String toString() {
    return klass.name + " instance";
  }
}
