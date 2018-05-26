//> Scanning on Demand compiler-c
#include <stdio.h>
//> Compiling Expressions compiler-include-stdlib
#include <stdlib.h>
//< Compiling Expressions compiler-include-stdlib
//> Local Variables not-yet
#include <string.h>
//< Local Variables not-yet

#include "common.h"
#include "compiler.h"
//> Garbage Collection not-yet
#include "memory.h"
//< Garbage Collection not-yet
#include "scanner.h"
//> Compiling Expressions include-debug

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif
//< Compiling Expressions include-debug
//> Compiling Expressions parser

typedef struct {
  bool hadError;
  bool panicMode;
  Token current;
  Token previous;
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
  PREC_UNARY,       // ! - +
  PREC_CALL,        // . () []
  PREC_PRIMARY
} Precedence;
//< precedence
/* Compiling Expressions parse-fn-type < Global Variables not-yet

typedef void (*ParseFn)();
*/
//> Global Variables not-yet
typedef void (*ParseFn)(bool canAssign);
//< Global Variables not-yet
//> parse-rule

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;
//< parse-rule
//> Local Variables not-yet

typedef struct {
  // The name of the local variable.
  Token name;

  // The depth in the scope chain that this variable was declared at. Zero is
  // the outermost scope--parameters for a method, or the first local block in
  // top level code. One is the scope within that, etc.
  int depth;
//> Closures not-yet

  // True if this local variable is captured as an upvalue by a function.
  bool isUpvalue;
//< Closures not-yet
} Local;
//> Closures not-yet

typedef struct {
  // The index of the local variable or upvalue being captured from the
  // enclosing function.
  uint8_t index;

  // Whether the captured variable is a local or upvalue in the enclosing
  // function.
  bool isLocal;
} Upvalue;
//< Closures not-yet
//> Calls and Functions not-yet

typedef enum {
  TYPE_FUNCTION,
//> Methods and Initializers not-yet
  TYPE_INITIALIZER,
  TYPE_METHOD,
//< Methods and Initializers not-yet
  TYPE_TOP_LEVEL
} FunctionType;
//< Calls and Functions not-yet

typedef struct Compiler {
//> Calls and Functions not-yet
  // The compiler for the enclosing function, if any.
  struct Compiler* enclosing;

  // The function being compiled.
  ObjFunction* function;
  FunctionType type;

//< Calls and Functions not-yet
  // The currently in scope local variables.
  Local locals[UINT8_COUNT];

  // The number of local variables currently in scope.
  int localCount;
//> Closures not-yet
  Upvalue upvalues[UINT8_COUNT];
//< Closures not-yet

  // The current level of block scope nesting. Zero is the outermost local
  // scope. 0 is global scope.
  int scopeDepth;
} Compiler;
//< Local Variables not-yet
//> Methods and Initializers not-yet

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;

  Token name;
//> Superclasses not-yet
  bool hasSuperclass;
//< Superclasses not-yet
} ClassCompiler;
//< Methods and Initializers not-yet

Parser parser;
//< Compiling Expressions parser
//> Local Variables not-yet

Compiler* current = NULL;
//< Local Variables not-yet
//> Methods and Initializers not-yet

ClassCompiler* currentClass = NULL;
//< Methods and Initializers not-yet
/* Compiling Expressions compiling-chunk < Calls and Functions not-yet

Chunk* compilingChunk;

static Chunk* currentChunk() {
  return compilingChunk;
}

*/
//> Calls and Functions not-yet

static Chunk* currentChunk() {
  return &current->function->chunk;
}
//< Calls and Functions not-yet
//> Compiling Expressions error-at
static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;

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
//> Global Variables not-yet

static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}
//< Global Variables not-yet
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
//> Jumping Forward and Back not-yet

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

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
  return currentChunk()->count - 2;
}
//< Jumping Forward and Back not-yet
//> Compiling Expressions emit-return
static void emitReturn() {
/* Calls and Functions not-yet < Methods and Initializers not-yet
  emitByte(OP_NIL);
*/
//> Methods and Initializers not-yet
  // An initializer automatically returns "this".
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }

