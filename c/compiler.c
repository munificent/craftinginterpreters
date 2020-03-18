//> Scanning on Demand compiler-c
#include <stdio.h>
//> Compiling Expressions compiler-include-stdlib
#include <stdlib.h>
//< Compiling Expressions compiler-include-stdlib
//> Local Variables compiler-include-string
#include <string.h>
//< Local Variables compiler-include-string

#include "common.h"
#include "compiler.h"
//> Garbage Collection compiler-include-memory
#include "memory.h"
//< Garbage Collection compiler-include-memory
#include "scanner.h"
//> Compiling Expressions include-debug

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif
//< Compiling Expressions include-debug
//> Compiling Expressions parser

typedef struct {
  Token current;
  Token previous;
//> had-error-field
  bool hadError;
//< had-error-field
//> panic-mode-field
  bool panicMode;
//< panic-mode-field
} Parser;
//> precedence

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;
//< precedence
//> parse-fn-type

//< parse-fn-type
/* Compiling Expressions parse-fn-type < Global Variables parse-fn-type
typedef void (*ParseFn)();
*/
//> Global Variables parse-fn-type
typedef void (*ParseFn)(bool canAssign);
//< Global Variables parse-fn-type
//> parse-rule

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;
//< parse-rule
//> Local Variables local-struct

typedef struct {
  Token name;
  int depth;
//> Closures is-captured-field
  bool isCaptured;
//< Closures is-captured-field
} Local;
//< Local Variables local-struct
//> Closures upvalue-struct
typedef struct {
  uint8_t index;
  bool isLocal;
} Upvalue;
//< Closures upvalue-struct
//> Calls and Functions function-type-enum
typedef enum {
  TYPE_FUNCTION,
//> Methods and Initializers initializer-type-enum
  TYPE_INITIALIZER,
//< Methods and Initializers initializer-type-enum
//> Methods and Initializers method-type-enum
  TYPE_METHOD,
//< Methods and Initializers method-type-enum
  TYPE_SCRIPT
} FunctionType;
//< Calls and Functions function-type-enum
//> Local Variables compiler-struct

typedef struct Compiler {
//> Calls and Functions enclosing-field
  struct Compiler* enclosing;
//< Calls and Functions enclosing-field
//> Calls and Functions function-fields
  ObjFunction* function;
  FunctionType type;

//< Calls and Functions function-fields
  Local locals[UINT8_COUNT];
  int localCount;
//> Closures upvalues-array
  Upvalue upvalues[UINT8_COUNT];
//< Closures upvalues-array
  int scopeDepth;
} Compiler;
//< Local Variables compiler-struct
//> Methods and Initializers class-compiler-struct

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
  Token name;
//> Superclasses has-superclass
  bool hasSuperclass;
//< Superclasses has-superclass
} ClassCompiler;
//< Methods and Initializers class-compiler-struct

Parser parser;

//< Compiling Expressions parser
//> Local Variables current-compiler
Compiler* current = NULL;
//< Local Variables current-compiler
//> Methods and Initializers current-class

ClassCompiler* currentClass = NULL;
//< Methods and Initializers current-class
//> Compiling Expressions compiling-chunk

/* Compiling Expressions compiling-chunk < Calls and Functions current-chunk
Chunk* compilingChunk;

static Chunk* currentChunk() {
  return compilingChunk;
}
*/
//> Calls and Functions current-chunk
static Chunk* currentChunk() {
  return &current->function->chunk;
}
//< Calls and Functions current-chunk

