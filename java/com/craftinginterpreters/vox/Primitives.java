package com.craftinginterpreters.vox;

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

  static String represent(Object object) {
    if (object instanceof String) {
      return "\"" + ((String) object).replace("\"", "\\\"") + "\"";
    }

    return stringify(object);
  }

  static Object print(Interpreter interpreter, Object argument) {
    System.out.println(stringify(argument));
    return argument;
  }

  static Object clock(Interpreter interpreter) {
    return (double)System.currentTimeMillis();
  }
}
