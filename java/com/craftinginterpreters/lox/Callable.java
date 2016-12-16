//>= Functions 1
package com.craftinginterpreters.lox;

import java.util.List;

interface Callable {
//>= Functions 1
  // The number of arguments this function requires.
  int requiredArguments();

  Object call(Interpreter interpreter, List<Object> arguments);
}
