package com.craftinginterpreters.vox;

import java.util.HashMap;
import java.util.Map;

class GlobalEnvironment implements Environment {
  private final Map<String, Object> values = new HashMap<>();

  @Override
  public Object get(String name, int line) {
    if (!values.containsKey(name)) {
      throw new RuntimeError("Undefined variable '" + name + "'.", line);
    }

    return values.get(name);

  }

  // TODO: Move into Environment?
  @Override
  public Object get(Token name) {
    return get(name.text, name.line);
  }

  @Override
  public void set(Token name, Object value) {
    if (!values.containsKey(name.text)) {
      throw new RuntimeError("Undefined variable '" + name.text + "'.", name);
    }

    values.put(name.text, value);
  }

  @Override
  public Environment declare(Token name) {
    // Note: Can't just use define(name, null). That will
    // overwrite a previously defined global value.
    if (!values.containsKey(name.text)) {
      values.put(name.text, null);
    }
    return this;
  }

  @Override
  public Environment define(String name, Object value) {
    values.put(name, value);
    return this;
  }

  @Override
  public Environment enterScope() {
    return new LocalEnvironment(this, "", null);
  }
}
