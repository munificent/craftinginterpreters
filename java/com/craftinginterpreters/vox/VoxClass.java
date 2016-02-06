package com.craftinginterpreters.vox;

import java.util.List;
import java.util.Map;

class VoxClass extends VoxObject implements Callable {
  final String name;
  private final VoxClass superclass;
  private final VoxFunction constructor;
  private final Map<String, VoxFunction> methods;

  VoxClass(String name, VoxClass superclass, VoxFunction constructor,
           Map<String, VoxFunction> methods) {
    this.name = name;
    this.superclass = superclass;
    this.constructor = constructor;
    this.methods = methods;
  }

  VoxFunction findMethod(String name) {
    VoxClass voxClass = this;
    while (voxClass != null) {
      if (voxClass.methods.containsKey(name)) {
        return voxClass.methods.get(name).bind(this);
      }

      voxClass = voxClass.superclass;
    }

    // Not found.
    return null;
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
