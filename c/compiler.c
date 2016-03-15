#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "object.h"
#include "scanner.h"
#include "string.h"
#include "vm.h"

#define MAX_LOCALS 256

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

typedef struct
{
  // The name of the local variable.
  Token name;
  
  // The depth in the scope chain that this variable was declared at. Zero is
  // the outermost scope--parameters for a method, or the first local block in
  // top level code. One is the scope within that, etc.
  int depth;
} Local;

typedef struct Compiler {
  // The compiler for the enclosing function, if any.
  struct Compiler* enclosing;

  // The function being compiled.
  ObjFunction* function;
  
  // The currently in scope local variables.
  Local locals[MAX_LOCALS];
  
  // The number of local variables currently in scope.
  int numLocals;
  
  // The current level of block scope nesting. Zero is the outermost local
  // scope. -1 is global scope.
  int scopeDepth;
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

static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;
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

// Emits [instruction] followed by a placeholder for a jump offset. The
// placeholder can be patched by calling [jumpPatch]. Returns the index of the
// placeholder.
static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return compiler->function->codeCount - 2;
}

// Replaces the placeholder argument for a previous CODE_JUMP or CODE_JUMP_IF
// instruction with an offset that jumps to the current end of bytecode.
static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = compiler->function->codeCount - offset - 2;
  
  // TODO: Do this.
  //if (jump > MAX_JUMP) error(compiler, "Too much code to jump over.");
  
  compiler->function->code[offset] = (jump >> 8) & 0xff;
  compiler->function->code[offset + 1] = jump & 0xff;
}

static void beginScope() {
  compiler->scopeDepth++;
}

static void endScope() {
  compiler->scopeDepth--;
  
  while (compiler->numLocals > 0 &&
         compiler->locals[compiler->numLocals - 1].depth > compiler->scopeDepth) {
    // TODO: Close upvalues.
    emitByte(OP_POP);
    compiler->numLocals--;
  }
}

// Forward declarations since the grammar is recursive.
static void expression();
static void statement();
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

static void and_(bool canAssign) {
  // left operand...
  // OP_JUMP_IF       ------.
  // OP_POP // left operand |
  // right operand...       |
  //   <--------------------'
  // ...
  
  // Short circuit if the left operand is false.
  int endJump = emitJump(OP_JUMP_IF_FALSE);
  
  // Compile the right operand.
  emitByte(OP_POP); // Left operand.
  parsePrecedence(PREC_AND);
  
  patchJump(endJump);
}

static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);

  // Compile the right-hand operand.
  parsePrecedence((Precedence)(rule->precedence + 1));

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
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

static void null_(bool canAssign) {
  emitByte(OP_NULL);
}

static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  uint8_t constant = allocateConstant();
  compiler->function->constants.values[constant] = (Value)newNumber(value);

  emitBytes(OP_CONSTANT, constant);
}

static void or_(bool canAssign) {
  // left operand...
  // OP_JUMP_IF       ---.
  // OP_JUMP          ---+--.
  //   <-----------------'  |
  // OP_POP // left operand |
  // right operand...       |
  //   <--------------------'
  // ...
  
  // If the operand is *true* we want to keep it, so when it's false, jump to
  // the code to evaluate the right operand.
  int elseJump = emitJump(OP_JUMP_IF_FALSE);

  // If we get here, the operand is true, so jump to the end to keep it.
  int endJump = emitJump(OP_JUMP);
  
  // Compile the right operand.
  patchJump(elseJump);
  emitByte(OP_POP); // Left operand.
  
  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void string(bool canAssign) {
  uint8_t constant = allocateConstant();
  ObjString* string = newString((uint8_t*)parser.previous.start + 1,
                                parser.previous.length - 2);
  compiler->function->constants.values[constant] = (Value)string;
  
  emitBytes(OP_CONSTANT, constant);
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

static int resolveLocal(Token* name) {
  for (int i = compiler->numLocals - 1; i >= 0; i--) {
    if (compiler->locals[i].name.length == name->length &&
        memcmp(compiler->locals[i].name.start, name->start, name->length) == 0)
    {
      return i;
    }
  }

  return -1;
}

static void variable(bool canAssign) {
  // Look it up in the local scopes. Look in reverse order so that the most
  // nested variable is found first and shadows outer ones.
  // TODO: Simplify code.
  int local = resolveLocal(&parser.previous);
  if (local != -1) {
    if (canAssign && match(TOKEN_EQUAL)) {
      expression();
      emitBytes(OP_SET_LOCAL, (uint8_t)local);
    } else {
      emitBytes(OP_GET_LOCAL, (uint8_t)local);
    }
  } else {
    uint8_t constant = nameConstant();
    
    if (canAssign && match(TOKEN_EQUAL)) {
      expression();
      emitBytes(OP_SET_GLOBAL, constant);
    } else {
      emitBytes(OP_GET_GLOBAL, constant);
    }
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
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
  { NULL,     NULL,    PREC_NONE },       // TOKEN_DOT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
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

  { NULL,     and_,    PREC_AND },        // TOKEN_AND
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
  { boolean,  NULL,    PREC_NONE },       // TOKEN_FALSE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
  { null_,    NULL,    PREC_NONE },       // TOKEN_NULL
  { NULL,     or_,     PREC_OR },         // TOKEN_OR
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

static void block() {
  consume(TOKEN_LEFT_BRACE, "Expect '{' before block.");

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    statement();
  }
  
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
  
  beginScope();
  
  // Jump to the else branch if the condition is false.
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  
  // Compile the then branch.
  emitByte(OP_POP); // Condition.
  statement(compiler);
  
  // Jump over the else branch when the if branch is taken.
  int endJump = emitJump(OP_JUMP);
  
  // Compile the else branch.
  patchJump(elseJump);
  emitByte(OP_POP); // Condition.
  
  if (match(TOKEN_ELSE)) statement();
  
  patchJump(endJump);
  endScope();
}

static void varStatement() {
  consume(TOKEN_IDENTIFIER, "Expect variable name.");
  Token name = parser.previous;
  uint8_t constant = nameConstant();
  
  // Compile the initializer.
  consume(TOKEN_EQUAL, "Expect '=' after variable name.");
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after initializer.");
  
  if (compiler->scopeDepth == -1) {
    emitBytes(OP_DEFINE_GLOBAL, constant);
  } else {
    // TODO: Error if already declared in current scope.
    Local* local = &compiler->locals[compiler->numLocals];
    local->name = name;
    local->depth = compiler->scopeDepth;
    // TODO: Check for overflow.
    compiler->numLocals++;
  }
}

static void statement() {
  if (match(TOKEN_IF)) {
    ifStatement();
    return;
  }
  
  if (match(TOKEN_VAR)) {
    varStatement();
    return;
  }
  
  // TODO: Other statements.

  if (check(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
    return;
  }
  
  expression();
  emitByte(OP_POP);
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
}

ObjFunction* compile(const char* source) {
  initScanner(source);

  Compiler mainCompiler;
  mainCompiler.enclosing = NULL;
  mainCompiler.function = NULL;
  mainCompiler.numLocals = 0;
  mainCompiler.scopeDepth = -1;

  compiler = &mainCompiler;

  mainCompiler.function = newFunction();

  // Prime the pump.
  parser.hadError = false;
  advance();

  if (!match(TOKEN_EOF)) {
    do {
      statement();
    } while (!match(TOKEN_EOF));
  }

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
