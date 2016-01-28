package com.craftinginterpreters.vox;

class Token {
  final TokenType type;
  final String text;

  Token(TokenType type) {
    this.type = type;
    text = "";
  }

  Token(TokenType type, String text) {
    this.type = type;
    this.text = text;
  }

  public String toString() {
    return type + " " + text;
  }
}