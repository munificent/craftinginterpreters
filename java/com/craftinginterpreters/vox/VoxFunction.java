package com.craftinginterpreters.vox;

import java.util.List;

class VoxFunction implements Callable {
  private final Stmt.Function declaration;
  private final Environment closure;
  private final boolean isInitializer;

  VoxFunction(Stmt.Function declaration, Environment closure,
              boolean isInitializer) {
    this.declaration = declaration;
    this.closure = closure;
    this.isInitializer = isInitializer;
  }

  VoxFunction bind(VoxInstance self, VoxClass methodClass) {
    Environment scope = closure
        .enterScope()
        .define("this", self)
        .define("class", methodClass);
    return new VoxFunction(declaration, scope, isInitializer);
  }

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
        environment = environment.define(
            declaration.parameters.get(i).text,
            arguments.get(i));
      }

      interpreter.execute(declaration.body, environment);
    } catch (Return returnValue) {
      result = returnValue.value;
    }

    return isInitializer ? closure.get("this", 0) : result;
  }
}