//< Methods and Initializers not-yet
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
//> Jumping Forward and Back not-yet
// Replaces the placeholder argument for a previous CODE_JUMP or CODE_JUMP_IF
// instruction with an offset that jumps to the current end of bytecode.
static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}
//< Jumping Forward and Back not-yet
//> Local Variables not-yet
/* Local Variables not-yet < Calls and Functions not-yet
static void initCompiler(Compiler* compiler) {
*/
//> Calls and Functions not-yet
static void initCompiler(Compiler* compiler, int scopeDepth,
                         FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
//< Calls and Functions not-yet
  compiler->localCount = 0;
/* Local Variables not-yet < Calls and Functions not-yet
  compiler->scopeDepth = 0;
*/
//> Calls and Functions not-yet
  compiler->scopeDepth = scopeDepth;
  compiler->function = newFunction();
//< Calls and Functions not-yet
  current = compiler;
//> Calls and Functions not-yet

  switch (type) {
    case TYPE_FUNCTION:
      current->function->name = copyString(parser.previous.start,
                                           parser.previous.length);
      break;

//> Methods and Initializers not-yet
    case TYPE_INITIALIZER:
    case TYPE_METHOD: {
      int length = currentClass->name.length + parser.previous.length + 1;

      char* chars = ALLOCATE(char, length + 1);
      memcpy(chars, currentClass->name.start, currentClass->name.length);
      chars[currentClass->name.length] = '.';
      memcpy(chars + currentClass->name.length + 1, parser.previous.start,
             parser.previous.length);
      chars[length] = '\0';

      current->function->name = takeString(chars, length);
      break;
    }
//< Methods and Initializers not-yet
    case TYPE_TOP_LEVEL:
      current->function->name = NULL;
      break;
  }

  // The first slot is always implicitly declared.
  Local* local = &current->locals[current->localCount++];
  local->depth = current->scopeDepth;
//> Closures not-yet
  local->isUpvalue = false;
//< Closures not-yet
/* Calls and Functions not-yet < Methods and Initializers not-yet
  local->name.start = "";
  local->name.length = 0;
*/
//> Methods and Initializers not-yet
  if (type != TYPE_FUNCTION) {
    // In a method, it holds the receiver, "this".
    local->name.start = "this";
    local->name.length = 4;
  } else {
    // In a function, it holds the function, but cannot be referenced, so has
    // no name.
    local->name.start = "";
    local->name.length = 0;
  }
//< Methods and Initializers not-yet
//< Calls and Functions not-yet
}
//< Local Variables not-yet
//> Compiling Expressions end-compiler
/* Compiling Expressions end-compiler < Calls and Functions not-yet
static void endCompiler() {
*/
//> Calls and Functions not-yet
static ObjFunction* endCompiler() {
//< Calls and Functions not-yet
  emitReturn();
//> Calls and Functions not-yet

  ObjFunction* function = current->function;
//< Calls and Functions not-yet
//> dump-chunk
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
/* Compiling Expressions dump-chunk < Calls and Functions not-yet
    disassembleChunk(currentChunk(), "code");
*/
//> Calls and Functions not-yet
    disassembleChunk(currentChunk(),
        function->name != NULL ? function->name->chars : "<top>");
//< Calls and Functions not-yet
  }
#endif
//< dump-chunk
//> Calls and Functions not-yet
  current = current->enclosing;

  return function;
//< Calls and Functions not-yet
}
//< Compiling Expressions end-compiler
//> Local Variables not-yet

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
/* Local Variables not-yet < Closures not-yet
    emitByte(OP_POP);
*/
//> Closures not-yet
    if (current->locals[current->localCount - 1].isUpvalue) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
//< Closures not-yet
    current->localCount--;
  }
}
//< Local Variables not-yet
//> Compiling Expressions forward-declarations

static void expression();
//> Global Variables not-yet
static void statement();
static void declaration();
//< Global Variables not-yet
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

//< Compiling Expressions forward-declarations
//> Global Variables not-yet

