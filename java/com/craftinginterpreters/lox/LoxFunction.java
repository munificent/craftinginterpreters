//> Functions lox-function
package com.craftinginterpreters.lox;

import java.util.List;

class LoxFunction implements LoxCallable {
  private final Stmt.Function declaration;
//> closure-field
  private final Environment closure;
//< closure-field
//> Classes is-initializer-field
  private final boolean isInitializer;
//< Classes is-initializer-field

/* Functions lox-function < Functions closure-constructor
  LoxFunction(Stmt.Function declaration) {
*/
/* Functions closure-constructor < Classes lox-function-constructor
  LoxFunction(Stmt.Function declaration, Environment closure) {
*/
//> Classes lox-function-constructor
  LoxFunction(Stmt.Function declaration, Environment closure,
              boolean isInitializer) {
//< Classes lox-function-constructor
//> closure-constructor
    this.closure = closure;
//< closure-constructor
    this.declaration = declaration;
//> Classes initialize-is-initializer
    this.isInitializer = isInitializer;
//< Classes initialize-is-initializer
  }
//> Classes bind-self
  LoxFunction bind(LoxInstance self) {
    Environment environment = new Environment(closure);
    environment.define("this", self);
    return new LoxFunction(declaration, environment, isInitializer);
  }
//< Classes bind-self
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
//> Classes return-this
    if (isInitializer) return closure.getAt(0, "this");
//< Classes return-this
    return null;
  }
//< function-call
}
