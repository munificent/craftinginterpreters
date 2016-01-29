package com.craftinginterpreters.vox;

import java.util.List;

interface Callable {
  Object call(Interpreter interpreter, List<Object> arguments);
}
