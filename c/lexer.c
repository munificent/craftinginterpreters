#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

typedef struct
{
  const char* name;
  size_t      length;
  TokenType   type;
} Keyword;

// The table of reserved words and their associated token types.
static Keyword keywords[] =
{
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

void initLexer(Lexer* lexer, const char* source) {
  lexer->source = source;
  lexer->tokenStart = source;
  lexer->current = source;
  lexer->line = 1;
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

static bool isAtEnd(Lexer* lexer) {
  return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
  lexer->current++;
  return lexer->current[-1];
}

static char peek(Lexer* lexer) {
  return *lexer->current;
}

static char peekNext(Lexer* lexer) {
  if (isAtEnd(lexer)) return '\0';
  return lexer->current[1];
}

static bool match(Lexer* lexer, char expected) {
  if (isAtEnd(lexer)) return false;
  if (*lexer->current != expected) return false;
  
  lexer->current++;
  return true;
}

// TODO: Take value?
static Token makeToken(Lexer* lexer, TokenType type) {
  Token token;
  // TODO: Use struct initializer?
  token.type = type;
  token.start = lexer->tokenStart;
  token.length = (int)(lexer->current - lexer->tokenStart);
  token.line = lexer->line;
  
  return token;
}

static void skipWhitespace(Lexer* lexer) {
  while (true) {
    char c = peek(lexer);
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance(lexer);
        break;
        
      case '\n':
        lexer->line++;
        advance(lexer);
        break;
        
      case '/':
        if (peekNext(lexer) == '/') {
          // A comment goes until the end of the line.
          while (peek(lexer) != '\n' && !isAtEnd(lexer)) advance(lexer);
        }
        break;
        
      default:
        return;
    }
  }
}

static Token identifier(Lexer* lexer) {
  while (isAlphaNumeric(peek(lexer))) advance(lexer);
  
  TokenType type = TOKEN_IDENTIFIER;

  // See if the identifier is a reserved word.
  size_t length = lexer->current - lexer->tokenStart;
  for (Keyword* keyword = keywords; keyword->name != NULL; keyword++)
  {
    if (length == keyword->length &&
        memcmp(lexer->tokenStart, keyword->name, length) == 0)
    {
      type = keyword->type;
      break;
    }
  }
  
  return makeToken(lexer, type);
}

static Token number(Lexer* lexer) {
  while (isDigit(peek(lexer))) advance(lexer);
  
  // Look for a fractional part.
  if (peek(lexer) == '.' && isDigit(peekNext(lexer))) {
    // Consume the "."
    advance(lexer);
    
    while (isDigit(peek(lexer))) advance(lexer);
  }
  
  // double value = Double.parseDouble(source.substring(tokenStart, current));
  return makeToken(lexer, TOKEN_NUMBER);
}

static Token string(Lexer* lexer) {
  // TODO: Escapes.
  // TODO: What about newlines?
  while (peek(lexer) != '"' && !isAtEnd(lexer)) advance(lexer);
  
  // Unterminated string.
  // TODO: Include error message. Maybe use special type?
  if (isAtEnd(lexer)) return makeToken(lexer, TOKEN_ERROR);
  
  // The closing ".
  advance(lexer);
  return makeToken(lexer, TOKEN_STRING);
}

Token nextToken(Lexer* lexer) {
  skipWhitespace(lexer);
  
  // The next token starts with the current character.
  lexer->tokenStart = lexer->current;

  if (isAtEnd(lexer)) return makeToken(lexer, TOKEN_EOF);

  char c = advance(lexer);

  if (isAlpha(c)) return identifier(lexer);
  if (isDigit(c)) return number(lexer);
  
  switch (c) {
    case '(': return makeToken(lexer, TOKEN_LEFT_PAREN);
    case ')': return makeToken(lexer, TOKEN_RIGHT_PAREN);
    case '[': return makeToken(lexer, TOKEN_LEFT_BRACKET);
    case ']': return makeToken(lexer, TOKEN_RIGHT_BRACKET);
    case '{': return makeToken(lexer, TOKEN_LEFT_BRACE);
    case '}': return makeToken(lexer, TOKEN_RIGHT_BRACE);
    case ';': return makeToken(lexer, TOKEN_SEMICOLON);
    case ',': return makeToken(lexer, TOKEN_COMMA);
    case '+': return makeToken(lexer, TOKEN_PLUS);
    case '-': return makeToken(lexer, TOKEN_MINUS);
    case '*': return makeToken(lexer, TOKEN_STAR);
    case '/': return makeToken(lexer, TOKEN_SLASH);
    case '%': return makeToken(lexer, TOKEN_PERCENT);
    case '!':
      if (match(lexer, '=')) return makeToken(lexer, TOKEN_BANG_EQUAL);
      return makeToken(lexer, TOKEN_BANG);
      
    case '.':
      if (isDigit(peek(lexer))) return number(lexer);
      return makeToken(lexer, TOKEN_DOT);
      
    case '=':
      if (match(lexer, '=')) return makeToken(lexer, TOKEN_EQUAL_EQUAL);
      return makeToken(lexer, TOKEN_EQUAL);
      
    case '<':
      if (match(lexer, '=')) return makeToken(lexer, TOKEN_LESS_EQUAL);
      return makeToken(lexer, TOKEN_LESS);
      
    case '>':
      if (match(lexer, '=')) return makeToken(lexer, TOKEN_GREATER_EQUAL);
      return makeToken(lexer, TOKEN_GREATER);

    case '"': return string(lexer);
  }

  // TODO: Tests for this. (Can use "|" or "&".)
  return makeToken(lexer, TOKEN_ERROR);
}