// Creates a string constant for the given identifier token. Returns the
// index of the constant.
static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}
//< Global Variables not-yet
//> Local Variables not-yet
static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler* compiler, Token* name, bool inFunction) {
  // Look it up in the local scopes. Look in reverse order so that the most
  // nested variable is found first and shadows outer ones.
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name))
    {
      if (!inFunction && local->depth == -1) {
        error("Cannot read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}
//> Closures not-yet

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
  if (compiler->function->upvalueCount == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }

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
  int local = resolveLocal(compiler->enclosing, name, true);
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
//< Closures not-yet

static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->localCount];
  local->name = name;

  // The local is declared but not yet defined.
  local->depth = -1;
//> Closures not-yet
  local->isUpvalue = false;
//< Closures not-yet
  current->localCount++;
}

// Allocates a local slot for the value currently on the stack, if we're in a
// local scope.
static void declareVariable() {
  // Global variables are implicitly declared.
  if (current->scopeDepth == 0) return;

  // See if a local variable with this name is already declared in this scope.
  Token* name = &parser.previous;
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) break;
    if (identifiersEqual(name, &local->name)) {
      error("Variable with this name already declared in this scope.");
    }
  }

  addLocal(*name);
}
//< Local Variables not-yet
//> Global Variables not-yet
static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
/* Global Variables not-yet < Local Variables not-yet
  return identifierConstant(&parser.previous);
*/
//> Local Variables not-yet

  // If it's a global variable, create a string constant for it.
  if (current->scopeDepth == 0) {
    return identifierConstant(&parser.previous);
  }

  declareVariable();
  return 0;
//< Local Variables not-yet
}

static void defineVariable(uint8_t global) {
/* Global Variables not-yet < Local Variables not-yet
  emitBytes(OP_DEFINE_GLOBAL, global);
*/
//> Local Variables not-yet
  if (current->scopeDepth == 0) {
    emitBytes(OP_DEFINE_GLOBAL, global);
  } else {
    // Mark the local as defined now.
    current->locals[current->localCount - 1].depth = current->scopeDepth;
  }
//< Local Variables not-yet
}
//< Global Variables not-yet
//> Calls and Functions not-yet
static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      argCount++;

      if (argCount > 8) {
        error("Cannot have more than 8 arguments.");
      }
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return argCount;
}
//< Calls and Functions not-yet
//> Jumping Forward and Back not-yet
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
//< Jumping Forward and Back not-yet
//> Compiling Expressions binary
/* Compiling Expressions binary < Global Variables not-yet
static void binary() {
*/
//> Global Variables not-yet
static void binary(bool canAssign) {
//< Global Variables not-yet
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);

  // Compile the right-hand operand.
  parsePrecedence((Precedence)(rule->precedence + 1));

  // Emit the operator instruction.
  switch (operatorType) {
//> Types of Values not-yet
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
    case TOKEN_GREATER:       emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
//< Types of Values not-yet
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default:
      return; // Unreachable.
  }
}
//< Compiling Expressions binary
//> Calls and Functions not-yet
static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitByte(OP_CALL_0 + argCount);
}
//< Calls and Functions not-yet
//> Classes and Instances not-yet
static void dot(bool canAssign) {
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
  uint8_t name = identifierConstant(&parser.previous);

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
//> Methods and Initializers not-yet
  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE_0 + argCount, name);
//< Methods and Initializers not-yet
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}
//< Classes and Instances not-yet
//> Types of Values not-yet
/* Types of Values not-yet < Global Variables not-yet
static void false_() {
*/
//> Global Variables not-yet
static void false_(bool canAssign) {
//< Global Variables not-yet
  emitByte(OP_FALSE);
}
//< Types of Values not-yet
//> Compiling Expressions grouping
/* Compiling Expressions grouping < Global Variables not-yet
static void grouping() {
*/
//> Global Variables not-yet
static void grouping(bool canAssign) {
//< Global Variables not-yet
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}
//< Compiling Expressions grouping
/* Types of Values not-yet < Global Variables not-yet
static void nil() {
*/
//> Types of Values not-yet
//> Global Variables not-yet
static void nil(bool canAssign) {
//< Global Variables not-yet
  emitByte(OP_NIL);
}
//< Types of Values not-yet
/* Compiling Expressions number < Global Variables not-yet
static void number() {
*/
//> Compiling Expressions number
//> Global Variables not-yet
static void number(bool canAssign) {
//< Global Variables not-yet
  double value = strtod(parser.previous.start, NULL);
/* Compiling Expressions number < Types of Values not-yet
  emitConstant(value);
*/
//> Types of Values not-yet
  emitConstant(NUMBER_VAL(value));
//< Types of Values not-yet
}
//< Compiling Expressions number
//> Jumping Forward and Back not-yet
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
//< Jumping Forward and Back not-yet
/* Strings not-yet < Global Variables not-yet
static void string() {
*/
//> Strings not-yet
//> Global Variables not-yet
static void string(bool canAssign) {
//< Global Variables not-yet
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}
//< Strings not-yet
//> Global Variables not-yet
// Compiles a reference to a variable whose name is the given token.
static void namedVariable(Token name, bool canAssign) {
/* Global Variables not-yet < Local Variables not-yet
  int arg = identifierConstant(&name);
*/
//> Local Variables not-yet
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name, false);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
//< Local Variables not-yet
//> Closures not-yet
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
//< Closures not-yet
//> Local Variables not-yet
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

