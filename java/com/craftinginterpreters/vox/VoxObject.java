package com.craftinginterpreters.vox;

import java.util.HashMap;
import java.util.Map;

class VoxObject {
  private VoxClass voxClass;
  final Map<String, Object> properties = new HashMap<>();

  void setClass(VoxClass voxClass) {
    this.voxClass = voxClass;
  }

  Object getProperty(Token name) {
    if (properties.containsKey(name.text)) return properties.get(name.text);

    if (voxClass.methods.containsKey(name.text)) {
      return voxClass.methods.get(name.text).bind(this);
    }

    throw new RuntimeError(
        "No property named '" + name.text + "' on " + this + ".", name);
  }

  @Override
  public String toString() {
    if (voxClass == null) return "[Object]";
    return voxClass.name + " instance";
  }
}
