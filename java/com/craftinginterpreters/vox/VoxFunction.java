package com.craftinginterpreters.vox;

import java.util.List;

class VoxFunction extends VoxObject implements Callable {
  private final Stmt.Function declaration;
  private final Environment closure;

  VoxFunction(Stmt.Function declaration, Environment closure) {
    this.declaration = declaration;
    this.closure = closure;
  }

  VoxFunction bind(VoxObject self) {
    Environment env = new Environment(closure);
    env.assign("this", self);
    return new VoxFunction(declaration, env);
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
      Environment env = new Environment(closure);
      for (int i = 0; i < declaration.parameters.size(); i++) {
        env.define(declaration.parameters.get(i), arguments.get(i));
      }

      interpreter.execute(declaration.body, env);
      return null;
    } catch (Return returnValue) {
      return returnValue.value;
    }
  }
}
