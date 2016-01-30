package com.craftinginterpreters.vox;

import java.util.List;

// Defines the primitive functions available to Vox that are implemented in
// Java.
class Primitives {
  static boolean isTrue(Object object) {
    if (object == null) return false;
    if (object instanceof Boolean) return (boolean)object;
    return true;
  }

  static String stringify(Object object) {
    if (object == null) return "null";

    // Hack. Work around Java adding ".0" to integer-valued doubles.
    if (object instanceof Double) {
      String text = object.toString();
      if (text.endsWith(".0")) text = text.substring(0, text.length() - 2);
      return text;
    }

    return object.toString();
  }

  static Object print(Interpreter interpreter, List<Object> arguments) {
    System.out.println(stringify(arguments.get(0)));
    return arguments.get(0);
  }

  static Object clock(Interpreter interpreter, List<Object> arguments) {
    return (double)System.currentTimeMillis();
  }
}
