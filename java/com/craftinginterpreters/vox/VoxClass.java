package com.craftinginterpreters.vox;

import java.util.List;
import java.util.Map;

class VoxClass implements Callable {
  final String name;
  final VoxClass superclass;
  private final Map<String, VoxFunction> methods;

  VoxClass(String name, VoxClass superclass, Map<String, VoxFunction> methods) {
    this.name = name;
    this.superclass = superclass;
    this.methods = methods;
  }

  VoxFunction findMethod(VoxInstance instance, String name) {
    VoxClass voxClass = this;
    while (voxClass != null) {
      if (voxClass.methods.containsKey(name)) {
        return voxClass.methods.get(name).bind(instance, voxClass);
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
    VoxFunction initializer = methods.get("init");
    if (initializer == null) return 0;
    return initializer.requiredArguments();
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    VoxInstance instance = new VoxInstance(this);

    VoxFunction initializer = methods.get("init");
    if (initializer != null) {
      initializer.bind(instance, this).call(interpreter, arguments);
    }

    return instance;
  }
}
