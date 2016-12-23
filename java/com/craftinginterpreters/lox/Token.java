//> Scanning token-class
package com.craftinginterpreters.lox;

class Token {
  final TokenType type;
  final String text;
  final Object value;
/* < token-line

  Token(TokenType type, String text, Object value) {
    this.type = type;
    this.text = text;
    this.value = value;
  }

*/
//> token-line
  final int line;

  Token(TokenType type, String text, Object value, int line) {
    this.type = type;
    this.text = text;
    this.value = value;
    this.line = line;
  }

//< token-line
  public String toString() {
    return type + " " + text + " " + value;
  }
}