//< Compiling Expressions compiling-chunk
//> Compiling Expressions error-at
static void errorAt(Token* token, const char* message) {
//> check-panic-mode
  if (parser.panicMode) return;
//< check-panic-mode
//> set-panic-mode
  parser.panicMode = true;

//< set-panic-mode
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}
//< Compiling Expressions error-at
//> Compiling Expressions error
static void error(const char* message) {
  errorAt(&parser.previous, message);
}
//< Compiling Expressions error
//> Compiling Expressions error-at-current
static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}
//< Compiling Expressions error-at-current
//> Compiling Expressions advance

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) break;

    errorAtCurrent(parser.current.start);
  }
}
//< Compiling Expressions advance
//> Compiling Expressions consume
static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}
//< Compiling Expressions consume
//> Global Variables check
static bool check(TokenType type) {
  return parser.current.type == type;
}
//< Global Variables check
//> Global Variables match
static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}
//< Global Variables match
//> Compiling Expressions emit-byte
static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}
//< Compiling Expressions emit-byte
//> Compiling Expressions emit-bytes
static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}
//< Compiling Expressions emit-bytes
//> Jumping Back and Forth emit-loop
static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}
//< Jumping Back and Forth emit-loop
//> Jumping Back and Forth emit-jump
static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}
//< Jumping Back and Forth emit-jump
//> Compiling Expressions emit-return
static void emitReturn() {
/* Calls and Functions return-nil < Methods and Initializers return-this
  emitByte(OP_NIL);
*/
//> Methods and Initializers return-this
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }

//< Methods and Initializers return-this
  emitByte(OP_RETURN);
}
//< Compiling Expressions emit-return
//> Compiling Expressions make-constant
static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}
//< Compiling Expressions make-constant
//> Compiling Expressions emit-constant
static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}
//< Compiling Expressions emit-constant
//> Jumping Back and Forth patch-jump
static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}
//< Jumping Back and Forth patch-jump
//> Local Variables init-compiler
/* Local Variables init-compiler < Calls and Functions init-compiler
static void initCompiler(Compiler* compiler) {
*/
//> Calls and Functions init-compiler
static void initCompiler(Compiler* compiler, FunctionType type) {
//> store-enclosing
  compiler->enclosing = current;
//< store-enclosing
  compiler->function = NULL;
  compiler->type = type;
//< Calls and Functions init-compiler
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
//> Calls and Functions init-function
  compiler->function = newFunction();
//< Calls and Functions init-function
  current = compiler;
//> Calls and Functions init-function-name

  if (type != TYPE_SCRIPT) {
    current->function->name = copyString(parser.previous.start,
                                         parser.previous.length);
  }
//< Calls and Functions init-function-name
//> Calls and Functions init-function-slot

  Local* local = &current->locals[current->localCount++];
  local->depth = 0;
//> Closures init-zero-local-is-captured
  local->isCaptured = false;
//< Closures init-zero-local-is-captured
/* Calls and Functions init-function-slot < Methods and Initializers slot-zero
  local->name.start = "";
  local->name.length = 0;
*/
//> Methods and Initializers slot-zero
  if (type != TYPE_FUNCTION) {
    local->name.start = "this";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
//< Methods and Initializers slot-zero
//< Calls and Functions init-function-slot
}
//< Local Variables init-compiler
//> Compiling Expressions end-compiler
/* Compiling Expressions end-compiler < Calls and Functions end-compiler
static void endCompiler() {
*/
//> Calls and Functions end-compiler
static ObjFunction* endCompiler() {
//< Calls and Functions end-compiler
  emitReturn();
//> Calls and Functions end-function
  ObjFunction* function = current->function;

//< Calls and Functions end-function
//> dump-chunk
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
/* Compiling Expressions dump-chunk < Calls and Functions disassemble-end
    disassembleChunk(currentChunk(), "code");
*/
//> Calls and Functions disassemble-end
    disassembleChunk(currentChunk(),
        function->name != NULL ? function->name->chars : "<script>");
//< Calls and Functions disassemble-end
  }
#endif
//< dump-chunk
//> Calls and Functions return-function

//> restore-enclosing
  current = current->enclosing;
//< restore-enclosing
  return function;
//< Calls and Functions return-function
}
//< Compiling Expressions end-compiler
//> Local Variables begin-scope
static void beginScope() {
  current->scopeDepth++;
}
//< Local Variables begin-scope
//> Local Variables end-scope
static void endScope() {
  current->scopeDepth--;
//> pop-locals

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >
            current->scopeDepth) {
/* Local Variables pop-locals < Closures end-scope
    emitByte(OP_POP);
*/
//> Closures end-scope
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
//< Closures end-scope
    current->localCount--;
  }
