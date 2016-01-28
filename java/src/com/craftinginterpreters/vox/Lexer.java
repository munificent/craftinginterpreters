package com.craftinginterpreters.vox;

import java.util.HashMap;
import java.util.Map;

class Lexer {
  private static final Map<String, TokenType> keywords;

  static {
    keywords = new HashMap<String, TokenType>();
    keywords.put("and", TokenType.AND);
    keywords.put("class", TokenType.CLASS);
    keywords.put("else", TokenType.ELSE);
    keywords.put("for", TokenType.FOR);
    keywords.put("fun", TokenType.FUN);
    keywords.put("if", TokenType.IF);
    keywords.put("null", TokenType.NULL);
    keywords.put("or", TokenType.OR);
    keywords.put("return", TokenType.RETURN);
    keywords.put("var", TokenType.VAR);
    keywords.put("while", TokenType.WHILE);
  }

  private final String source;
  private int tokenStart = 0;
  private int current = 0;

  Lexer(String source) {
    this.source = source;
  }

  Token nextToken() {
    skipWhitespace();

    if (current >= source.length()) return new Token(TokenType.EOF, "");

    // The next token starts with the current character.
    tokenStart = current;

    char c = advance();

    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
      case '(': return makeToken(TokenType.LEFT_PAREN);
      case ')': return makeToken(TokenType.RIGHT_PAREN);
      case '[': return makeToken(TokenType.LEFT_BRACKET);
      case ']': return makeToken(TokenType.RIGHT_BRACKET);
      case '{': return makeToken(TokenType.LEFT_BRACE);
      case '}': return makeToken(TokenType.RIGHT_BRACE);
      case ';': return makeToken(TokenType.SEMICOLON);
      case ',': return makeToken(TokenType.COMMA);
//      case "\"": return this.string();
      case '+': return makeToken(TokenType.PLUS);
      case '-': return makeToken(TokenType.MINUS);
      case '*': return makeToken(TokenType.STAR);
      case '/': return makeToken(TokenType.SLASH);
      case '%': return makeToken(TokenType.PERCENT);
      case '!':
        if (match('=')) return makeToken(TokenType.BANG_EQUAL);
        return makeToken(TokenType.BANG);

//      case ".":
//        if (isDigit(this.peek())) return this.number();
//
//        return this.makeToken(Token.dot);

      case '=':
        if (match('=')) return makeToken(TokenType.EQUAL_EQUAL);
        return makeToken(TokenType.EQUAL);

      case '<':
        if (match('=')) return makeToken(TokenType.LESS_EQUAL);
        return makeToken(TokenType.LESS);

      case '>':
        if (match('=')) return makeToken(TokenType.GREATER_EQUAL);
        return makeToken(TokenType.GREATER);
    }

    return makeToken(TokenType.ERROR);
  }

  private void skipWhitespace() {
    while (true) {
      char c = peek();
      if (isWhitespace(c)) {
        advance();
      } else if (c == '/' && peek(1) == '/') {
        // A comment goes until the end of the line.
        while (peek() != '\n' && current < source.length()) advance();
      } else {
        break;
      }
    }
  }

  private Token identifier() {
    while (isAlphaNumeric(peek())) advance();

    // See if the identifier is a reserved word.
    String text = source.substring(tokenStart, current);

    TokenType type = keywords.get(text);
    if (type == null) type = TokenType.IDENTIFIER;

    return makeToken(type);
  }

  private Token number() {
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && !isAlpha(peek(1))) {
      // Consume the "."
      advance();

      while (isDigit(peek())) advance();
    }

    // TODO: Parse number?
    return makeToken(TokenType.NUMBER);
  }

//  Lexer.prototype.string = function() {
//    // TODO: Escapes.
//    // TODO: What about newlines?
//    this.consumeWhile(function(c) { return c != "\""; });
//
//    // Unterminated string.
//    if (this.isAtEnd()) return this.makeToken(Token.error);
//
//    // The closing ".
//    this.advance();
//
//    return this.makeToken(Token.string, function(text) {
//      // Trim the surrounding quotes.
//      return text.substring(1, text.length - 1);
//    });
//  }

  private Token makeToken(TokenType type) {
    String text = source.substring(tokenStart, current);
    return new Token(type, text);
  }

  private boolean match(char expected) {
    if (current >= source.length()) return false;
    if (source.charAt(current) != expected) return false;

    current++;
    return true;
  }

  private char advance() {
    current++;
    return source.charAt(current - 1);
  }

  private char peek() {
    return peek(0);
  }

  private char peek(int ahead) {
    if (current + ahead >= source.length()) return '\0';

    return source.charAt(current + ahead);
  }

  // Returns true if `c` is an English letter or underscore.
  private boolean isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
  }

  // Returns true if `c` is an English letter, underscore, or digit.
  private boolean isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
  }

  // Returns true if `c` is a space, newline, or tab.
  private boolean isWhitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t';
  }

  // Returns true if `c` is a digit.
  private boolean isDigit(char c) {
    return c >= '0' && c <= '9';
  }

  // TODO: Use?
  private boolean isAtEnd() {
    return current >= source.length();
  }
}
