//>= Classes
package com.craftinginterpreters.lox;

import java.util.List;
import java.util.Map;

class LoxClass implements Callable {
  final String name;
//>= Inheritance
  final LoxClass superclass;
//>= Classes
  private final Map<String, LoxFunction> methods;

/*== Classes
  LoxClass(String name, Map<String, LoxFunction> methods) {
*/
//>= Inheritance
  LoxClass(String name, LoxClass superclass,
           Map<String, LoxFunction> methods) {
//>= Classes
    this.name = name;
//>= Inheritance
    this.superclass = superclass;
//>= Classes
    this.methods = methods;
  }

  LoxFunction findMethod(LoxInstance instance, String name) {
/*== Classes
    if (methods.containsKey(name)) {
      return methods.get(name).bind(instance);
    }

    // Not found.
    return null;
*/
//>= Inheritance
    LoxClass klass = this;
    while (klass != null) {
      if (klass.methods.containsKey(name)) {
        return klass.methods.get(name).bind(instance);
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
