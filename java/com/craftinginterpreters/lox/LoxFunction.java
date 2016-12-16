//>= Functions 1
package com.craftinginterpreters.lox;

import java.util.List;

class LoxFunction implements Callable {
  private final Stmt.Function declaration;
  private final Environment closure;
//>= Classes 1
  private final boolean isInitializer;
//>= Functions 1

/*>= Functions 1 < Classes 1
LoxFunction(Stmt.Function declaration, Environment closure) {
*/
//>= Classes 1
  LoxFunction(Stmt.Function declaration, Environment closure,
              boolean isInitializer) {
//>= Functions 1
    this.declaration = declaration;
    this.closure = closure;
//>= Classes 1
    this.isInitializer = isInitializer;
//>= Functions 1
  }

//>= Classes 1
  LoxFunction bind(LoxInstance self) {
    Environment environment = closure.enterScope();
    environment.define("this", self);
    return new LoxFunction(declaration, environment, isInitializer);
  }

//>= Functions 1
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

/*>= Functions 1 < Classes 1
    return result;
*/
//>= Classes 1
    return isInitializer ? closure.getAt(0, "this") : result;
//>= Functions 1
  }
}