//< pop-locals
}
//< Local Variables end-scope
//> Compiling Expressions forward-declarations

static void expression();
//> Global Variables forward-declarations
static void statement();
static void declaration();
//< Global Variables forward-declarations
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

//< Compiling Expressions forward-declarations
//> Global Variables identifier-constant
static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}
//< Global Variables identifier-constant
//> Local Variables identifiers-equal
static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}
//< Local Variables identifiers-equal
//> Local Variables resolve-local
static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
//> own-initializer-error
      if (local->depth == -1) {
        error("Cannot read local variable in its own initializer.");
      }
//< own-initializer-error
      return i;
    }
  }

  return -1;
}
//< Local Variables resolve-local
//> Closures add-upvalue
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
  int upvalueCount = compiler->function->upvalueCount;
//> existing-upvalue

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

//< existing-upvalue
//> too-many-upvalues
  if (upvalueCount == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }

//< too-many-upvalues
  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}
//< Closures add-upvalue
//> Closures resolve-upvalue
static int resolveUpvalue(Compiler* compiler, Token* name) {
  if (compiler->enclosing == NULL) return -1;

  int local = resolveLocal(compiler->enclosing, name);
  if (local != -1) {
//> mark-local-captured
    compiler->enclosing->locals[local].isCaptured = true;
//< mark-local-captured
    return addUpvalue(compiler, (uint8_t)local, true);
  }
//> resolve-upvalue-recurse

  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
  }
//< resolve-upvalue-recurse

  return -1;
}
//< Closures resolve-upvalue
//> Local Variables add-local
static void addLocal(Token name) {
//> too-many-locals
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

//< too-many-locals
  Local* local = &current->locals[current->localCount++];
  local->name = name;
/* Local Variables add-local < Local Variables declare-undefined
  local->depth = current->scopeDepth;
*/
//> declare-undefined
  local->depth = -1;
//< declare-undefined
//> Closures init-is-captured
  local->isCaptured = false;
//< Closures init-is-captured
}
//< Local Variables add-local
//> Local Variables declare-variable
static void declareVariable() {
  // Global variables are implicitly declared.
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;
//> existing-in-scope
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break; // [negative]
    }
    
    if (identifiersEqual(name, &local->name)) {
      error("Variable with this name already declared in this scope.");
    }
  }

//< existing-in-scope
  addLocal(*name);
}
//< Local Variables declare-variable
//> Global Variables parse-variable
static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
//> Local Variables parse-local

  declareVariable();
  if (current->scopeDepth > 0) return 0;

//< Local Variables parse-local
  return identifierConstant(&parser.previous);
}
//< Global Variables parse-variable
//> Local Variables mark-initialized
static void markInitialized() {
//> Calls and Functions check-depth
  if (current->scopeDepth == 0) return;
//< Calls and Functions check-depth
  current->locals[current->localCount - 1].depth =
      current->scopeDepth;
}
//< Local Variables mark-initialized
//> Global Variables define-variable
static void defineVariable(uint8_t global) {
//> Local Variables define-variable
  if (current->scopeDepth > 0) {
//> define-local
    markInitialized();
//< define-local
    return;
  }

//< Local Variables define-variable
  emitBytes(OP_DEFINE_GLOBAL, global);
}
//< Global Variables define-variable
//> Calls and Functions argument-list
static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
//> arg-limit

      if (argCount == 255) {
        error("Cannot have more than 255 arguments.");
      }
