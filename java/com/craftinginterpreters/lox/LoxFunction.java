//> Functions not-yet
package com.craftinginterpreters.lox;

import java.util.List;

class LoxFunction implements Callable {
  private final Stmt.Function declaration;
  private final Environment closure;
//> Classes not-yet
  private final boolean isInitializer;
//< Classes not-yet

/* Functions not-yet < Classes not-yet
LoxFunction(Stmt.Function declaration, Environment closure) {
*/
//> Classes not-yet
  LoxFunction(Stmt.Function declaration, Environment closure,
              boolean isInitializer) {
//< Classes not-yet
    this.declaration = declaration;
    this.closure = closure;
//> Classes not-yet
    this.isInitializer = isInitializer;
//< Classes not-yet
  }

//> Classes not-yet
  LoxFunction bind(LoxInstance self) {
    Environment environment = closure.enterScope();
    environment.define("this", self);
    return new LoxFunction(declaration, environment, isInitializer);
  }

//< Classes not-yet
  @Override
  public String toString() {
    return declaration.name.text;
  }

  @Override
  public int requiredArguments() {
    return declaration.parameters.size();
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    Object result = null;

    try {
      Environment environment = closure.enterScope();
      for (int i = 0; i < declaration.parameters.size(); i++) {
        environment.define(declaration.parameters.get(i).text,
            arguments.get(i));
      }

      interpreter.executeBody(declaration.body, environment);
    } catch (Return returnValue) {
      result = returnValue.value;
    }

/* Functions not-yet < Classes not-yet
    return result;
*/
//> Classes not-yet
    return isInitializer ? closure.getAt(0, "this") : result;
//< Classes not-yet
  }
}
