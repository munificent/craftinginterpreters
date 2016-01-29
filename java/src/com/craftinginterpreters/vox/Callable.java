package com.craftinginterpreters.vox;

import java.util.List;

interface Callable {
  Object call(List<Object> arguments);
}
