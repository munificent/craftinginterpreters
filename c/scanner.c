#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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
  {"null",    4, TOKEN_NULL},
  {"or",      2, TOKEN_OR},
  {"return",  6, TOKEN_RETURN},
  {"true",    4, TOKEN_TRUE},
  {"var",     3, TOKEN_VAR},
  {"while",   5, TOKEN_WHILE},
  // Sentinel to mark the end of the array.
  {NULL,      0, TOKEN_EOF}
};

void initScanner(Scanner* scanner, const char* source) {
  scanner->source = source;
  scanner->tokenStart = source;
  scanner->current = source;
  scanner->line = 1;
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

static bool isAtEnd(Scanner* scanner) {
  return *scanner->current == '\0';
}

static char advance(Scanner* scanner) {
  scanner->current++;
  return scanner->current[-1];
}

static char peek(Scanner* scanner) {
  return *scanner->current;
}

static char peekNext(Scanner* scanner) {
  if (isAtEnd(scanner)) return '\0';
  return scanner->current[1];
}

static bool match(Scanner* scanner, char expected) {
  if (isAtEnd(scanner)) return false;
  if (*scanner->current != expected) return false;

  scanner->current++;
  return true;
}

// TODO: Take value?
static Token makeToken(Scanner* scanner, TokenType type) {
  Token token;
  // TODO: Use struct initializer?
  token.type = type;
  token.start = scanner->tokenStart;
  token.length = (int)(scanner->current - scanner->tokenStart);
  token.line = scanner->line;

  return token;
}

static void skipWhitespace(Scanner* scanner) {
  while (true) {
    char c = peek(scanner);
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance(scanner);
        break;

      case '\n':
        scanner->line++;
        advance(scanner);
        break;

      case '/':
        if (peekNext(scanner) == '/') {
          // A comment goes until the end of the line.
          while (peek(scanner) != '\n' && !isAtEnd(scanner)) advance(scanner);
        }
        break;

      default:
        return;
    }
  }
}

static Token identifier(Scanner* scanner) {
  while (isAlphaNumeric(peek(scanner))) advance(scanner);

  TokenType type = TOKEN_IDENTIFIER;

  // See if the identifier is a reserved word.
  size_t length = scanner->current - scanner->tokenStart;
  for (Keyword* keyword = keywords; keyword->name != NULL; keyword++) {
    if (length == keyword->length &&
        memcmp(scanner->tokenStart, keyword->name, length) == 0) {
      type = keyword->type;
      break;
    }
  }

  return makeToken(scanner, type);
}

static Token number(Scanner* scanner) {
  while (isDigit(peek(scanner))) advance(scanner);

  // Look for a fractional part.
  if (peek(scanner) == '.' && isDigit(peekNext(scanner))) {
    // Consume the "."
    advance(scanner);

    while (isDigit(peek(scanner))) advance(scanner);
  }

  // double value = Double.parseDouble(source.substring(tokenStart, current));
  return makeToken(scanner, TOKEN_NUMBER);
}

static Token string(Scanner* scanner) {
  // TODO: Escapes.
  // TODO: What about newlines?
  while (peek(scanner) != '"' && !isAtEnd(scanner)) advance(scanner);

  // Unterminated string.
  // TODO: Include error message. Maybe use special type?
  if (isAtEnd(scanner)) return makeToken(scanner, TOKEN_ERROR);

  // The closing ".
  advance(scanner);
  return makeToken(scanner, TOKEN_STRING);
}

Token nextToken(Scanner* scanner) {
  skipWhitespace(scanner);

  // The next token starts with the current character.
  scanner->tokenStart = scanner->current;

  if (isAtEnd(scanner)) return makeToken(scanner, TOKEN_EOF);

  char c = advance(scanner);

  if (isAlpha(c)) return identifier(scanner);
  if (isDigit(c)) return number(scanner);

  switch (c) {
    case '(': return makeToken(scanner, TOKEN_LEFT_PAREN);
    case ')': return makeToken(scanner, TOKEN_RIGHT_PAREN);
    case '[': return makeToken(scanner, TOKEN_LEFT_BRACKET);
    case ']': return makeToken(scanner, TOKEN_RIGHT_BRACKET);
    case '{': return makeToken(scanner, TOKEN_LEFT_BRACE);
    case '}': return makeToken(scanner, TOKEN_RIGHT_BRACE);
    case ';': return makeToken(scanner, TOKEN_SEMICOLON);
    case ',': return makeToken(scanner, TOKEN_COMMA);
    case '+': return makeToken(scanner, TOKEN_PLUS);
    case '-': return makeToken(scanner, TOKEN_MINUS);
    case '*': return makeToken(scanner, TOKEN_STAR);
    case '/': return makeToken(scanner, TOKEN_SLASH);
    case '%': return makeToken(scanner, TOKEN_PERCENT);
    case '!':
      if (match(scanner, '=')) return makeToken(scanner, TOKEN_BANG_EQUAL);
      return makeToken(scanner, TOKEN_BANG);

    case '.':
      if (isDigit(peek(scanner))) return number(scanner);
      return makeToken(scanner, TOKEN_DOT);

    case '=':
      if (match(scanner, '=')) return makeToken(scanner, TOKEN_EQUAL_EQUAL);
      return makeToken(scanner, TOKEN_EQUAL);

    case '<':
      if (match(scanner, '=')) return makeToken(scanner, TOKEN_LESS_EQUAL);
      return makeToken(scanner, TOKEN_LESS);

    case '>':
      if (match(scanner, '=')) return makeToken(scanner, TOKEN_GREATER_EQUAL);
      return makeToken(scanner, TOKEN_GREATER);

    case '"': return string(scanner);
  }

  // TODO: Tests for this. (Can use "|" or "&".)
  return makeToken(scanner, TOKEN_ERROR);
}
