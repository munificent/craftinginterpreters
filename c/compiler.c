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

static void emitByteOp(uint8_t op, uint8_t argument) {
  emitByte(op);
  emitByte(argument);
}

// Forward declarations since the grammar is recursive.
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence, bool canAssign);

static uint8_t allocateConstant() {
  ObjFunction* function = compiler->function;
  if (function->constantCapacity < function->constantCount + 1) {
    if (function->constantCapacity == 0) {
      function->constantCapacity = 4;
    } else {
      function->constantCapacity *= 2;
    }

    function->constants = reallocate(function->constants,
                                     sizeof(Value) * function->constantCapacity);
  }

  // Make sure allocating the constant later doesn't see an uninitialized
  // slot.
  // TODO: Do something cleaner. Pin the constant?
  function->constants[function->constantCount] = NULL;
  return (uint8_t)function->constantCount++;
  // TODO: check for overflow.
}

static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  uint8_t constant = allocateConstant();
  compiler->function->constants[constant] = (Value)newNumber(value);

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
  
  Compiler mainCompiler;
  mainCompiler.enclosing = NULL;
  mainCompiler.function = NULL;
  
  compiler = &mainCompiler;

  mainCompiler.function = newFunction();
  
  // Prime the pump.
  parser.hadError = false;
  advance();

  // TODO: Should parse multiple statements.
  statement();

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