//< Local Variables not-yet
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
/* Global Variables not-yet < Local Variables not-yet
    emitBytes(OP_SET_GLOBAL, (uint8_t)arg);
*/
//> Local Variables not-yet
    emitBytes(setOp, (uint8_t)arg);
//< Local Variables not-yet
  } else {
/* Global Variables not-yet < Local Variables not-yet
    emitBytes(OP_GET_GLOBAL, (uint8_t)arg);
*/
//> Local Variables not-yet
    emitBytes(getOp, (uint8_t)arg);
//< Local Variables not-yet
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}
//< Global Variables not-yet
//> Superclasses not-yet
static Token syntheticToken(const char* text) {
  Token token;
  token.start = text;
  token.length = (int)strlen(text);
  return token;
}

static void pushSuperclass() {
  if (currentClass == NULL) return;
  namedVariable(syntheticToken("super"), false);
}

static void super_(bool canAssign) {
  if (currentClass == NULL) {
    error("Cannot use 'super' outside of a class.");
  } else if (!currentClass->hasSuperclass) {
    error("Cannot use 'super' in a class with no superclass.");
  }

  consume(TOKEN_DOT, "Expect '.' after 'super'.");
  consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
  uint8_t name = identifierConstant(&parser.previous);

  // Push the receiver.
  namedVariable(syntheticToken("this"), false);

  if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();

    pushSuperclass();
    emitBytes(OP_SUPER_0 + argCount, name);
  } else {
    pushSuperclass();
    emitBytes(OP_GET_SUPER, name);
  }
}
//< Superclasses not-yet
//> Methods and Initializers not-yet
static void this_(bool canAssign) {
  if (currentClass == NULL) {
    error("Cannot use 'this' outside of a class.");
  } else {
    variable(false);
  }
}
//< Methods and Initializers not-yet
/* Types of Values not-yet < Global Variables not-yet

static void true_() {
*/
//> Types of Values not-yet
//> Global Variables not-yet

