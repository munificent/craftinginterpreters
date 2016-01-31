package com.craftinginterpreters.vox;

import java.util.List;
import java.util.Map;

class VoxClass extends VoxObject implements Callable {
  final String name;
  // TODO: Superclass.
  private final VoxFunction constructor;
  final Map<String, VoxFunction> methods;

  VoxClass(String name, VoxFunction constructor,
           Map<String, VoxFunction> methods) {
    this.name = name;
    this.constructor = constructor;
    this.methods = methods;
  }

  @Override
  public String toString() {
    return name;
  }

  @Override
  public int requiredArguments() {
    if (constructor == null) return 0;
    return constructor.requiredArguments();
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    VoxObject instance = new VoxObject();
    instance.setClass(this);

    if (constructor != null) {
      constructor.bind(instance).call(interpreter, arguments);
    }

    return instance;
  }
}
