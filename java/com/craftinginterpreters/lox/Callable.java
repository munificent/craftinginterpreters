//> Functions callable
package com.craftinginterpreters.lox;

import java.util.List;

interface Callable {
//> callable-arity
  int arity();
//< callable-arity
  Object call(Interpreter interpreter, List<Object> arguments);
}
