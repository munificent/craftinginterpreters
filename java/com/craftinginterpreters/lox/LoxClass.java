//> Classes lox-class
package com.craftinginterpreters.lox;

import java.util.List;
import java.util.Map;

/* Classes lox-class < Classes lox-class-callable
class LoxClass {
*/
//> lox-class-callable
class LoxClass implements LoxCallable {
//< lox-class-callable
  final String name;
//> Inheritance not-yet
  final LoxClass superclass;
//< Inheritance not-yet
/* Classes lox-class < Classes lox-class-methods

  LoxClass(String name) {
    this.name = name;
  }
*/
//> lox-class-methods
  private final Map<String, LoxFunction> methods;

/* Classes lox-class-methods < Inheritance not-yet
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
//< lox-class-methods
//> lox-class-find-method
  LoxFunction findMethod(LoxInstance instance, String name) {
    if (methods.containsKey(name)) {
/* Classes lox-class-find-method < Classes lox-class-find-method-bind
      return methods.get(name);
*/
//> lox-class-find-method-bind
      return methods.get(name).bind(instance);
//< lox-class-find-method-bind
    }

//> Inheritance not-yet
    if (superclass != null) {
      return superclass.findMethod(instance, name);
    }

//< Inheritance not-yet
    return null;
  }
//< lox-class-find-method

  @Override
  public String toString() {
    return name;
  }
//> lox-class-arity
  @Override
  public int arity() {
/* Classes lox-class-arity < Classes lox-initializer-arity
    return 0;
*/
//> lox-initializer-arity
    LoxFunction initializer = methods.get("init");
    if (initializer == null) return 0;
    return initializer.arity();
//< lox-initializer-arity
  }
//< lox-class-arity
//> lox-class-call
  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    LoxInstance instance = new LoxInstance(this);
//> lox-class-call-initializer
    LoxFunction initializer = methods.get("init");
    if (initializer != null) {
      initializer.bind(instance).call(interpreter, arguments);
    }

//< lox-class-call-initializer
    return instance;
  }
//< lox-class-call
}
