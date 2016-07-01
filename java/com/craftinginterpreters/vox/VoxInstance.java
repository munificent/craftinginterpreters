//>= Classes
package com.craftinginterpreters.vox;

import java.util.HashMap;
import java.util.Map;

class VoxInstance {
  private VoxClass klass;
  final Map<String, Object> fields = new HashMap<>();

  VoxInstance(VoxClass klass) {
    this.klass = klass;
  }

  Object getProperty(Token name) {
    if (fields.containsKey(name.text)) {
      return fields.get(name.text);
    }

    VoxFunction method = klass.findMethod(this, name.text);
    if (method != null) return method;

    throw new RuntimeError(name,
        "Undefined property '" + name.text + "'.");
  }

  @Override
  public String toString() {
    return klass.name + " instance";
  }
}
