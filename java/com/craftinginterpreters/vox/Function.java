package com.craftinginterpreters.vox;

import java.util.List;

interface Function {
  Object call(Interpreter interpreter, List<Object> arguments);
}
