//>= Classes 1
package com.craftinginterpreters.lox;

import java.util.List;
import java.util.Map;

class LoxClass implements Callable {
  final String name;
//>= Inheritance 1
  final LoxClass superclass;
//>= Classes 1
  private final Map<String, LoxFunction> methods;

/*>= Classes 1 < Inheritance 1
  LoxClass(String name, Map<String, LoxFunction> methods) {
*/
//>= Inheritance 1
  LoxClass(String name, LoxClass superclass,
           Map<String, LoxFunction> methods) {
//>= Classes 1
    this.name = name;
//>= Inheritance 1
    this.superclass = superclass;
//>= Classes 1
    this.methods = methods;
  }

  LoxFunction findMethod(LoxInstance instance, String name) {
/*>= Classes 1 < Inheritance 1
    if (methods.containsKey(name)) {
      return methods.get(name).bind(instance);
    }

    // Not found.
    return null;
*/
//>= Inheritance 1
    LoxClass klass = this;
    while (klass != null) {
      if (klass.methods.containsKey(name)) {
        return klass.methods.get(name).bind(instance);
      }

      klass = klass.superclass;
    }

    // Not found.
    return null;
//>= Classes 1
  }

  @Override
  public String toString() {
    return name;
  }

  @Override
  public int requiredArguments() {
    LoxFunction initializer = methods.get("init");
    if (initializer == null) return 0;
    return initializer.requiredArguments();
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    LoxInstance instance = new LoxInstance(this);

    LoxFunction initializer = methods.get("init");
    if (initializer != null) {
      initializer.bind(instance).call(interpreter, arguments);
    }

    return instance;
  }
}
