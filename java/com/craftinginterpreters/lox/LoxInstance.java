//> Classes lox-instance
package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

class LoxInstance {
  private LoxClass klass;
//> lox-instance-fields
  private final Map<String, Object> fields = new HashMap<>();
//< lox-instance-fields

  LoxInstance(LoxClass klass) {
    this.klass = klass;
  }
//> lox-instance-get-property
  Object get(Token name) {
    if (fields.containsKey(name.lexeme)) {
      return fields.get(name.lexeme);
    }

//> lox-instance-get-method
    LoxFunction method = klass.findMethod(this, name.lexeme);
    if (method != null) return method;

//< lox-instance-get-method
    throw new RuntimeError(name,
        "Undefined property '" + name.lexeme + "'.");
  }
//< lox-instance-get-property
//> lox-instance-set-property
  void set(Token name, Object value) {
    fields.put(name.lexeme, value);
  }
//< lox-instance-set-property
  @Override
  public String toString() {
    return klass.name + " instance";
  }
}
