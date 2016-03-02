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
  Token current;
  Token previous;
} Parser;

typedef struct Compiler {
  struct Compiler* parent;
  uint8_t code[MAX_CODE];
  int codeSize;

  // TODO: Make this a root.
  ObjArray* constants;
  int numConstants;
} Compiler;

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

typedef void (*ParseFn)(Compiler*, bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;

static void advance(Compiler* compiler) {
  parser.previous = parser.current;
  parser.current = scannerNext();
}

static void emitByte(Compiler* compiler, uint8_t byte) {
  compiler->code[compiler->codeSize++] = byte;
}

static void emitByteOp(Compiler* compiler, uint8_t op, uint8_t argument) {
  emitByte(compiler, op);
  emitByte(compiler, argument);
}

// Forward declarations since the grammar is recursive.
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Compiler* compiler, Precedence precedence,
                            bool canAssign);

static uint8_t addConstant(Compiler* compiler, Value constant) {
  // TODO: Need to pin value.
  compiler->constants = ensureArraySize(compiler->constants,
                                        compiler->numConstants + 1);
  compiler->constants->elements[compiler->numConstants++] = constant;
  return compiler->numConstants - 1;
  // TODO: check for overflow.
}

static void number(Compiler* compiler, bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  uint8_t constant = addConstant(compiler, (Value)newNumber(value));
  emitByteOp(compiler, OP_CONSTANT, constant);
}

static void infixOperator(Compiler* compiler, bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);

  // Compile the right-hand operand.
  parsePrecedence(compiler, (Precedence)(rule->precedence + 1), false);

  // Emit the operator instruction.
  // TODO: Other operators.
  switch (operatorType) {
    case TOKEN_PLUS: emitByte(compiler, OP_ADD); break;
    case TOKEN_MINUS: emitByte(compiler, OP_SUBTRACT); break;
    case TOKEN_STAR: emitByte(compiler, OP_MULTIPLY); break;
    case TOKEN_SLASH: emitByte(compiler, OP_DIVIDE); break;
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
static void parsePrecedence(Compiler* compiler, Precedence precedence,
                            bool canAssign) {
  advance(compiler);
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;

  if (prefixRule == NULL) {
    // TODO: Compile error.
    printf("Expected expression.\n");
    return;
  }

  prefixRule(compiler, canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance(compiler);
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(compiler, canAssign);
  }
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

void expression(Compiler* compiler) {
  parsePrecedence(compiler, PREC_ASSIGNMENT, true);
}

ObjFunction* compile(const char* source) {
  scannerInit(source);

  Compiler compiler;
  compiler.parent = NULL;
  compiler.codeSize = 0;
  compiler.constants = NULL;
  compiler.numConstants = 0;

  // Prime the pump.
  advance(&compiler);

  // TODO: Should parse statements.
  expression(&compiler);

  emitByte(&compiler, OP_RETURN);

  // TODO: Dropping numConstants is weird here. End up with an array that has
  // extra slots that we think are used.
  return newFunction(compiler.code, compiler.codeSize, compiler.constants);
}
