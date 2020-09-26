//> Scanning scanner-class
package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.craftinginterpreters.lox.TokenType.*; // [static-import]

class Scanner {
//> keyword-map
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
//< keyword-map
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
      // We are at the beginning of the next lexeme.
      start = current;
      scanToken();
    }

    tokens.add(new Token(EOF, "", null, line));
    return tokens;
  }
//< scan-tokens
//> scan-token
  private void scanToken() {
    char c = advance();
    switch (c) {
      case '(': addToken(LEFT_PAREN); break;
      case ')': addToken(RIGHT_PAREN); break;
      case '{': addToken(LEFT_BRACE); break;
      case '}': addToken(RIGHT_BRACE); break;
      case ',': addToken(COMMA); break;
      case '.': addToken(DOT); break;
      case '-': addToken(MINUS); break;
      case '+': addToken(PLUS); break;
      case ';': addToken(SEMICOLON); break;
      case '*': addToken(STAR); break; // [slash]
//> two-char-tokens
      case '!':
        addToken(match('=') ? BANG_EQUAL : BANG);
        break;
      case '=':
        addToken(match('=') ? EQUAL_EQUAL : EQUAL);
        break;
      case '<':
        addToken(match('=') ? LESS_EQUAL : LESS);
        break;
      case '>':
        addToken(match('=') ? GREATER_EQUAL : GREATER);
        break;
//< two-char-tokens
//> slash
      case '/':
        if (match('/')) {
          // A comment goes until the end of the line.
          while (peek() != '\n' && !isAtEnd()) advance();
        } else {
          addToken(SLASH);
        }
        break;
//< slash
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
/* Scanning char-error < Scanning digit-start
        Lox.error(line, "Unexpected character.");
*/
//> digit-start
        if (isDigit(c)) {
          number();
//> identifier-start
        } else if (isAlpha(c)) {
          identifier();
//< identifier-start
        } else {
          Lox.error(line, "Unexpected character.");
        }
//< digit-start
        break;
//< char-error
    }
  }
//< scan-token
//> identifier
  private void identifier() {
    while (isAlphaNumeric(peek())) advance();

/* Scanning identifier < Scanning keyword-type
    addToken(IDENTIFIER);
*/
//> keyword-type
    String text = source.substring(start, current);
    TokenType type = keywords.get(text);
    if (type == null) type = IDENTIFIER;
    addToken(type);
//< keyword-type
  }
//< identifier
//> number
  private void number() {
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
      // Consume the "."
      advance();

      while (isDigit(peek())) advance();
    }

    addToken(NUMBER,
        Double.parseDouble(source.substring(start, current)));
  }
//< number
//> string
  private void string() {
    while (peek() != '"' && !isAtEnd()) {
      if (peek() == '\n') line++;
      advance();
    }

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
    if (isAtEnd()) return '\0';
    return source.charAt(current);
  }
//< peek
//> peek-next
  private char peekNext() {
    if (current + 1 >= source.length()) return '\0';
    return source.charAt(current + 1);
  } // [peek-next]
//< peek-next
//> is-alpha
  private boolean isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
  }

  private boolean isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
  }
//< is-alpha
//> is-digit
  private boolean isDigit(char c) {
    return c >= '0' && c <= '9';
  } // [is-digit]
//< is-digit
//> is-at-end
  private boolean isAtEnd() {
    return current >= source.length();
  }
//< is-at-end
//> advance-and-add-token
  private char advance() {
    current++;
    return source.charAt(current - 1);
  }

  private void addToken(TokenType type) {
    addToken(type, null);
  }

  private void addToken(TokenType type, Object literal) {
    String text = source.substring(start, current);
    tokens.add(new Token(type, text, literal, line));
  }
//< advance-and-add-token
}
