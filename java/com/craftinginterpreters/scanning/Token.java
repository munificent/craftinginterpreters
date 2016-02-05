package com.craftinginterpreters.scanning;

class Token {
  final TokenType type;
  final String text;
  final Object value;
  final int line;

  Token(TokenType type, String text, Object value, int line) {
    this.type = type;
    this.text = text;
    this.value = value;
    this.line = line;
  }

  public String toString() {
    return type + " '" + text + "' " + value + " (line " + line + ")";
  }
}
