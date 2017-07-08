//> Functions lox-function
package com.craftinginterpreters.lox;

import java.util.List;

class LoxFunction implements Callable {
  private final Stmt.Function declaration;
//> closure-field
  private final Environment closure;
//< closure-field
//> Classes not-yet
  private final boolean isInitializer;
//< Classes not-yet

/* Functions lox-function < Functions closure-constructor
  LoxFunction(Stmt.Function declaration) {
*/
/* Functions closure-constructor < Classes not-yet
  LoxFunction(Stmt.Function declaration, Environment closure) {
*/
//> Classes not-yet
  LoxFunction(Stmt.Function declaration, Environment closure,
              boolean isInitializer) {
//< Classes not-yet
//> closure-constructor
    this.closure = closure;
//< closure-constructor
    this.declaration = declaration;
//> Classes not-yet
    this.isInitializer = isInitializer;
//< Classes not-yet
  }
//> Classes not-yet
  LoxFunction bind(LoxInstance self) {
    Environment environment = new Environment(closure);
    environment.define("this", self);
    return new LoxFunction(declaration, environment, isInitializer);
  }
//< Classes not-yet
//> function-to-string
  @Override
  public String toString() {
    return "<fn " + declaration.name.lexeme + ">";
  }
//< function-to-string
//> function-arity
  @Override
  public int arity() {
    return declaration.parameters.size();
  }
//< function-arity
//> function-call
  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
/* Functions function-call < Functions call-closure
    Environment environment = new Environment(interpreter.globals);
*/
//> call-closure
    Environment environment = new Environment(closure);
//< call-closure
    for (int i = 0; i < declaration.parameters.size(); i++) {
      environment.define(declaration.parameters.get(i).lexeme,
          arguments.get(i));
    }

/* Functions function-call < Functions catch-return
    interpreter.executeBlock(declaration.body, environment);
*/
//> catch-return
    try {
      interpreter.executeBlock(declaration.body, environment);
    } catch (Return returnValue) {
      return returnValue.value;
    }
//< catch-return
//> Classes not-yet
    if (isInitializer) return closure.getAt(0, "this");
//< Classes not-yet
    return null;
  }
//< function-call
}
