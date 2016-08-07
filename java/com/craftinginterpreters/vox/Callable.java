//>= Functions
package com.craftinginterpreters.vox;

import java.util.List;

interface Callable {
//>= Functions
  // The number of arguments this function requires.
  int requiredArguments();

  Object call(Interpreter interpreter, List<Object> arguments);
}
