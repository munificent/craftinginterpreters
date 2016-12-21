//> Scanning 5
package com.craftinginterpreters.lox;

enum TokenType {
  // Punctuators.
  LEFT_PAREN, RIGHT_PAREN,
  LEFT_BRACE, RIGHT_BRACE,
  BANG, BANG_EQUAL,
  COMMA,
  DOT,
  EQUAL, EQUAL_EQUAL,
  GREATER, GREATER_EQUAL,
  LESS, LESS_EQUAL,
  MINUS,
  PLUS,
  SEMICOLON,
  SLASH,
  STAR,

  // Tokens with values.
  IDENTIFIER, STRING, NUMBER,

  // Keywords.
  AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR, PRINT, RETURN,
  SUPER, THIS, TRUE, VAR, WHILE,

  EOF
}
