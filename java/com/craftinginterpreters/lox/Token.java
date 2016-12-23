//> Scanning 5
package com.craftinginterpreters.lox;

class Token {
  final TokenType type;
  final String text;
  final Object value;
/* < 6

  Token(TokenType type, String text, Object value) {
    this.type = type;
    this.text = text;
    this.value = value;
  }

*/
//> 6
  final int line;

  Token(TokenType type, String text, Object value, int line) {
    this.type = type;
    this.text = text;
    this.value = value;
    this.line = line;
  }

//< 6
  public String toString() {
    return type + " " + text + " " + value;
  }
}