//< arg-limit
      argCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return argCount;
}
//< Calls and Functions argument-list
//> Jumping Back and Forth and
static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}
//< Jumping Back and Forth and
//> Compiling Expressions binary
/* Compiling Expressions binary < Global Variables binary
static void binary() {
*/
//> Global Variables binary
static void binary(bool canAssign) {
//< Global Variables binary
  // Remember the operator.
  TokenType operatorType = parser.previous.type;

  // Compile the right operand.
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  // Emit the operator instruction.
  switch (operatorType) {
//> Types of Values comparison-operators
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
    case TOKEN_GREATER:       emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
//< Types of Values comparison-operators
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default:
      return; // Unreachable.
  }
}
//< Compiling Expressions binary
//> Calls and Functions compile-call
static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}
//< Calls and Functions compile-call
//> Classes and Instances compile-dot
static void dot(bool canAssign) {
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
  uint8_t name = identifierConstant(&parser.previous);

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
//> Methods and Initializers parse-call
  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
//< Methods and Initializers parse-call
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}
//< Classes and Instances compile-dot
//> Types of Values parse-literal
/* Types of Values parse-literal < Global Variables parse-literal
static void literal() {
*/
//> Global Variables parse-literal
static void literal(bool canAssign) {
//< Global Variables parse-literal
  switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    default:
      return; // Unreachable.
  }
}
//< Types of Values parse-literal
//> Compiling Expressions grouping
/* Compiling Expressions grouping < Global Variables grouping
static void grouping() {
*/
//> Global Variables grouping
static void grouping(bool canAssign) {
//< Global Variables grouping
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}
//< Compiling Expressions grouping
/* Compiling Expressions number < Global Variables number
static void number() {
*/
//> Compiling Expressions number
//> Global Variables number
static void number(bool canAssign) {
//< Global Variables number
  double value = strtod(parser.previous.start, NULL);
/* Compiling Expressions number < Types of Values const-number-val
  emitConstant(value);
*/
//> Types of Values const-number-val
  emitConstant(NUMBER_VAL(value));
//< Types of Values const-number-val
}
//< Compiling Expressions number
//> Jumping Back and Forth or
static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}
//< Jumping Back and Forth or
/* Strings parse-string < Global Variables string
static void string() {
*/
//> Strings parse-string
//> Global Variables string
static void string(bool canAssign) {
//< Global Variables string
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}
//< Strings parse-string
/* Global Variables read-named-variable < Global Variables named-variable-signature
static void namedVariable(Token name) {
*/
//> Global Variables named-variable-signature
static void namedVariable(Token name, bool canAssign) {
//< Global Variables named-variable-signature
/* Global Variables read-named-variable < Local Variables named-local
  uint8_t arg = identifierConstant(&name);
*/
//> Global Variables read-named-variable
//> Local Variables named-local
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
//> Closures named-variable-upvalue
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
//< Closures named-variable-upvalue
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }
//< Local Variables named-local
/* Global Variables read-named-variable < Global Variables named-variable
  emitBytes(OP_GET_GLOBAL, arg);
*/
//> named-variable

/* Global Variables named-variable < Global Variables named-variable-can-assign
  if (match(TOKEN_EQUAL)) {
*/
//> named-variable-can-assign
  if (canAssign && match(TOKEN_EQUAL)) {
//< named-variable-can-assign
    expression();
/* Global Variables named-variable < Local Variables emit-set
    emitBytes(OP_SET_GLOBAL, arg);
*/
//> Local Variables emit-set
    emitBytes(setOp, (uint8_t)arg);
//< Local Variables emit-set
  } else {
/* Global Variables named-variable < Local Variables emit-get
    emitBytes(OP_GET_GLOBAL, arg);
*/
//> Local Variables emit-get
    emitBytes(getOp, (uint8_t)arg);
//< Local Variables emit-get
  }
//< named-variable
}
//< Global Variables read-named-variable
/* Global Variables variable-without-assign < Global Variables variable
static void variable() {
  namedVariable(parser.previous);
}
*/
//> Global Variables variable
static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}
//< Global Variables variable
//> Superclasses synthetic-token
static Token syntheticToken(const char* text) {
  Token token;
  token.start = text;
  token.length = (int)strlen(text);
  return token;
}
//< Superclasses synthetic-token
//> Superclasses super
static void super_(bool canAssign) {
//> super-errors
  if (currentClass == NULL) {
    error("Cannot use 'super' outside of a class.");
  } else if (!currentClass->hasSuperclass) {
    error("Cannot use 'super' in a class with no superclass.");
  }

//< super-errors
  consume(TOKEN_DOT, "Expect '.' after 'super'.");
  consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
  uint8_t name = identifierConstant(&parser.previous);
//> super-get
  
  namedVariable(syntheticToken("this"), false);
/* Superclasses super-get < Superclasses super-invoke
  namedVariable(syntheticToken("super"), false);
  emitBytes(OP_GET_SUPER, name);
*/
//< super-get
//> super-invoke
  if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_SUPER_INVOKE, name);
    emitByte(argCount);
  } else {
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_GET_SUPER, name);
  }
