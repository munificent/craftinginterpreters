package com.craftinginterpreters.vox;

import java.util.List;

// Defines the primitive functions available to Vox that are implemented in
// Java.
public class Primitives {
  static Object print(Interpreter interpreter, List<Object> arguments) {
    Object value = arguments.get(0);
    if (value instanceof Double) {
      // TODO: Hack. Work around Java adding ".0" to integer-valued doubles.
      String text = value.toString();
      if (text.endsWith(".0")) text = text.substring(0, text.length() - 2);
      System.out.println(text);
    } else {
      System.out.println(value);
    }

    return value;
  }
}
