package com.craftinginterpreters.vox;

import java.util.HashMap;
import java.util.Map;

class VoxObject {
  private VoxClass voxClass;
  final Map<String, Object> properties = new HashMap<>();

  void setClass(VoxClass voxClass) {
    this.voxClass = voxClass;
  }

  Object getProperty(String name) {
    if (properties.containsKey(name)) return properties.get(name);

    if (voxClass.methods.containsKey(name)) {
      return voxClass.methods.get(name).bind(this);
    }

    throw new RuntimeError("No property named '" + name + "' on " + this);
  }

  @Override
  public String toString() {
    if (voxClass == null) return "[Object]";
    return voxClass.name + " instance";
  }
}
