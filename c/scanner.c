//>= Scanning on Demand 99
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
  const char* name;
  size_t      length;
  TokenType   type;
} Keyword;

// The table of reserved words and their associated token types.
static Keyword keywords[] = {
  {"and",     3, TOKEN_AND},
  {"class",   5, TOKEN_CLASS},
  {"else",    4, TOKEN_ELSE},
  {"false",   5, TOKEN_FALSE},
  {"for",     3, TOKEN_FOR},
  {"fun",     3, TOKEN_FUN},
  {"if",      2, TOKEN_IF},
  {"nil",     3, TOKEN_NIL},
  {"or",      2, TOKEN_OR},
  {"print",   5, TOKEN_PRINT},
  {"return",  6, TOKEN_RETURN},
  {"super" ,  5, TOKEN_SUPER},
  {"this",    4, TOKEN_THIS},
  {"true",    4, TOKEN_TRUE},
  {"var",     3, TOKEN_VAR},
  {"while",   5, TOKEN_WHILE},
  // Sentinel to mark the end of the array.
  {NULL,      0, TOKEN_EOF}
};

typedef struct {
  const char* source;
  const char* tokenStart;
  const char* current;
  int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
  scanner.source = source;
  scanner.tokenStart = source;
  scanner.current = source;
  scanner.line = 1;
}

// Returns true if `c` is an English letter or underscore.
static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

// Returns true if `c` is a digit.
static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

// Returns true if `c` is an English letter, underscore, or digit.
static bool isAlphaNumeric(char c) {
  return isAlpha(c) || isDigit(c);
}

static bool isAtEnd() {
  return *scanner.current == '\0';
}

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

static char peek() {
  return *scanner.current;
}

static char peekNext() {
  if (isAtEnd()) return '\0';
  return scanner.current[1];
}

static bool match(char expected) {
  if (isAtEnd()) return false;
  if (*scanner.current != expected) return false;

  scanner.current++;
  return true;
}

static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.tokenStart;
  token.length = (int)(scanner.current - scanner.tokenStart);
  token.line = scanner.line;

  return token;
}

static Token errorToken(const char* message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;

  return token;
}

static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;

      case '\n':
        scanner.line++;
        advance();
        break;

      case '/':
        if (peekNext() == '/') {
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

static Token identifier() {
  while (isAlphaNumeric(peek())) advance();

  TokenType type = TOKEN_IDENTIFIER;

  // See if the identifier is a reserved word.
  size_t length = scanner.current - scanner.tokenStart;
  for (Keyword* keyword = keywords; keyword->name != NULL; keyword++) {
    if (length == keyword->length &&
        memcmp(scanner.tokenStart, keyword->name, length) == 0) {
      type = keyword->type;
      break;
    }
  }

  return makeToken(type);
}

static Token number() {
  while (isDigit(peek())) advance();

  // Look for a fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    // Consume the "."
    advance();

    while (isDigit(peek())) advance();
  }

  return makeToken(TOKEN_NUMBER);
}

static Token string() {
  while (peek() != '"' && !isAtEnd()) advance();

  // Unterminated string.
  if (isAtEnd()) return errorToken("Unterminated string.");

  // The closing ".
  advance();
  return makeToken(TOKEN_STRING);
}

Token scanToken() {
  skipWhitespace();

  // The next token starts with the current character.
  scanner.tokenStart = scanner.current;

  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();

  if (isAlpha(c)) return identifier();
  if (isDigit(c)) return number();

  switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case '+': return makeToken(TOKEN_PLUS);
    case '-': return makeToken(TOKEN_MINUS);
    case '*': return makeToken(TOKEN_STAR);
    case '/': return makeToken(TOKEN_SLASH);
    case '!':
      if (match('=')) return makeToken(TOKEN_BANG_EQUAL);
      return makeToken(TOKEN_BANG);

    case '.':
      if (isDigit(peek())) return number();
      return makeToken(TOKEN_DOT);

    case '=':
      if (match('=')) return makeToken(TOKEN_EQUAL_EQUAL);
      return makeToken(TOKEN_EQUAL);

    case '<':
      if (match('=')) return makeToken(TOKEN_LESS_EQUAL);
      return makeToken(TOKEN_LESS);

    case '>':
      if (match('=')) return makeToken(TOKEN_GREATER_EQUAL);
      return makeToken(TOKEN_GREATER);

    case '"': return string();
  }

  return errorToken("Unexpected character.");
}
