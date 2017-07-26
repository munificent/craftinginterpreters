//> Functions callable
package com.craftinginterpreters.lox;

import java.util.List;

interface LoxCallable {
//> callable-arity
  int arity();
//< callable-arity
  Object call(Interpreter interpreter, List<Object> arguments);
}
