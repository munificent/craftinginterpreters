//>= Scanning 5
package com.craftinginterpreters.lox;

enum TokenType {
  LEFT_PAREN, RIGHT_PAREN,
  LEFT_BRACE, RIGHT_BRACE,
  BANG,
  BANG_EQUAL,
  COMMA,
  DOT,
  EQUAL,
  EQUAL_EQUAL,
  GREATER,
  GREATER_EQUAL,
  LESS,
  LESS_EQUAL,
  MINUS,
  PLUS,
  SEMICOLON,
  SLASH,
  STAR,

  IDENTIFIER, STRING, NUMBER,

  // Using keywords for "and" and "or" instead of "||" and "&&"
  // since we don't define the bitwise forms and it's weird to
  // lex "||" without "|".
  AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR, PRINT, RETURN,
  SUPER, THIS, TRUE, VAR, WHILE,

  EOF
}