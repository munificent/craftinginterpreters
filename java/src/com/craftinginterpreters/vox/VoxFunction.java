package com.craftinginterpreters.vox;

import java.util.List;

class VoxFunction extends VoxObject implements Function {
  final Stmt.Function declaration;
  final Variables closure;

  VoxFunction(Stmt.Function declaration, Variables closure) {
    this.declaration = declaration;
    this.closure = closure;
  }

  VoxFunction bind(VoxObject self) {
    return new VoxFunction(declaration, closure.define("this", self));
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    Variables before = interpreter.variables;
    try {
      interpreter.variables = closure;
      for (int i = 0; i < declaration.parameters.size(); i++) {
        interpreter.variables = interpreter.variables.define(
            declaration.parameters.get(i), arguments.get(i));
      }

      interpreter.execute(declaration.body);
      return null;
    } catch (Return returnValue) {
      return returnValue.value;
    } finally {
      interpreter.variables = before;
    }
  }
}
