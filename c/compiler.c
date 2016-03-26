#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "scanner.h"
#include "vm.h"

//#define DEBUG_PRINT_CODE

// TODO: These are kind of pointless. Unify?
#define MAX_LOCALS 256
#define MAX_UPVALUES 256

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

typedef struct {
  // The name of the local variable.
  Token name;
  
  // The depth in the scope chain that this variable was declared at. Zero is
  // the outermost scope--parameters for a method, or the first local block in
  // top level code. One is the scope within that, etc.
  int depth;
  
  // True if this local variable is captured as an upvalue by a function.
  bool isUpvalue;
} Local;

typedef struct {
  // The index of the local variable or upvalue being captured from the
  // enclosing function.
  uint8_t index;
  
  // Whether the captured variable is a local or upvalue in the enclosing
  // function.
  bool isLocal;
} Upvalue;

typedef struct Compiler {
  // The compiler for the enclosing function, if any.
  struct Compiler* enclosing;

  // The function being compiled.
  ObjFunction* function;
  
  // The currently in scope local variables.
  Local locals[MAX_LOCALS];
  
  // The number of local variables currently in scope.
  int localCount;
  
  Upvalue upvalues[MAX_UPVALUES];
  
  int upvalueCount;
  
  // The current level of block scope nesting. Zero is the outermost local
  // scope. -1 is global scope.
  int scopeDepth;
} Compiler;

Parser parser;

// The compiler for the innermost function currently being compiled.
Compiler* current = NULL;

static void advance() {
  parser.previous = parser.current;
  parser.current = scanToken();
}

static void errorAt(int line, const char* message) {
  fprintf(stderr, "[line %d] Error: %s\n", line, message);
  parser.hadError = true;
}

static void error(const char* message) {
  errorAt(parser.previous.line, message);
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type != type) error(message);
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
  ObjFunction* function = current->function;
  // TODO: allocateConstant() has almost the exact same code. Reuse somehow.
  if (function->codeCapacity < function->codeCount + 1) {
    if (function->codeCapacity == 0) {
      function->codeCapacity = 4;
    } else {
      function->codeCapacity *= 2;
    }

    function->code = reallocate(function->code,
                                sizeof(uint8_t) * function->codeCapacity);
    function->codeLines = reallocate(function->codeLines,
                                      sizeof(int) * function->codeCapacity);
  }

  function->code[function->codeCount] = byte;
  function->codeLines[function->codeCount++] = parser.previous.line;
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);
  
  // TODO: Check for overflow.
  int offset = current->function->codeCount - loopStart + 2;
  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

// Emits [instruction] followed by a placeholder for a jump offset. The
// placeholder can be patched by calling [jumpPatch]. Returns the index of the
// placeholder.
static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return current->function->codeCount - 2;
}

// Replaces the placeholder argument for a previous CODE_JUMP or CODE_JUMP_IF
// instruction with an offset that jumps to the current end of bytecode.
static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = current->function->codeCount - offset - 2;
  
  // TODO: Do this.
  //if (jump > MAX_JUMP) error(compiler, "Too much code to jump over.");
  
  current->function->code[offset] = (jump >> 8) & 0xff;
  current->function->code[offset + 1] = jump & 0xff;
}

static void beginCompiler(Compiler* compiler) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->localCount = 0;
  compiler->upvalueCount = 0;
  compiler->scopeDepth = -1;
  
  current = compiler;
  
  compiler->function = newFunction();
}

static ObjFunction* endCompiler() {
  emitBytes(OP_NULL, OP_RETURN);
  
  ObjFunction* function = current->function;
  current = current->enclosing;

#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) printFunction(function);
#endif
  
  return function;
}

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;
  
  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
    if (current->locals[current->localCount - 1].isUpvalue) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

// Forward declarations since the grammar is recursive.
static void expression();
static void statement();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t addConstant(Value value) {
  // Make sure the value doesn't get collected when resizing the array.
  push(value);
  ObjFunction* function = current->function;
  ensureArrayCapacity(&function->constants);
  
  function->constants.values[function->constants.count] = pop();
  return (uint8_t)function->constants.count++;
  // TODO: check for overflow.
}

