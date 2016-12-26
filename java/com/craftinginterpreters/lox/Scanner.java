//> Scanning scanner-class
package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.craftinginterpreters.lox.TokenType.*;

class Scanner {
//> not-yet
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

//< not-yet
  private final String source;
  private final List<Token> tokens = new ArrayList<>();
//> scan-state
  private int start = 0;
  private int current = 0;
  private int line = 1;
//< scan-state

  Scanner(String source) {
    this.source = source;
  }
//> scan-tokens
  List<Token> scanTokens() {
    while (!isAtEnd()) {
      // The next token starts with the current character.
      start = current;

      char c = advance();
      switch (c) {
        case '(': addToken(LEFT_PAREN); break;
        case ')': addToken(RIGHT_PAREN); break;
//> single-char-tokens
        case '{': addToken(LEFT_BRACE); break;
        case '}': addToken(RIGHT_BRACE); break;
        case ';': addToken(SEMICOLON); break;
        case ',': addToken(COMMA); break;
        case '+': addToken(PLUS); break;
        case '-': addToken(MINUS); break;
        case '*': addToken(STAR); break;
//< single-char-tokens
//> two-char-tokens
        case '!': addToken(match('=') ? BANG_EQUAL : BANG); break;
        case '=': addToken(match('=') ? EQUAL_EQUAL : EQUAL); break;
        case '<': addToken(match('=') ? LESS_EQUAL : LESS); break;
        case '>': addToken(match('=') ? GREATER_EQUAL : GREATER); break;
//< two-char-tokens
//> slash
        case '/':
          if (match('/')) {
            comment();
          } else {
            addToken(SLASH);
          }
          break;
//< slash
//> not-yet
        case '.':
          if (isDigit(peek())) {
            number();
          } else {
            addToken(DOT);
          }
          break;
//< not-yet
//> whitespace

        case ' ':
        case '\r':
        case '\t':
          // Ignore whitespace.
          break;

        case '\n':
          line++;
          break;
//< whitespace
//> string-start

        case '"': string(); break;
//< string-start
//> char-error

        default:
/* Scanning char-error < Scanning not-yet
          Lox.error(line, "Unexpected character.");
*/
//> not-yet
          if (isAlpha(c)) {
            identifier();
          } else if (isDigit(c)) {
            number();
          } else {
            Lox.error(line, "Unexpected character.");
          }
//< not-yet
          break;
//< char-error
      }
    }

    tokens.add(new Token(EOF, "", null, line));
    return tokens;
  }
//< scan-tokens
//> comment
  private void comment() {
    // A comment goes until the end of the line.
    while (peek() != '\n' && !isAtEnd()) advance();
  }
//< comment
//> not-yet
  private void identifier() {
    while (isAlphaNumeric(peek())) advance();

    // See if the identifier is a reserved word.
    String text = source.substring(start, current);

    TokenType type = keywords.get(text);
    if (type == null) type = IDENTIFIER;

    addToken(type);
  }

  private void number() {
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
      // Consume the "."
      advance();

      while (isDigit(peek())) advance();
    }

    double value = Double.parseDouble(
        source.substring(start, current));
    addToken(NUMBER, value);
  }
//< not-yet
//> string
  private void string() {
    while (peek() != '"' && !isAtEnd()) {
      if (peek() == '\n') {
        Lox.error(line, "String literals may not contain newlines.");
      }

      advance();
    }

    // Unterminated string.
    if (isAtEnd()) {
      Lox.error(line, "Unterminated string.");
      return;
    }

    // The closing ".
    advance();

    // Trim the surrounding quotes.
    String value = source.substring(start + 1, current - 1);
    addToken(STRING, value);
  }
//< string
//> match
  private boolean match(char expected) {
    if (isAtEnd()) return false;
    if (source.charAt(current) != expected) return false;

    current++;
    return true;
  }
//< match
//> peek
  private char peek() {
    if (current >= source.length()) return '\0';
    return source.charAt(current);
  }
//< peek
//> not-yet
  private char peekNext() {
    if (current + 1 >= source.length()) return '\0';
    return source.charAt(current + 1);
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
//< not-yet
//> is-at-end-and-advance
  private boolean isAtEnd() {
    return current >= source.length();
  }

  private char advance() {
    current++;
    return source.charAt(current - 1);
  }
//< is-at-end-and-advance
//> add-token
  private void addToken(TokenType type) {
    addToken(type, null);
  }

  private void addToken(TokenType type, Object value) {
    String text = source.substring(start, current);
    tokens.add(new Token(type, text, value, line));
  }
//< add-token
}