//< super-invoke
}
//< Superclasses super
//> Methods and Initializers this
static void this_(bool canAssign) {
//> this-outside-class
  if (currentClass == NULL) {
    error("Cannot use 'this' outside of a class.");
    return;
  }
//< this-outside-class
  variable(false);
} // [this]
//< Methods and Initializers this
//> Compiling Expressions unary
/* Compiling Expressions unary < Global Variables unary
static void unary() {
*/
//> Global Variables unary
static void unary(bool canAssign) {
//< Global Variables unary
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
/* Compiling Expressions unary < Compiling Expressions unary-operand
  expression();
*/
//> unary-operand
  parsePrecedence(PREC_UNARY);
//< unary-operand

  // Emit the operator instruction.
  switch (operatorType) {
//> Types of Values compile-not
    case TOKEN_BANG: emitByte(OP_NOT); break;
//< Types of Values compile-not
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default:
      return; // Unreachable.
  }
}
//< Compiling Expressions unary
//> Compiling Expressions rules
ParseRule rules[] = {
/* Compiling Expressions rules < Calls and Functions infix-left-paren
  { grouping, NULL,    PREC_NONE },       // TOKEN_LEFT_PAREN
*/
//> Calls and Functions infix-left-paren
  { grouping, call,    PREC_CALL },       // TOKEN_LEFT_PAREN
//< Calls and Functions infix-left-paren
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE [big]
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
/* Compiling Expressions rules < Classes and Instances table-dot
  { NULL,     NULL,    PREC_NONE },       // TOKEN_DOT
*/
//> Classes and Instances table-dot
  { NULL,     dot,     PREC_CALL },       // TOKEN_DOT
//< Classes and Instances table-dot
  { unary,    binary,  PREC_TERM },       // TOKEN_MINUS
  { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR
/* Compiling Expressions rules < Types of Values table-not
  { NULL,     NULL,    PREC_NONE },       // TOKEN_BANG
*/
//> Types of Values table-not
  { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
//< Types of Values table-not
/* Compiling Expressions rules < Types of Values table-equal
  { NULL,     NULL,    PREC_NONE },       // TOKEN_BANG_EQUAL
*/
//> Types of Values table-equal
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
//< Types of Values table-equal
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
/* Compiling Expressions rules < Types of Values table-comparisons
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL_EQUAL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_GREATER
  { NULL,     NULL,    PREC_NONE },       // TOKEN_GREATER_EQUAL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LESS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LESS_EQUAL
*/
//> Types of Values table-comparisons
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
//< Types of Values table-comparisons
/* Compiling Expressions rules < Global Variables table-identifier
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
*/
//> Global Variables table-identifier
  { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
//< Global Variables table-identifier
/* Compiling Expressions rules < Strings table-string
  { NULL,     NULL,    PREC_NONE },       // TOKEN_STRING
*/
//> Strings table-string
  { string,   NULL,    PREC_NONE },       // TOKEN_STRING
//< Strings table-string
  { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
/* Compiling Expressions rules < Jumping Back and Forth table-and
  { NULL,     NULL,    PREC_NONE },       // TOKEN_AND
*/
//> Jumping Back and Forth table-and
  { NULL,     and_,    PREC_AND },        // TOKEN_AND
//< Jumping Back and Forth table-and
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
/* Compiling Expressions rules < Types of Values table-false
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FALSE
*/
//> Types of Values table-false
  { literal,  NULL,    PREC_NONE },       // TOKEN_FALSE
//< Types of Values table-false
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
/* Compiling Expressions rules < Types of Values table-nil
  { NULL,     NULL,    PREC_NONE },       // TOKEN_NIL
*/
//> Types of Values table-nil
  { literal,  NULL,    PREC_NONE },       // TOKEN_NIL
//< Types of Values table-nil
/* Compiling Expressions rules < Jumping Back and Forth table-or
  { NULL,     NULL,    PREC_NONE },       // TOKEN_OR
*/
//> Jumping Back and Forth table-or
  { NULL,     or_,     PREC_OR },         // TOKEN_OR
//< Jumping Back and Forth table-or
  { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
/* Compiling Expressions rules < Superclasses table-super
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER
*/
//> Superclasses table-super
  { super_,   NULL,    PREC_NONE },       // TOKEN_SUPER
//< Superclasses table-super
/* Compiling Expressions rules < Methods and Initializers table-this
  { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
*/
//> Methods and Initializers table-this
  { this_,    NULL,    PREC_NONE },       // TOKEN_THIS
//< Methods and Initializers table-this
/* Compiling Expressions rules < Types of Values table-true
  { NULL,     NULL,    PREC_NONE },       // TOKEN_TRUE
*/
//> Types of Values table-true
  { literal,  NULL,    PREC_NONE },       // TOKEN_TRUE
//< Types of Values table-true
  { NULL,     NULL,    PREC_NONE },       // TOKEN_VAR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};
//< Compiling Expressions rules
//> Compiling Expressions parse-precedence
static void parsePrecedence(Precedence precedence) {
/* Compiling Expressions parse-precedence < Compiling Expressions precedence-body
  // What goes here?
*/
//> precedence-body
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

/* Compiling Expressions precedence-body < Global Variables prefix-rule
  prefixRule();
*/
//> Global Variables prefix-rule
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);
//< Global Variables prefix-rule
//> infix

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
/* Compiling Expressions infix < Global Variables infix-rule
    infixRule();
*/
//> Global Variables infix-rule
    infixRule(canAssign);
//< Global Variables infix-rule
  }
//> Global Variables invalid-assign

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
//< Global Variables invalid-assign
//< infix
//< precedence-body
}
//< Compiling Expressions parse-precedence
//> Compiling Expressions get-rule
static ParseRule* getRule(TokenType type) {
  return &rules[type];
}
//< Compiling Expressions get-rule
//> Compiling Expressions expression
static void expression() {
/* Compiling Expressions expression < Compiling Expressions expression-body
  // What goes here?
*/
//> expression-body
  parsePrecedence(PREC_ASSIGNMENT);
//< expression-body
}
//< Compiling Expressions expression
//> Local Variables block
static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}
//< Local Variables block
//> Calls and Functions compile-function
static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope(); // [no-end-scope]

  // Compile the parameter list.
  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
//> parameters
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Cannot have more than 255 parameters.");
      }
      
      uint8_t paramConstant = parseVariable("Expect parameter name.");
      defineVariable(paramConstant);
    } while (match(TOKEN_COMMA));
  }