static void true_(bool canAssign) {
//< Global Variables not-yet
  emitByte(OP_TRUE);
}
//< Types of Values not-yet
//> Compiling Expressions unary
/* Compiling Expressions unary < Global Variables not-yet
static void unary() {
*/
//> Global Variables not-yet
static void unary(bool canAssign) {
//< Global Variables not-yet
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_CALL);

  // Emit the operator instruction.
  switch (operatorType) {
//> Types of Values not-yet
    case TOKEN_BANG: emitByte(OP_NOT); break;
//< Types of Values not-yet
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default:
      return; // Unreachable.
  }
}
//< Compiling Expressions unary
//> Compiling Expressions rules
ParseRule rules[] = {
/* Compiling Expressions rules < Calls and Functions not-yet
  { grouping, NULL,    PREC_CALL },       // TOKEN_LEFT_PAREN
*/
//> Calls and Functions not-yet
  { grouping, call,    PREC_CALL },       // TOKEN_LEFT_PAREN
//< Calls and Functions not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
/* Compiling Expressions rules < Classes and Instances not-yet
  { NULL,     NULL,    PREC_CALL },       // TOKEN_DOT
*/
//> Classes and Instances not-yet
  { NULL,     dot,     PREC_CALL },       // TOKEN_DOT
//< Classes and Instances not-yet
  { unary,    binary,  PREC_TERM },       // TOKEN_MINUS
  { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR
/* Compiling Expressions rules < Types of Values not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     NULL,    PREC_EQUALITY },   // TOKEN_BANG_EQUAL
*/
//> Types of Values not-yet
  { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
//< Types of Values not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
/* Compiling Expressions rules < Types of Values not-yet
  { NULL,     NULL,    PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_LESS_EQUAL
*/
//> Types of Values not-yet
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
//< Types of Values not-yet
/* Compiling Expressions rules < Global Variables not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
*/
//> Global Variables not-yet
  { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
//< Global Variables not-yet
/* Compiling Expressions rules < Strings not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_STRING
*/
//> Strings not-yet
  { string,   NULL,    PREC_NONE },       // TOKEN_STRING
//< Strings not-yet
  { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
/* Compiling Expressions rules < Jumping Forward and Back not-yet
  { NULL,     NULL,    PREC_AND },        // TOKEN_AND
*/
//> Jumping Forward and Back not-yet
  { NULL,     and_,    PREC_AND },        // TOKEN_AND
//< Jumping Forward and Back not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
/* Compiling Expressions rules < Types of Values not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FALSE
*/
//> Types of Values not-yet
  { false_,   NULL,    PREC_NONE },       // TOKEN_FALSE
//< Types of Values not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
/* Compiling Expressions rules < Types of Values not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_NIL
*/
//> Types of Values not-yet
  { nil,      NULL,    PREC_NONE },       // TOKEN_NIL
//< Types of Values not-yet
/* Compiling Expressions rules < Jumping Forward and Back not-yet
  { NULL,     NULL,    PREC_OR },         // TOKEN_OR
*/
//> Jumping Forward and Back not-yet
  { NULL,     or_,     PREC_OR },         // TOKEN_OR
//< Jumping Forward and Back not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
/* Compiling Expressions rules < Superclasses not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER
*/
//> Superclasses not-yet
  { super_,   NULL,    PREC_NONE },       // TOKEN_SUPER
//< Superclasses not-yet
/* Compiling Expressions rules < Methods and Initializers not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
*/
//> Methods and Initializers not-yet
  { this_,    NULL,    PREC_NONE },       // TOKEN_THIS
//< Methods and Initializers not-yet
/* Compiling Expressions rules < Types of Values not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_TRUE
*/
//> Types of Values not-yet
  { true_,    NULL,    PREC_NONE },       // TOKEN_TRUE
//< Types of Values not-yet
  { NULL,     NULL,    PREC_NONE },       // TOKEN_VAR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};
//< Compiling Expressions rules
//> Compiling Expressions parse-precedence
static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

/* Compiling Expressions parse-precedence < Global Variables not-yet
  prefixRule();
*/
//> Global Variables not-yet
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);
//< Global Variables not-yet
//> infix
  
  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
/* Compiling Expressions infix < Global Variables not-yet
    infixRule();
*/
//> Global Variables not-yet
    infixRule(canAssign);
//< Global Variables not-yet
  }
//> Global Variables not-yet

  if (canAssign && match(TOKEN_EQUAL)) {
    // If we get here, we didn't parse the "=" even though we could have, so
    // the LHS must not be a valid lvalue.
    error("Invalid assignment target.");
    expression();
  }
//< Global Variables not-yet
//< infix
}
//< Compiling Expressions parse-precedence
//> Compiling Expressions get-rule
static ParseRule* getRule(TokenType type) {
  return &rules[type];
}
//< Compiling Expressions get-rule
//> Compiling Expressions expression
void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}
//< Compiling Expressions expression
//> Local Variables not-yet
static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}
//< Local Variables not-yet
//> Calls and Functions not-yet
static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, 1, type);

  // Compile the parameter list.
  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      uint8_t paramConstant = parseVariable("Expect parameter name.");
      defineVariable(paramConstant);

      current->function->arity++;
      if (current->function->arity > 8) {
        error("Cannot have more than 8 parameters.");
      }
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

  // The body.
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();

  // Create the function object.
  endScope();
  ObjFunction* function = endCompiler();
