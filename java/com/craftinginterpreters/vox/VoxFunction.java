package com.craftinginterpreters.vox;

import java.util.List;

class VoxFunction implements Callable {
  private final Stmt.Function declaration;
  private final Environment closure;

  VoxFunction(Stmt.Function declaration, Environment closure) {
    this.declaration = declaration;
    this.closure = closure;
  }

  VoxFunction bind(VoxObject self) {
    return new VoxFunction(declaration, new LocalEnvironment(closure, "this", self));
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
    try {
      Environment environment = closure.enterScope();
      for (int i = 0; i < declaration.parameters.size(); i++) {
        environment = environment.define(
            declaration.parameters.get(i).text,
            arguments.get(i));
      }

      interpreter.execute(declaration.body, environment);
      return null;
    } catch (Return returnValue) {
      return returnValue.value;
    }
  }
}