//< parameters
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

  // The body.
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();

  // Create the function object.
  ObjFunction* function = endCompiler();
/* Calls and Functions compile-function < Closures emit-closure
  emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
*/
//> Closures emit-closure
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));
//< Closures emit-closure
//> Closures capture-upvalues

  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
//< Closures capture-upvalues
}
//< Calls and Functions compile-function
//> Methods and Initializers method
static void method() {
  consume(TOKEN_IDENTIFIER, "Expect method name.");
  uint8_t constant = identifierConstant(&parser.previous);
//> method-body

//< method-body
/* Methods and Initializers method-body < Methods and Initializers method-type
  FunctionType type = TYPE_FUNCTION;
*/
//> method-type
  FunctionType type = TYPE_METHOD;
//< method-type
//> initializer-name
  if (parser.previous.length == 4 &&
      memcmp(parser.previous.start, "init", 4) == 0) {
    type = TYPE_INITIALIZER;
  }
  
//< initializer-name
//> method-body
  function(type);
//< method-body
  emitBytes(OP_METHOD, constant);
}
//< Methods and Initializers method
//> Classes and Instances class-declaration
static void classDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
//> Methods and Initializers class-name
  Token className = parser.previous;
//< Methods and Initializers class-name
  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();

  emitBytes(OP_CLASS, nameConstant);
  defineVariable(nameConstant);

