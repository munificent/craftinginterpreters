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

/*== Classes
  VoxClass(String name, Map<String, VoxFunction> methods) {
*/
//>= Inheritance
  VoxClass(String name, VoxClass superclass,
           Map<String, VoxFunction> methods) {
//>= Classes
    this.name = name;
//>= Inheritance
    this.superclass = superclass;
//>= Classes
    this.methods = methods;
  }

  VoxFunction findMethod(VoxInstance instance, String name) {
/*== Classes
    if (methods.containsKey(name)) {
      return methods.get(name).bind(instance);
    }

    // Not found.
    return null;
*/
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
/*== Classes
      initializer.bind(instance).call(interpreter, arguments);
*/
//>= Inheritance
      initializer.bind(instance, superclass).call(interpreter, arguments);
//>= Classes
    }

    return instance;
  }
}
