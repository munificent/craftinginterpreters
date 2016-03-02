#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "scanner.h"
#include "vm.h"

// TODO: Grow dynamically.
#define MAX_CODE  1024

typedef struct {
  bool hadError;
  Token current;
  Token previous;
} Parser;

// A function being compiled.
typedef struct Function {
  struct Function* enclosing;
  uint8_t code[MAX_CODE];
  int codeSize;

  // TODO: Make this a root.
  ObjArray* constants;
  int numConstants;
} Function;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! - +
  PREC_CALL,        // . () []
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;

// The innermost function currently being compiled.
Function* function;

static void advance() {
  parser.previous = parser.current;
  parser.current = scanToken();
}

static void error(const char* message) {
  fprintf(stderr, "%s\n", message);
  parser.hadError = true;
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type != type) {
    error(message);
  }
  
  advance();
}

static void emitByte(uint8_t byte) {
  function->code[function->codeSize++] = byte;
}

static void emitByteOp(uint8_t op, uint8_t argument) {
  emitByte(op);
  emitByte(argument);
}

// Forward declarations since the grammar is recursive.
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence, bool canAssign);

static uint8_t addConstant(Value constant) {
  // TODO: Need to pin value.
  function->constants = ensureArraySize(function->constants,
                                        function->numConstants + 1);
  function->constants->elements[function->numConstants++] = constant;
  return (uint8_t)function->numConstants - 1;
  // TODO: check for overflow.
}

static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  uint8_t constant = addConstant((Value)newNumber(value));
  emitByteOp(OP_CONSTANT, constant);
}

static void infixOperator(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);

  // Compile the right-hand operand.
  parsePrecedence((Precedence)(rule->precedence + 1), false);

  // Emit the operator instruction.
  // TODO: Other operators.
  switch (operatorType) {
    case TOKEN_PLUS: emitByte(OP_ADD); break;
    case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
    default:
      assert(false); // Unreachable.
  }
}

ParseRule rules[] = {
  { NULL,   NULL,           PREC_NONE },   // TOKEN_LEFT_PAREN
  { NULL,   NULL,           PREC_NONE },   // TOKEN_RIGHT_PAREN
  { NULL,   NULL,           PREC_NONE },   // TOKEN_LEFT_BRACKET
  { NULL,   NULL,           PREC_NONE },   // TOKEN_RIGHT_BRACKET
  { NULL,   NULL,           PREC_NONE },   // TOKEN_LEFT_BRACE
  { NULL,   NULL,           PREC_NONE },   // TOKEN_RIGHT_BRACE
  { NULL,   NULL,           PREC_NONE },   // TOKEN_BANG
  { NULL,   NULL,           PREC_NONE },   // TOKEN_BANG_EQUAL
  { NULL,   NULL,           PREC_NONE },   // TOKEN_COMMA
  { NULL,   NULL,           PREC_NONE },   // TOKEN_DOT
  { NULL,   NULL,           PREC_NONE },   // TOKEN_EQUAL
  { NULL,   NULL,           PREC_NONE },   // TOKEN_EQUAL_EQUAL
  { NULL,   NULL,           PREC_NONE },   // TOKEN_GREATER
  { NULL,   NULL,           PREC_NONE },   // TOKEN_GREATER_EQUAL
  { NULL,   NULL,           PREC_NONE },   // TOKEN_LESS
  { NULL,   NULL,           PREC_NONE },   // TOKEN_LESS_EQUAL
  { NULL,   infixOperator,  PREC_TERM },   // TOKEN_MINUS
  { NULL,   infixOperator,  PREC_TERM },   // TOKEN_PLUS
  { NULL,   NULL,           PREC_NONE },   // TOKEN_SEMICOLON
  { NULL,   infixOperator,  PREC_FACTOR }, // TOKEN_SLASH
  { NULL,   infixOperator,  PREC_FACTOR }, // TOKEN_STAR

  { NULL,   NULL,           PREC_NONE },   // TOKEN_IDENTIFIER
  { NULL,   NULL,           PREC_NONE },   // TOKEN_STRING
  { number, NULL,           PREC_NONE },   // TOKEN_NUMBER

  { NULL,   NULL,           PREC_NONE },   // TOKEN_AND
  { NULL,   NULL,           PREC_NONE },   // TOKEN_CLASS
  { NULL,   NULL,           PREC_NONE },   // TOKEN_ELSE
  { NULL,   NULL,           PREC_NONE },   // TOKEN_FALSE
  { NULL,   NULL,           PREC_NONE },   // TOKEN_FUN
  { NULL,   NULL,           PREC_NONE },   // TOKEN_FOR
  { NULL,   NULL,           PREC_NONE },   // TOKEN_IF
  { NULL,   NULL,           PREC_NONE },   // TOKEN_NULL
  { NULL,   NULL,           PREC_NONE },   // TOKEN_OR
  { NULL,   NULL,           PREC_NONE },   // TOKEN_RETURN
  { NULL,   NULL,           PREC_NONE },   // TOKEN_THIS
  { NULL,   NULL,           PREC_NONE },   // TOKEN_TRUE
  { NULL,   NULL,           PREC_NONE },   // TOKEN_VAR
  { NULL,   NULL,           PREC_NONE },   // TOKEN_WHILE

  { NULL,   NULL,           PREC_NONE },   // TOKEN_ERROR
  { NULL,   NULL,           PREC_NONE },   // TOKEN_EOF
};

// TODO: Do we need canAssign, or does precedence cover it?
// Top-down operator precedence parser.
static void parsePrecedence(Precedence precedence, bool canAssign) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;

  if (prefixRule == NULL) {
    error("Expected expression.\n");
    return;
  }

  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

void expression() {
  parsePrecedence(PREC_ASSIGNMENT, true);
}

static void statement() {
  // TODO: Other statements.
  
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
}

ObjFunction* compile(const char* source) {
  initScanner(source);
  
  Function main;
  main.enclosing = NULL;
  main.codeSize = 0;
  main.constants = NULL;
  main.numConstants = 0;

  function = &main;
  
  // Prime the pump.
  parser.hadError = false;
  advance();

  // TODO: Should parse multiple statements.
  statement();

  emitByte(OP_RETURN);

  function = NULL;
  
  // If there was a compile error, the code is not valid, so don't create a
  // function.
  if (parser.hadError) return NULL;
  
  // TODO: Dropping numConstants is weird here. End up with an array that has
  // extra slots that we think are used.
  return newFunction(main.code, main.codeSize, main.constants);
}