/* Calls and Functions not-yet < Closures not-yet
  emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
*/
//> Closures not-yet

  // Capture the upvalues in the new closure object.
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

  // Emit arguments for each upvalue to know whether to capture a local or
  // an upvalue.
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
//< Closures not-yet
}
//< Calls and Functions not-yet
//> Methods and Initializers not-yet
static void method() {
  consume(TOKEN_IDENTIFIER, "Expect method name.");
  uint8_t constant = identifierConstant(&parser.previous);

  // If the method is named "init", it's an initializer.
  FunctionType type = TYPE_METHOD;
  if (parser.previous.length == 4 &&
      memcmp(parser.previous.start, "init", 4) == 0) {
    type = TYPE_INITIALIZER;
  }

  function(type);

  emitBytes(OP_METHOD, constant);
}
//< Methods and Initializers not-yet
//> Classes and Instances not-yet
static void classDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();

//> Methods and Initializers not-yet
  ClassCompiler classCompiler;
  classCompiler.name = parser.previous;
//< Methods and Initializers not-yet
//> Superclasses not-yet
  classCompiler.hasSuperclass = false;
//< Superclasses not-yet
//> Methods and Initializers not-yet
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

//< Methods and Initializers not-yet
/* Classes and Instances not-yet < Superclasses not-yet
  emitBytes(OP_CLASS, nameConstant);
*/
//> Superclasses not-yet
  if (match(TOKEN_LESS)) {
    consume(TOKEN_IDENTIFIER, "Expect superclass name.");
    classCompiler.hasSuperclass = true;

    beginScope();

    // Store the superclass in a local variable named "super".
    variable(false);
    addLocal(syntheticToken("super"));

    emitBytes(OP_SUBCLASS, nameConstant);
  } else {
    emitBytes(OP_CLASS, nameConstant);
  }
//< Superclasses not-yet

  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
//> Methods and Initializers not-yet
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }
//< Methods and Initializers not-yet
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
//> Superclasses not-yet

  if (classCompiler.hasSuperclass) {
    endScope();
  }
//< Superclasses not-yet
  defineVariable(nameConstant);
//> Methods and Initializers not-yet

  currentClass = currentClass->enclosing;
//< Methods and Initializers not-yet
}
//< Classes and Instances not-yet
//> Calls and Functions not-yet
static void funDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  function(TYPE_FUNCTION);
  defineVariable(global);
}
//< Calls and Functions not-yet
//> Global Variables not-yet
static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    // Compile the initializer.
    expression();
  } else {
    // Default to nil.
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}

static void expressionStatement() {
  expression();
  emitByte(OP_POP);
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");

}
//< Global Variables not-yet
//> Jumping Forward and Back not-yet
static void forStatement() {
  // for (var i = 0; i < 10; i = i + 1) print i;
  //
  //   var i = 0;
  // start:                      <--.
  //   if (i < 10) goto exit;  --.  |
  //   goto body;  -----------.  |  |
  // increment:            <--+--+--+--.
  //   i = i + 1;             |  |  |  |
  //   goto start;  ----------+--+--'  |
  // body:                 <--'  |     |
  //   print i;                  |     |
  //   goto increment;  ---------+-----'
  // exit:                    <--'

  // Create a scope for the loop variable.
  beginScope();

  // The initialization clause.
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_VAR)) {
    varDeclaration();
  } else if (match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else {
    expressionStatement();
  }

  int loopStart = currentChunk()->count;

  // The exit condition.
  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  // Increment step.
  if (!match(TOKEN_RIGHT_PAREN)) {
    // We don't want to execute the increment before the body, so jump over it.
    int bodyJump = emitJump(OP_JUMP);

    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    // After the increment, start the whole loop over.
    emitLoop(loopStart);

    // At the end of the body, we want to jump to the increment, not the top
    // of the loop.
    loopStart = incrementStart;

    patchJump(bodyJump);
  }

  // Compile the body.
  statement();

  // Jump back to the beginning (or the increment).
  emitLoop(loopStart);

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }

  endScope(); // Loop variable.
}

