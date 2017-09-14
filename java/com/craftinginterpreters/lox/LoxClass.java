//> Classes lox-class
package com.craftinginterpreters.lox;

import java.util.List;
import java.util.Map;

class LoxClass implements LoxCallable {
  final String name;
//> Inheritance not-yet
  final LoxClass superclass;
//< Inheritance not-yet
  private final Map<String, LoxFunction> methods;

/* Classes lox-class < Inheritance not-yet
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
//> lox-class-find-method
  LoxFunction findMethod(LoxInstance instance, String name) {
/* Classes lox-class-find-method < Inheritance not-yet
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
//< lox-class-find-method

  @Override
  public String toString() {
    return name;
  }
//> lox-class-arity
  @Override
  public int arity() {
    LoxFunction initializer = methods.get("init");
    if (initializer == null) return 0;
    return initializer.arity();
  }
//< lox-class-arity
//> lox-class-call
  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    LoxInstance instance = new LoxInstance(this);

    LoxFunction initializer = methods.get("init");
    if (initializer != null) {
      initializer.bind(instance).call(interpreter, arguments);
    }

    return instance;
  }
//< lox-class-call
}
