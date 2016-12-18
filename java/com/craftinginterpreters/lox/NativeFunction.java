//>= Functions 99
package com.craftinginterpreters.lox;

import java.util.List;

class NativeFunction implements Callable {
  private final int requiredArguments;
  private final JavaFunction function;

  NativeFunction(int requiredArguments, JavaFunction function) {
    this.requiredArguments = requiredArguments;
    this.function = function;
  }

  interface JavaFunction {
    Object call(List<Object> arguments);
  }

  // The number of arguments this function requires.
  public int requiredArguments() {
    return requiredArguments;
  }

  public Object call(Interpreter interpreter, List<Object> arguments) {
    return function.call(arguments);
  }
}
