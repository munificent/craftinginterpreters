//>= Scanning
package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.craftinginterpreters.lox.TokenType.*;

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
    keywords.put("nil",    NIL);
    keywords.put("or",     OR);
    keywords.put("print",  PRINT);
    keywords.put("return", RETURN);
    keywords.put("super",  SUPER);
    keywords.put("this",   THIS);
    keywords.put("true",   TRUE);
    keywords.put("var",    VAR);
    keywords.put("while",  WHILE);
  }

  private final String source;
  private final ErrorReporter errorReporter;
  private final List<Token> tokens = new ArrayList<>();
  private int tokenStart = 0;
  private int current = 0;
  private int line = 1;

  Scanner(String source, ErrorReporter errorReporter) {
    this.source = source;
    this.errorReporter = errorReporter;
  }

  List<Token> scanTokens() {
    while (!isAtEnd()) {
      // The next token starts with the current character.
      tokenStart = current;

      char c = advance();
      switch (c) {
        case '(': addToken(LEFT_PAREN); break;
        case ')': addToken(RIGHT_PAREN); break;
        case '{': addToken(LEFT_BRACE); break;
        case '}': addToken(RIGHT_BRACE); break;
        case ';': addToken(SEMICOLON); break;
        case ',': addToken(COMMA); break;
        case '+': addToken(PLUS); break;
        case '-': addToken(MINUS); break;
        case '*': addToken(STAR); break;
        case '!': addToken(match('=') ? BANG_EQUAL : BANG); break;
        case '=': addToken(match('=') ? EQUAL_EQUAL : EQUAL); break;
        case '<': addToken(match('=') ? LESS_EQUAL : LESS); break;
        case '>': addToken(match('=') ? GREATER_EQUAL : GREATER); break;

        case '/':
          if (match('/')) {
            comment();
          } else {
            addToken(SLASH);
          }
          break;

        case '.':
          if (isDigit(peek())) {
            number();
          } else {
            addToken(DOT);
          }
          break;

        case '"': string(); break;

        case ' ':
        case '\r':
        case '\t':
          // Ignore whitespace.
          break;

        case '\n':
          line++;
          break;

        default:
          if (isAlpha(c)) {
            identifier();
          } else if (isDigit(c)) {
            number();
          } else {
            errorReporter.error(line, "Unexpected character.");
          }
          break;
      }
    }

    tokens.add(new Token(EOF, "", null, line));
    return tokens;
  }

  private void comment() {
    // A comment goes until the end of the line.
    while (peek() != '\n' && !isAtEnd()) advance();
  }

  private void identifier() {
    while (isAlphaNumeric(peek())) advance();

    // See if the identifier is a reserved word.
    String text = source.substring(tokenStart, current);

    TokenType type = keywords.get(text);
    if (type == null) type = IDENTIFIER;

    addToken(type);
  }

  private void number() {
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peek(1))) {
      // Consume the "."
      advance();

      while (isDigit(peek())) advance();
    }

    double value = Double.parseDouble(
        source.substring(tokenStart, current));
    addToken(NUMBER, value);
  }

  private void string() {
    while (peek() != '"' && !isAtEnd()) advance();

    // Unterminated string.
    if (isAtEnd()) {
      errorReporter.error(line, "Unterminated string.");
      return;
    }

    // The closing ".
    advance();

    // Trim the surrounding quotes.
    String value = source.substring(tokenStart + 1, current - 1);
    addToken(STRING, value);
  }

  private void addToken(TokenType type) {
    addToken(type, null);
  }

  private void addToken(TokenType type, Object value) {
    String text = source.substring(tokenStart, current);
    tokens.add(new Token(type, text, value, line));
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