static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

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
}
//< Jumping Forward and Back not-yet
//> Global Variables not-yet
static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}
//< Global Variables not-yet
//> Calls and Functions not-yet
static void returnStatement() {
  if (current->type == TYPE_TOP_LEVEL) {
    error("Cannot return from top-level code.");
  }

  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
//> Methods and Initializers not-yet
    if (current->type == TYPE_INITIALIZER) {
      error("Cannot return a value from an initializer.");
    }

//< Methods and Initializers not-yet
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}
//< Calls and Functions not-yet
//> Jumping Forward and Back not-yet
static void whileStatement() {
  int loopStart = currentChunk()->count;

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  // Jump out of the loop if the condition is false.
  int exitJump = emitJump(OP_JUMP_IF_FALSE);

  // Compile the body.
  emitByte(OP_POP); // Condition.
  statement();

  // Loop back to the start.
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP); // Condition.
}
//< Jumping Forward and Back not-yet
//> Global Variables not-yet

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

static void declaration() {
//> Classes and Instances not-yet
  if (match(TOKEN_CLASS)) {
    classDeclaration();
/* Calls and Functions not-yet < Classes and Instances not-yet
  if (match(TOKEN_FUN)) {
*/
  } else if (match(TOKEN_FUN)) {
//< Classes and Instances not-yet
//> Calls and Functions not-yet
    funDeclaration();
/* Global Variables not-yet < Calls and Functions not-yet
  if (match(TOKEN_VAR)) {
*/
  } else if (match(TOKEN_VAR)) {
//< Calls and Functions not-yet
    varDeclaration();
  } else {
    statement();
  }
  
  if (parser.panicMode) synchronize();
}

static void statement() {
/* Global Variables not-yet < Jumping Forward and Back not-yet
  if (match(TOKEN_PRINT)) {
*/
//> Jumping Forward and Back not-yet
  if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_PRINT)) {
//< Jumping Forward and Back not-yet
    printStatement();
//> Calls and Functions not-yet
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
//< Calls and Functions not-yet
//> Jumping Forward and Back not-yet
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
//< Jumping Forward and Back not-yet
//> Local Variables not-yet
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
//< Local Variables not-yet
  } else {
    expressionStatement();
  }
}
//< Global Variables not-yet
/* Scanning on Demand compiler-c < Compiling Expressions compile-signature

void compile(const char* source) {
*/
/* Compiling Expressions compile-signature < Calls and Functions not-yet

bool compile(const char* source, Chunk* chunk) {
*/
//> Calls and Functions not-yet

ObjFunction* compile(const char* source) {
//< Calls and Functions not-yet
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
    printf("%2d '%.*s'\n", token.type, token.length, token.start);

    if (token.type == TOKEN_EOF) break;
  }
*/
//> Local Variables not-yet
  Compiler mainCompiler;
//< Local Variables not-yet
/* Local Variables not-yet < Calls and Functions not-yet
  initCompiler(&mainCompiler);
*/
//> Calls and Functions not-yet
  initCompiler(&mainCompiler, 0, TYPE_TOP_LEVEL);
//< Calls and Functions not-yet
/* Compiling Expressions init-compile-chunk < Calls and Functions not-yet
  compilingChunk = chunk;
*/
//> Compiling Expressions compile-chunk
//> init-parser-error

  parser.hadError = false;
  parser.panicMode = false;
  
//< init-parser-error
  advance();

//< Compiling Expressions compile-chunk
/* Compiling Expressions compile-chunk < Global Variables not-yet
  expression();
  consume(TOKEN_EOF, "Expect end of expression.");
*/
//> Global Variables not-yet
  if (!match(TOKEN_EOF)) {
    do {
      declaration();
    } while (!match(TOKEN_EOF));
  }

//< Global Variables not-yet
/* Compiling Expressions finish-compile < Calls and Functions not-yet
  endCompiler();
  return !parser.hadError;
*/
//> Calls and Functions not-yet
  ObjFunction* function = endCompiler();

  // If there was a compile error, the code is not valid, so don't create a
  // function.
  return parser.hadError ? NULL : function;
//< Calls and Functions not-yet
}
//> Garbage Collection not-yet

void grayCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    grayObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
//< Garbage Collection not-yet
