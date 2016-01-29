package com.craftinginterpreters.vox;

import java.util.HashMap;
import java.util.Map;

class Lexer {
  private static final Map<String, TokenType> keywords;

  static {
    keywords = new HashMap<>();
    keywords.put("and", TokenType.AND);
    keywords.put("class", TokenType.CLASS);
    keywords.put("else", TokenType.ELSE);
    keywords.put("false", TokenType.FALSE);
    keywords.put("for", TokenType.FOR);
    keywords.put("fun", TokenType.FUN);
    keywords.put("if", TokenType.IF);
    keywords.put("null", TokenType.NULL);
    keywords.put("or", TokenType.OR);
    keywords.put("return", TokenType.RETURN);
    keywords.put("true", TokenType.TRUE);
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

    // The next token starts with the current character.
    tokenStart = current;

    if (isAtEnd()) return makeToken(TokenType.EOF);

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
      case '+': return makeToken(TokenType.PLUS);
      case '-': return makeToken(TokenType.MINUS);
      case '*': return makeToken(TokenType.STAR);
      case '/': return makeToken(TokenType.SLASH);
      case '%': return makeToken(TokenType.PERCENT);
      case '!':
        if (match('=')) return makeToken(TokenType.BANG_EQUAL);
        return makeToken(TokenType.BANG);

      case '.':
        if (isDigit(peek())) return number();
        return makeToken(TokenType.DOT);

      case '=':
        if (match('=')) return makeToken(TokenType.EQUAL_EQUAL);
        return makeToken(TokenType.EQUAL);

      case '<':
        if (match('=')) return makeToken(TokenType.LESS_EQUAL);
        return makeToken(TokenType.LESS);

      case '>':
        if (match('=')) return makeToken(TokenType.GREATER_EQUAL);
        return makeToken(TokenType.GREATER);

      case '"': return string();
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
        while (peek() != '\n' && !isAtEnd()) advance();
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

    double value = Double.parseDouble(source.substring(tokenStart, current));
    return makeToken(TokenType.NUMBER, value);
  }

  private Token string() {
    // TODO: Escapes.
    // TODO: What about newlines?
    while (peek() != '"' && !isAtEnd()) advance();

    // Unterminated string.
    if (isAtEnd()) return makeToken(TokenType.ERROR, "Unterminated string.");

    // The closing ".
    advance();

    // Trim the surrounding quotes.
    String value = source.substring(tokenStart + 1, current - 1);
    return makeToken(TokenType.STRING, value);
  }

  private Token makeToken(TokenType type) {
    return makeToken(type, null);
  }

  private Token makeToken(TokenType type, Object value) {
    String text = source.substring(tokenStart, current);
    return new Token(type, text, value);
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

  private boolean isAtEnd() {
    return current >= source.length();
  }
}