// Creates a string constant for the previous identifier token. Returns the
// index of the constant.
static uint8_t nameConstant() {
  return addConstant((Value)newString((uint8_t*)parser.previous.start,
                                      parser.previous.length));
}

static void emitConstant(Value value) {
  uint8_t constant = addConstant(value);
  emitBytes(OP_CONSTANT, constant);
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
  emitConstant((Value)newBool(parser.previous.type == TOKEN_TRUE));
}

static void call(bool canAssign) {
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      argCount++;
      // TODO: Check for overflow.
    } while (match(TOKEN_COMMA));
  }
  
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  emitByte(OP_CALL_0 + argCount);
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
  // TODO: Handle error.
  emitConstant((Value)newNumber(value));
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
  emitConstant((Value)newString((uint8_t*)parser.previous.start + 1,
                                parser.previous.length - 2));
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

static int resolveLocal(Compiler* compiler, Token* name) {
  // Look it up in the local scopes. Look in reverse order so that the most
  // nested variable is found first and shadows outer ones.
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    if (compiler->locals[i].name.length == name->length &&
        memcmp(compiler->locals[i].name.start, name->start, name->length) == 0)
    {
      return i;
    }
  }
  
  return -1;
}

// Adds an upvalue to [compiler]'s function with the given properties. Does not
// add one if an upvalue for that variable is already in the list. Returns the
// index of the upvalue.
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
  // Look for an existing one.
  for (int i = 0; i < compiler->function->upvalueCount; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) return i;
  }
  
  // If we got here, it's a new upvalue.
  compiler->upvalues[compiler->function->upvalueCount].isLocal = isLocal;
  compiler->upvalues[compiler->function->upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}

// Attempts to look up [name] in the functions enclosing the one being compiled
// by [compiler]. If found, it adds an upvalue for it to this compiler's list
// of upvalues (unless it's already in there) and returns its index. If not
// found, returns -1.
//
// If the name is found outside of the immediately enclosing function, this
// will flatten the closure and add upvalues to all of the intermediate
// functions so that it gets walked down to this one.
static int resolveUpvalue(Compiler* compiler, Token* name) {
  // If we are at the top level, we didn't find it.
  if (compiler->enclosing == NULL) return -1;
  
  // See if it's a local variable in the immediately enclosing function.
  int local = resolveLocal(compiler->enclosing, name);
  if (local != -1) {
    // Mark the local as an upvalue so we know to close it when it goes out of
    // scope.
    compiler->enclosing->locals[local].isUpvalue = true;
    return addUpvalue(compiler, (uint8_t)local, true);
  }
  
  // See if it's an upvalue in the immediately enclosing function. In other
  // words, if it's a local variable in a non-immediately enclosing function.
  // This "flattens" closures automatically: it adds upvalues to all of the
  // intermediate functions to get from the function where a local is declared
  // all the way into the possibly deeply nested function that is closing over
  // it.
  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
  }
  
  // If we got here, we walked all the way up the parent chain and couldn't
  // find it.
  return -1;
}

static Token parseVariable(const char* error, uint8_t* constant) {
  consume(TOKEN_IDENTIFIER, error);
  Token name = parser.previous;
  if (current->scopeDepth == -1) {
    *constant = nameConstant();
  }
  return name;
}

static void declareVariable(Token* name, uint8_t constant) {
  if (current->scopeDepth == -1) {
    emitBytes(OP_DEFINE_GLOBAL, constant);
    return;
  }
  
  // See if a local variable with this name is already declared in this scope.
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth < current->scopeDepth) break;
    if (name->length == local->name.length &&
        memcmp(name->start, local->name.start, name->length) == 0) {
      // TODO: Having to report error at explicit line means errors could be
      // reported out of order. Another option is to declare the local in
      // parseVariable(), but that means putting it in a half-declared state,
      // so that it can't be accessed in its own initializer. Ruby and JS do
      // something odd here, where a variable can be accessed in its own
      // initializer and doing so implicitly nulls it out (!).
      errorAt(name->line,
              "Variable with this name already declared in this scope.");
    }
  }
  
  Local* local = &current->locals[current->localCount];
  local->name = *name;
  local->depth = current->scopeDepth;
  local->isUpvalue = false;
  // TODO: Check for overflow.
  current->localCount++;
}