//> Methods and Initializers create-class-compiler
  ClassCompiler classCompiler;
  classCompiler.name = parser.previous;
//> Superclasses init-has-superclass
  classCompiler.hasSuperclass = false;
//< Superclasses init-has-superclass
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

//< Methods and Initializers create-class-compiler
//> Superclasses compile-superclass
  if (match(TOKEN_LESS)) {
    consume(TOKEN_IDENTIFIER, "Expect superclass name.");
    variable(false);
//> inherit-self

    if (identifiersEqual(&className, &parser.previous)) {
      error("A class cannot inherit from itself.");
    }

//< inherit-self
//> superclass-variable
    beginScope();
    addLocal(syntheticToken("super"));
    defineVariable(0);
    
//< superclass-variable
    namedVariable(className, false);
    emitByte(OP_INHERIT);
//> set-has-superclass
    classCompiler.hasSuperclass = true;
//< set-has-superclass
  }
  
//< Superclasses compile-superclass
//> Methods and Initializers load-class
  namedVariable(className, false);
//< Methods and Initializers load-class
  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
//> Methods and Initializers class-body
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }
//< Methods and Initializers class-body
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
//> Methods and Initializers pop-class
  emitByte(OP_POP);
//< Methods and Initializers pop-class
//> Superclasses end-superclass-scope

  if (classCompiler.hasSuperclass) {
    endScope();
  }
//< Superclasses end-superclass-scope
//> Methods and Initializers pop-enclosing

  currentClass = currentClass->enclosing;
//< Methods and Initializers pop-enclosing
}
//< Classes and Instances class-declaration
//> Calls and Functions fun-declaration
static void funDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}
//< Calls and Functions fun-declaration
//> Global Variables var-declaration
static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}
//< Global Variables var-declaration
//> Global Variables expression-statement
static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}
//< Global Variables expression-statement
//> Jumping Back and Forth for-statement
static void forStatement() {
//> for-begin-scope
  beginScope();

//< for-begin-scope
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
/* Jumping Back and Forth for-statement < Jumping Back and Forth for-initializer
  consume(TOKEN_SEMICOLON, "Expect ';'.");
*/
//> for-initializer
  if (match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }
//< for-initializer

  int loopStart = currentChunk()->count;

/* Jumping Back and Forth for-statement < Jumping Back and Forth for-exit
  consume(TOKEN_SEMICOLON, "Expect ';'.");
*/
//> for-exit
  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

//< for-exit
/* Jumping Back and Forth for-statement < Jumping Back and Forth for-increment
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
*/
//> for-increment
  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);

    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }
//< for-increment

  statement();

  emitLoop(loopStart);
//> exit-jump

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }
//< exit-jump
//> for-end-scope

  endScope();
//< for-end-scope
}
//< Jumping Back and Forth for-statement
//> Jumping Back and Forth if-statement
static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); // [paren]

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
//> pop-then
  emitByte(OP_POP);
//< pop-then
  statement();

//> jump-over-else
  int elseJump = emitJump(OP_JUMP);

//< jump-over-else
  patchJump(thenJump);
//> pop-end
  emitByte(OP_POP);
//< pop-end
//> compile-else

  if (match(TOKEN_ELSE)) statement();
//< compile-else
//> patch-else
  patchJump(elseJump);
