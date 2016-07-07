//>= Classes
package com.craftinginterpreters.vox;

import java.util.List;
import java.util.Map;

class VoxClass implements Callable {
  final String name;
//>= Inheritance
  final VoxClass superclass;
//>= Classes
  private final Map<String, VoxFunction> methods;

  VoxClass(String name, VoxClass superclass,
           Map<String, VoxFunction> methods) {
    this.name = name;
//>= Inheritance
    this.superclass = superclass;
//>= Classes
    this.methods = methods;
  }

  VoxFunction findMethod(VoxInstance instance, String name) {
//>= Inheritance
    VoxClass klass = this;
    while (klass != null) {
      if (klass.methods.containsKey(name)) {
        return klass.methods.get(name).bind(instance, klass.superclass);
      }

      klass = klass.superclass;
    }

    // Not found.
    return null;
//>= Classes
    // TODO: Non-inheritance version.
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
      initializer.bind(instance, superclass).call(interpreter, arguments);
    }

    return instance;
  }
}