static void variable(bool canAssign) {
  // TODO: Simplify code.
  uint8_t getOp, setOp;
  
  int arg = resolveLocal(current, &parser.previous);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &parser.previous)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    arg = nameConstant();
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

ParseRule rules[] = {
  { grouping, call,    PREC_CALL },       // TOKEN_LEFT_PAREN
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

static void funStatement() {
  uint8_t nameConstant = 0xff;
  Token name = parseVariable("Expect function name.", &nameConstant);

  Compiler functionCompiler;
  beginCompiler(&functionCompiler);
  beginScope();
  
  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      uint8_t paramConstant;
      Token param = parseVariable("Expect parameter name.", &paramConstant);
      declareVariable(&param, paramConstant);
      current->function->arity++;
      // TODO: Check for overflow.
    } while (match(TOKEN_COMMA));
  }
  
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

  block();

  endScope();
  ObjFunction* function = endCompiler();
  
  // TODO: Always create closure?
  if (function->upvalueCount == 0) {
    emitConstant((Value)function);
  } else {
    // Capture the upvalues in the new closure object.
    uint8_t constant = addConstant((Value)function);
    emitBytes(OP_CLOSURE, constant);
    
    // Emit arguments for each upvalue to know whether to capture a local or
    // an upvalue.
    for (int i = 0; i < function->upvalueCount; i++) {
      emitByte(functionCompiler.upvalues[i].isLocal ? 1 : 0);
      emitByte(functionCompiler.upvalues[i].index);
    }
  }
  
  // TODO: Closure stuff to capture frame.
  
  declareVariable(&name, nameConstant);
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
  statement();
  
  // Jump over the else branch when the if branch is taken.
  int endJump = emitJump(OP_JUMP);
  
  // Compile the else branch.
  patchJump(elseJump);
  emitByte(OP_POP); // Condition.
  
  if (match(TOKEN_ELSE)) statement();
  
  patchJump(endJump);
  endScope();
}

static void returnStatement() {
  if (match(TOKEN_SEMICOLON)) {
    emitByte(OP_NULL);
  } else {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
  }
  
  emitByte(OP_RETURN);
}

static void varStatement() {
  uint8_t constant = 0xff;
  Token name = parseVariable("Expect variable name.", &constant);
  
  // Compile the initializer.
  consume(TOKEN_EQUAL, "Expect '=' after variable name.");
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after initializer.");
  
  declareVariable(&name, constant);
}

static void whileStatement() {
  int loopStart = current->function->codeCount;
  
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
  
  beginScope();
  
  // Jump out of the loop if the condition is false.
  int exitJump = emitJump(OP_JUMP_IF_FALSE);

  // Compile the body.
  emitByte(OP_POP); // Condition.
  statement();

  // Loop back to the start.
  emitLoop(loopStart);
  
  patchJump(exitJump);
  endScope();
}

static void statement() {
  if (match(TOKEN_FUN)) {
    funStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
  } else if (match(TOKEN_VAR)) {
    varStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else if (check(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expression();
    emitByte(OP_POP);
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  }
  
  // TODO: Other statements.
}

ObjFunction* compile(const char* source) {
  initScanner(source);

  Compiler mainCompiler;
  beginCompiler(&mainCompiler);

  // Prime the pump.
  parser.hadError = false;
  advance();

  if (!match(TOKEN_EOF)) {
    do {
      statement();
    } while (!match(TOKEN_EOF));
  }

  ObjFunction* function = endCompiler();
  
  // If there was a compile error, the code is not valid, so don't create a
  // function.
  return parser.hadError ? NULL : function;
}

void grayCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    grayValue((Value)compiler->function);
    compiler = compiler->enclosing;
  }
}