//< patch-else
}
//< Jumping Back and Forth if-statement
//> Global Variables print-statement
static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}
//< Global Variables print-statement
//> Calls and Functions return-statement
static void returnStatement() {
//> return-from-script
  if (current->type == TYPE_SCRIPT) {
    error("Cannot return from top-level code.");
  }

//< return-from-script
  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
//> Methods and Initializers return-from-init
    if (current->type == TYPE_INITIALIZER) {
      error("Cannot return a value from an initializer.");
    }

//< Methods and Initializers return-from-init
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}
//< Calls and Functions return-statement
//> Jumping Back and Forth while-statement
static void whileStatement() {
//> loop-start
  int loopStart = currentChunk()->count;

//< loop-start
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  statement();
//> loop

  emitLoop(loopStart);
//< loop

  patchJump(exitJump);
  emitByte(OP_POP);
}
//< Jumping Back and Forth while-statement
//> Global Variables synchronize
static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;

    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default:
        // Do nothing.
        ;
    }

    advance();
  }
}
//< Global Variables synchronize
//> Global Variables declaration
static void declaration() {
//> Classes and Instances match-class
  if (match(TOKEN_CLASS)) {
    classDeclaration();
/* Calls and Functions match-fun < Classes and Instances match-class
  if (match(TOKEN_FUN)) {
*/
  } else if (match(TOKEN_FUN)) {
//< Classes and Instances match-class
//> Calls and Functions match-fun
    funDeclaration();
/* Global Variables match-var < Calls and Functions match-fun
  if (match(TOKEN_VAR)) {
*/
  } else if (match(TOKEN_VAR)) {
//< Calls and Functions match-fun
//> match-var
    varDeclaration();
  } else {
    statement();
  }
//< match-var
/* Global Variables declaration < Global Variables match-var
  statement();
*/
//> call-synchronize

  if (parser.panicMode) synchronize();
//< call-synchronize
}
//< Global Variables declaration
//> Global Variables statement
static void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
//> Jumping Back and Forth parse-for
  } else if (match(TOKEN_FOR)) {
    forStatement();
//< Jumping Back and Forth parse-for
//> Jumping Back and Forth parse-if
  } else if (match(TOKEN_IF)) {
    ifStatement();
//< Jumping Back and Forth parse-if
//> Calls and Functions match-return
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
//< Calls and Functions match-return
//> Jumping Back and Forth parse-while
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
//< Jumping Back and Forth parse-while
//> Local Variables parse-block
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
//< Local Variables parse-block
//> parse-expressions-statement
  } else {
    expressionStatement();
//< parse-expressions-statement
  }
}
//< Global Variables statement

/* Scanning on Demand compiler-c < Compiling Expressions compile-signature
void compile(const char* source) {
*/
/* Compiling Expressions compile-signature < Calls and Functions compile-signature
bool compile(const char* source, Chunk* chunk) {
*/
//> Calls and Functions compile-signature
ObjFunction* compile(const char* source) {
//< Calls and Functions compile-signature
  initScanner(source);
/* Scanning on Demand dump-tokens < Compiling Expressions compile-chunk
  int line = -1;
  for (;;) {
    Token token = scanToken();
    if (token.line != line) {
      printf("%4d ", token.line);
      line = token.line;
    } else {
      printf("   | ");
    }
    printf("%2d '%.*s'\n", token.type, token.length, token.start); // [format]

    if (token.type == TOKEN_EOF) break;
  }
*/
//> Local Variables compiler
  Compiler compiler;
//< Local Variables compiler
/* Local Variables compiler < Calls and Functions call-init-compiler
  initCompiler(&compiler);
*/
//> Calls and Functions call-init-compiler
  initCompiler(&compiler, TYPE_SCRIPT);
//< Calls and Functions call-init-compiler
/* Compiling Expressions init-compile-chunk < Calls and Functions call-init-compiler
  compilingChunk = chunk;
*/
//> Compiling Expressions compile-chunk
//> init-parser-error

  parser.hadError = false;
  parser.panicMode = false;

//< init-parser-error
  advance();
//< Compiling Expressions compile-chunk
/* Compiling Expressions compile-chunk < Global Variables compile
  expression();
  consume(TOKEN_EOF, "Expect end of expression.");
*/
//> Global Variables compile

  while (!match(TOKEN_EOF)) {
    declaration();
  }

//< Global Variables compile
/* Compiling Expressions finish-compile < Calls and Functions call-end-compiler
  endCompiler();
*/
/* Compiling Expressions return-had-error < Calls and Functions call-end-compiler
  return !parser.hadError;
*/
//> Calls and Functions call-end-compiler
  ObjFunction* function = endCompiler();
  return parser.hadError ? NULL : function;
//< Calls and Functions call-end-compiler
}
//> Garbage Collection mark-compiler-roots
void markCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    markObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
//< Garbage Collection mark-compiler-roots
