#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "object.h"
#include "scanner.h"
#include "vm.h"

typedef struct {
  bool hadError;
  Token current;
  Token previous;
} Parser;

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

typedef struct Compiler {
  // The compiler for the enclosing function, if any.
  struct Compiler* enclosing;

  // The function being compiled.
  ObjFunction* function;
} Compiler;

Parser parser;

// The compiler for the innermost function currently being compiled.
Compiler* compiler;

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

static bool match(TokenType type) {
  if (parser.current.type != type) return false;
  advance();
  return true;
}

static void emitByte(uint8_t byte) {
  ObjFunction* function = compiler->function;
  // TODO: allocateConstant() has almost the exact same code. Reuse somehow.
  if (function->codeCapacity < function->codeCount + 1) {
    if (function->codeCapacity == 0) {
      function->codeCapacity = 4;
    } else {
      function->codeCapacity *= 2;
    }

    function->code = reallocate(function->code,
                                sizeof(uint8_t) * function->codeCapacity);
  }

  function->code[function->codeCount++] = byte;
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

// TODO: Remove?
//static void emitShort(uint16_t value) {
//  emitByte((value >> 8) & 0xff);
//  emitByte(value & 0xff);
//}

// Forward declarations since the grammar is recursive.
static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t allocateConstant() {
  ObjFunction* function = compiler->function;
  ensureArrayCapacity(&function->constants);
  
  // Make sure allocating the constant later doesn't see an uninitialized
  // slot.
  // TODO: Do something cleaner. Pin the constant?
  function->constants.values[function->constants.count] = NULL;
  return (uint8_t)function->constants.count++;
  // TODO: check for overflow.
}

// Creates a string constant for the previous identifier token. Returns the
// index of the constant.
static uint8_t nameConstant() {
  uint8_t constant = allocateConstant();
  ObjString* name = newString((uint8_t*)parser.previous.start,
                              parser.previous.length);
  compiler->function->constants.values[constant] = (Value)name;
  return constant;
}

static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);

  // Compile the right-hand operand.
  parsePrecedence((Precedence)(rule->precedence + 1));

  // Emit the operator instruction.
  // TODO: Other operators.
  switch (operatorType) {
    case TOKEN_GREATER:       emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default:
      assert(false); // Unreachable.
  }
}

static void boolean(bool canAssign) {
  uint8_t constant = allocateConstant();
  compiler->function->constants.values[constant] =
      (Value)newBool(parser.previous.type == TOKEN_TRUE);

  emitBytes(OP_CONSTANT, constant);
}

static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  
  // Compile the operand.
  parsePrecedence((Precedence)(PREC_UNARY + 1));

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_BANG: emitByte(OP_NOT); break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default:
      assert(false); // Unreachable.
  }
}

static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  uint8_t constant = allocateConstant();
  compiler->function->constants.values[constant] = (Value)newNumber(value);

  emitBytes(OP_CONSTANT, constant);
}

static void string(bool canAssign) {
  uint8_t constant = allocateConstant();
  ObjString* string = newString((uint8_t*)parser.previous.start + 1,
                                parser.previous.length - 2);
  compiler->function->constants.values[constant] = (Value)string;
  
  emitBytes(OP_CONSTANT, constant);
}

static void variable(bool canAssign) {
  uint8_t constant = nameConstant();
  
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_ASSIGN_GLOBAL, constant);
  } else {
    emitBytes(OP_GET_GLOBAL, constant);
  }
}

ParseRule rules[] = {
  { grouping, NULL,    PREC_NONE },       // TOKEN_LEFT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACKET
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACKET
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
  { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     NULL,    PREC_NONE },       // TOKEN_BANG_EQUAL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
  { NULL,     NULL,    PREC_NONE },       // TOKEN_DOT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
  { unary,    binary,  PREC_TERM },       // TOKEN_MINUS
  { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR

  { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
  { string,   NULL,    PREC_NONE },       // TOKEN_STRING
  { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER

  { NULL,     NULL,    PREC_NONE },       // TOKEN_AND
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
  { boolean,  NULL,    PREC_NONE },       // TOKEN_FALSE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
  { NULL,     NULL,    PREC_NONE },       // TOKEN_NULL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_OR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
  { boolean,  NULL,    PREC_NONE },       // TOKEN_TRUE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_VAR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE

  { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};

// Top-down operator precedence parser.
static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;

  if (prefixRule == NULL) {
    error("Expected expression.\n");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
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
  parsePrecedence(PREC_ASSIGNMENT);
}

static void statement() {
  if (match(TOKEN_VAR)) {
    consume(TOKEN_IDENTIFIER, "Expect variable name.");
    uint8_t constant = nameConstant();

    // Compile the initializer.
    consume(TOKEN_EQUAL, "Expect '=' after variable name.");
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after initializer.");

    emitBytes(OP_DEFINE_GLOBAL, constant);
    return;
  }
  
  // TODO: Other statements.

  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
}

ObjFunction* compile(const char* source) {
  initScanner(source);

  Compiler mainCompiler;
  mainCompiler.enclosing = NULL;
  mainCompiler.function = NULL;

  compiler = &mainCompiler;

  mainCompiler.function = newFunction();

  // Prime the pump.
  parser.hadError = false;
  advance();

  do {
    statement();
  } while (!match(TOKEN_EOF));

  emitByte(OP_RETURN);

  compiler = NULL;

  // If there was a compile error, the code is not valid, so don't create a
  // function.
  if (parser.hadError) return NULL;

  return mainCompiler.function;
}

void grayCompilerRoots() {
  Compiler* thisCompiler = compiler;
  while (thisCompiler != NULL) {
    grayValue((Value)thisCompiler->function);
    thisCompiler = thisCompiler->enclosing;
  }
}
