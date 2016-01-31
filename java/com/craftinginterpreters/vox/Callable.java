package com.craftinginterpreters.vox;

import java.util.List;

interface Callable {
  static Callable wrap(Primitive0 primitive) {
    return new Callable() {
      @Override
      public int requiredArguments() {
        return 0;
      }

      @Override
      public Object call(Interpreter interpreter, List<Object> arguments) {
        return primitive.call(interpreter);
      }
    };
  }

  static Callable wrap(Primitive1 primitive) {
    return new Callable() {
      @Override
      public int requiredArguments() {
        return 1;
      }

      @Override
      public Object call(Interpreter interpreter, List<Object> arguments) {
        return primitive.call(interpreter, arguments.get(0));
      }
    };
  }

  interface Primitive0 {
    Object call(Interpreter interpreter);
  }

  interface Primitive1 {
    Object call(Interpreter interpreter, Object argument);
  }

  // The number of arguments this function requires.
  int requiredArguments();

  Object call(Interpreter interpreter, List<Object> arguments);
}
