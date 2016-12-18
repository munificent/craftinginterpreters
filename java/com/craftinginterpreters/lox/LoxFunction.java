//>> Functions 99
package com.craftinginterpreters.lox;

import java.util.List;

class LoxFunction implements Callable {
  private final Stmt.Function declaration;
  private final Environment closure;
//>> Classes 99
  private final boolean isInitializer;
//<< Classes 99

/*>= Functions 99 < Classes 99
LoxFunction(Stmt.Function declaration, Environment closure) {
*/
//>> Classes 99
  LoxFunction(Stmt.Function declaration, Environment closure,
              boolean isInitializer) {
//<< Classes 99
    this.declaration = declaration;
    this.closure = closure;
//>> Classes 99
    this.isInitializer = isInitializer;
//<< Classes 99
  }

//>> Classes 99
  LoxFunction bind(LoxInstance self) {
    Environment environment = closure.enterScope();
    environment.define("this", self);
    return new LoxFunction(declaration, environment, isInitializer);
  }

//<< Classes 99
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

/*>= Functions 99 < Classes 99
    return result;
*/
//>> Classes 99
    return isInitializer ? closure.getAt(0, "this") : result;
//<< Classes 99
  }
}
