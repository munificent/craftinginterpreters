package com.craftinginterpreters.vox;

import java.util.HashMap;
import java.util.Map;

import static com.craftinginterpreters.vox.TokenType.*;

class Scanner {
  private static final Map<String, TokenType> keywords;

  static {
    keywords = new HashMap<>();
    keywords.put("and",    AND);
    keywords.put("class",  CLASS);
    keywords.put("else",   ELSE);
    keywords.put("false",  FALSE);
    keywords.put("for",    FOR);
    keywords.put("fun",    FUN);
    keywords.put("if",     IF);
    keywords.put("null",   NULL);
    keywords.put("or",     OR);
    keywords.put("return", RETURN);
    keywords.put("super",  SUPER);
    keywords.put("this",   THIS);
    keywords.put("true",   TRUE);
    keywords.put("var",    VAR);
    keywords.put("while",  WHILE);
  }

  private final String source;
  private int tokenStart = 0;
  private int current = 0;
  private int line = 1;

  Scanner(String source) {
    this.source = source;
  }

  Token scanToken() {
    skipWhitespace();

    // The next token starts with the current character.
    tokenStart = current;

    if (isAtEnd()) return makeToken(EOF);

    char c = advance();

    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
      case '(': return makeToken(LEFT_PAREN);
      case ')': return makeToken(RIGHT_PAREN);
      case '[': return makeToken(LEFT_BRACKET);
      case ']': return makeToken(RIGHT_BRACKET);
      case '{': return makeToken(LEFT_BRACE);
      case '}': return makeToken(RIGHT_BRACE);
      case ';': return makeToken(SEMICOLON);
      case ',': return makeToken(COMMA);
      case '+': return makeToken(PLUS);
      case '-': return makeToken(MINUS);
      case '*': return makeToken(STAR);
      case '/': return makeToken(SLASH);
      case '!':
        if (match('=')) return makeToken(BANG_EQUAL);
        return makeToken(BANG);

      case '.':
        if (isDigit(peek())) return number();
        return makeToken(DOT);

      case '=':
        if (match('=')) return makeToken(EQUAL_EQUAL);
        return makeToken(EQUAL);

      case '<':
        if (match('=')) return makeToken(LESS_EQUAL);
        return makeToken(LESS);

      case '>':
        if (match('=')) return makeToken(GREATER_EQUAL);
        return makeToken(GREATER);

      case '"': return string();
    }

    return makeToken(ERROR,
        "Unexpected character '" + c + "'.");
  }

  private void skipWhitespace() {
    while (true) {
      char c = peek();
      switch (c) {
        case ' ':
        case '\r':
        case '\t':
          advance();
          break;

        case '\n':
          line++;
          advance();
          break;

        case '/':
          if (peek(1) == '/') {
            // A comment goes until the end of the line.
            while (peek() != '\n' && !isAtEnd()) advance();
          } else {
            return;
          }
          break;

        default:
          return;
      }
    }
  }

  private Token identifier() {
    while (isAlphaNumeric(peek())) advance();

    // See if the identifier is a reserved word.
    String text = source.substring(tokenStart, current);

    TokenType type = keywords.get(text);
    if (type == null) type = IDENTIFIER;

    return makeToken(type);
  }

  private Token number() {
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peek(1))) {
      // Consume the "."
      advance();

      while (isDigit(peek())) advance();
    }

    double value = Double.parseDouble(
        source.substring(tokenStart, current));
    return makeToken(NUMBER, value);
  }

  private Token string() {
    // TODO: What about newlines?
    while (peek() != '"' && !isAtEnd()) advance();

    // Unterminated string.
    if (isAtEnd()) {
      return makeToken(ERROR, "Unterminated string.");
    }

    // The closing ".
    advance();

    // Trim the surrounding quotes.
    String value = source.substring(
        tokenStart + 1, current - 1);
    return makeToken(STRING, value);
  }

  private Token makeToken(TokenType type) {
    return makeToken(type, null);
  }

  private Token makeToken(TokenType type, Object value) {
    String text = source.substring(tokenStart, current);
    return new Token(type, text, value, line);
  }

  private boolean match(char expected) {
    if (isAtEnd()) return false;
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

  // Returns true if `c` is an English letter, underscore,
  // or digit.
  private boolean isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
  }

  // Returns true if `c` is a digit.
  private boolean isDigit(char c) {
    return c >= '0' && c <= '9';
  }

  private boolean isAtEnd() {
    return current >= source.length();
  }
}
