package com.craftinginterpreters.vox;

class Token {
  final TokenType type;
  final String text;
  final Object value;

  Token(TokenType type, String text, Object value) {
    this.type = type;
    this.text = text;
    this.value = value;
  }

  public String toString() {
    return type + " " + text + " " + value;
  }
}