//> Classes not-yet
package com.craftinginterpreters.lox;

import java.util.List;
import java.util.Map;

class LoxClass implements LoxCallable {
  final String name;
//> Inheritance not-yet
  final LoxClass superclass;
//< Inheritance not-yet
  private final Map<String, LoxFunction> methods;

/* Classes not-yet < Inheritance not-yet
  LoxClass(String name, Map<String, LoxFunction> methods) {
*/
//> Inheritance not-yet
  LoxClass(String name, LoxClass superclass,
           Map<String, LoxFunction> methods) {
//< Inheritance not-yet
    this.name = name;
//> Inheritance not-yet
    this.superclass = superclass;
//< Inheritance not-yet
    this.methods = methods;
  }

  LoxFunction findMethod(LoxInstance instance, String name) {
/* Classes not-yet < Inheritance not-yet
    if (methods.containsKey(name)) {
      return methods.get(name).bind(instance);
    }

    // Not found.
    return null;
*/
//> Inheritance not-yet
    LoxClass klass = this;
    while (klass != null) {
      if (klass.methods.containsKey(name)) {
        return klass.methods.get(name).bind(instance);
      }

      klass = klass.superclass;
    }

    // Not found.
    return null;
//< Inheritance not-yet
  }

  @Override
  public String toString() {
    return name;
  }

  @Override
  public int arity() {
    LoxFunction initializer = methods.get("init");
    if (initializer == null) return 0;
    return initializer.arity();
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    LoxInstance instance = new LoxInstance(this);

    LoxFunction initializer = methods.get("init");
    if (initializer != null) {
      initializer.bind(instance).call(interpreter, arguments);
    }

    return instance;
  }
}
