package com.craftinginterpreters.vox;

public enum TokenType {
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACKET,
  RIGHT_BRACKET,
  LEFT_BRACE,
  RIGHT_BRACE,
  COMMA,
  SEMICOLON,
  DOT,
  BANG,
  PLUS,
  MINUS,
  STAR,
  SLASH,
  PERCENT,
  EQUAL,
  EQUAL_EQUAL,
  BANG_EQUAL,
  LESS,
  GREATER,
  LESS_EQUAL,
  GREATER_EQUAL,

  IDENTIFIER,
  STRING,
  NUMBER,

  // Using keywords for "and" and "or" instead of "||" and "&&" since we don't
  // define the bitwise forms and it's weird to lex "||" without "|".
  AND,
  CLASS,
  ELSE,
  FUN,
  FOR,
  IF,
  NULL,
  OR,
  RETURN,
  VAR,
  WHILE,

  ERROR,
  EOF
}