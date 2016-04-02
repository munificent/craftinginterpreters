package com.craftinginterpreters.vox;

import java.util.HashMap;
import java.util.Map;

class VoxObject {
  private VoxClass voxClass;
  final Map<String, Object> fields = new HashMap<>();

  VoxObject(VoxClass voxClass) {
    this.voxClass = voxClass;
  }

  Object getField(Token name) {
    if (fields.containsKey(name.text)) {
      return fields.get(name.text);
    }

    VoxFunction method = voxClass.findMethod(this, name.text);
    if (method != null) return method;

    throw new RuntimeError(
        "Undefined field '" + name.text + "'.", name);
  }

  @Override
  public String toString() {
    return voxClass.name + " instance";
  }
}
