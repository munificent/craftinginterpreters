//>= Compiling Expressions
#include <assert.h>
//>= Uhh
#include <stdarg.h>
//>= Scanning on Demand
#include <stdio.h>
//>= Compiling Expressions
#include <stdlib.h>
//>= Local Variables
#include <string.h>
//>= Scanning on Demand

#include "common.h"
#include "compiler.h"
//>= Uhh
#include "object.h"
//>= Garbage Collection
#include "memory.h"
//>= Scanning on Demand
#include "scanner.h"
//>= Uhh
#include "vm.h"
//>= Compiling Expressions

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

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

/*>= Compiling Expressions <= Statements
typedef void (*ParseFn)();
*/
//>= Global Variables
typedef void (*ParseFn)(bool canAssign);
//>= Compiling Expressions

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;
//>= Local Variables

typedef struct {
  // The name of the local variable.
  Token name;
  
  // The depth in the scope chain that this variable was declared at. Zero is
  // the outermost scope--parameters for a method, or the first local block in
  // top level code. One is the scope within that, etc.
  int depth;
//>= Closures
  
  // True if this local variable is captured as an upvalue by a function.
  bool isUpvalue;
//>= Local Variables
} Local;
//>= Closures

typedef struct {
  // The index of the local variable or upvalue being captured from the
  // enclosing function.
  uint8_t index;
  
  // Whether the captured variable is a local or upvalue in the enclosing
  // function.
  bool isLocal;
} Upvalue;
//>= Functions

typedef enum {
  TYPE_FUNCTION,
//>= Methods and Initializers
  TYPE_INITIALIZER,
  TYPE_METHOD,
//>= Functions
  TYPE_TOP_LEVEL
} FunctionType;
//>= Local Variables

typedef struct Compiler {
//>= Functions
  // The compiler for the enclosing function, if any.
  struct Compiler* enclosing;

  // The function being compiled.
  ObjFunction* function;
//>= Functions
  
  FunctionType type;
  
//>= Local Variables
  
  // The currently in scope local variables.
  Local locals[UINT8_COUNT];
  
  // The number of local variables currently in scope.
  int localCount;
//>= Closures
  Upvalue upvalues[UINT8_COUNT];
//>= Local Variables
  
  // The current level of block scope nesting. Zero is the outermost local
  // scope. 0 is global scope.
  int scopeDepth;
} Compiler;
//>= Methods and Initializers

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
  
  Token name;
//>= Inheritance
  bool hasSuperclass;
//>= Methods and Initializers
} ClassCompiler;
//>= Compiling Expressions

Parser parser;
/*>= Compiling Expressions <= Jumping Forward and Back

Chunk* compilingChunk;
 
static Chunk* currentChunk() {
  return compilingChunk;
}
*/
//>= Local Variables

// The compiler for the innermost function currently being compiled.
Compiler* current = NULL;
//>= Methods and Initializers

// The compiler for the innermost class currently being compiled.
ClassCompiler* currentClass = NULL;
//>= Functions

static Chunk* currentChunk() {
  return &current->function->chunk;
}
//>= Compiling Expressions

static void errorAt(Token* token, const char* message) {
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

static void error(const char* message) {
  errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;
  
  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) break;
    
    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  
  errorAtCurrent(message);
  
  // If we're consuming a synchronizing token, keep going until we find it.
/*>= Compiling Expressions <= Hash Tables
  if (type == TOKEN_RIGHT_PAREN) {
*/
//>= Statements
  if (type == TOKEN_LEFT_BRACE ||
      type == TOKEN_RIGHT_BRACE ||
      type == TOKEN_RIGHT_PAREN ||
//>= Global Variables
      type == TOKEN_EQUAL ||
//>= Statements
      type == TOKEN_SEMICOLON) {
//>= Compiling Expressions
    while (parser.current.type != type &&
           parser.current.type != TOKEN_EOF) {
      advance();
    }
    
    advance();
  }
}
//>= Statements

static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}
//>= Compiling Expressions

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}
//>= Jumping Forward and Back

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
//>= Compiling Expressions

static void emitReturn() {
/*>= Functions <= Classes and Instances
  emitByte(OP_NIL);
*/
//>= Methods and Initializers
  // An initializer automatically returns "this".
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }
  
//>= Compiling Expressions
  emitByte(OP_RETURN);
}
//>= Jumping Forward and Back

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
/*>= Local Variables <= Jumping Forward and Back
 
static void initCompiler(Compiler* compiler) {
*/
//>= Functions

static void initCompiler(Compiler* compiler, int scopeDepth,
                         FunctionType type) {
//>= Functions
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
//>= Local Variables
  compiler->localCount = 0;
/*>= Local Variables <= Jumping Forward and Back
  compiler->scopeDepth = 0;
*/
//>= Functions
  compiler->scopeDepth = scopeDepth;
  compiler->function = newFunction();
//>= Local Variables
  current = compiler;
//>= Functions
  
  switch (type) {
    case TYPE_FUNCTION:
      current->function->name = copyString(parser.previous.start,
                                           parser.previous.length);
      break;
      
//>= Methods and Initializers
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
//>= Functions
    case TYPE_TOP_LEVEL:
      current->function->name = copyString("script", 6);
      break;
  }
  
  // The first slot is always implicitly declared.
  Local* local = &current->locals[current->localCount++];
  local->depth = current->scopeDepth;
//>= Closures
  local->isUpvalue = false;
/*>= Functions <= Classes and Instances
  local->name.start = "";
  local->name.length = 0;
*/
//>= Methods and Initializers
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
//>= Local Variables
}
/*>= Compiling Expressions <= Jumping Forward and Back

static void endCompiler() {
*/
//>= Functions

static ObjFunction* endCompiler() {
//>= Compiling Expressions
  emitReturn();
//>= Functions
  
  ObjFunction* function = current->function;
//>= Compiling Expressions
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
/*>= Compiling Expressions <= Jumping Forward and Back
    disassembleChunk(currentChunk(), "code");
*/
//>= Functions
    disassembleChunk(currentChunk(), function->name->chars);
//>= Compiling Expressions
  }
#endif
//>= Functions
  current = current->enclosing;
  
  return function;
//>= Compiling Expressions
}
//>= Local Variables

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;
  
  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
/*>= Local Variables <= Functions
    emitByte(OP_POP);
*/
//>= Closures
    if (current->locals[current->localCount - 1].isUpvalue) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
//>= Local Variables
    current->localCount--;
  }
}
//>= Compiling Expressions

// Forward declarations since the grammar is recursive.
static void expression();
//>= Statements
static void statement();
static void declaration();
//>= Compiling Expressions
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
//>= Compiling Expressions

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant == -1) {
    error("Too many constants in one chunk.");
    return 0;
  }
  
  return (uint8_t)constant;
}
//>= Global Variables

// Creates a string constant for the given identifier token. Returns the
// index of the constant.
static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}
//>= Compiling Expressions

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}
//>= Local Variables

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}
//>= Local Variables

static int resolveLocal(Compiler* compiler, Token* name, bool inFunction) {
  // Look it up in the local scopes. Look in reverse order so that the most
  // nested variable is found first and shadows outer ones.
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name))
    {
      if (!inFunction && local->depth == -1) {
        error("Cannot read local variable before initializing it.");
      }
      return i;
    }
  }
  
  return -1;
}
//>= Closures

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
//>= Local Variables

static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }
  
  Local* local = &current->locals[current->localCount];
  local->name = name;
  
  // The local is declared but not yet defined.
  local->depth = -1;
//>= Closures
  local->isUpvalue = false;
//>= Local Variables
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
//>= Global Variables

static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
/*== Global Variables
  return identifierConstant(&parser.previous);
*/
//>= Local Variables
  
  // If it's a global variable, create a string constant for it.
  if (current->scopeDepth == 0) {
    return identifierConstant(&parser.previous);
  }
  
  declareVariable();
  return 0;
//>= Global Variables
}

static void defineVariable(uint8_t global) {
/*== Global Variables
  emitBytes(OP_DEFINE_GLOBAL, global);
*/
//>= Local Variables
  if (current->scopeDepth == 0) {
    emitBytes(OP_DEFINE_GLOBAL, global);
  } else {
    // Mark the local as defined now.
    current->locals[current->localCount - 1].depth = current->scopeDepth;
  }
//>= Global Variables
}
//>= Functions

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
//>= Jumping Forward and Back

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
/*>= Compiling Expressions <= Statements

static void binary() {
*/
//>= Global Variables

static void binary(bool canAssign) {
//>= Compiling Expressions
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);

  // Compile the right-hand operand.
  parsePrecedence((Precedence)(rule->precedence + 1));

  // Emit the operator instruction.
  switch (operatorType) {
//>= Types of Values
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
    case TOKEN_GREATER:       emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
//>= Compiling Expressions
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default:
      assert(false); // Unreachable.
  }
}
//>= Functions

static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitByte(OP_CALL_0 + argCount);
}
//>= Classes and Instances

static void dot(bool canAssign) {
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
  uint8_t name = identifierConstant(&parser.previous);
  
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_FIELD, name);
//>= Methods and Initializers
  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE_0 + argCount, name);
//>= Classes and Instances
  } else {
    emitBytes(OP_GET_FIELD, name);
  }
}
/*>= Types of Values <= Statements
 
static void false_() {
*/
//>= Global Variables

static void false_(bool canAssign) {
//>= Types of Values
  emitByte(OP_FALSE);
}
/*>= Compiling Expressions <= Statements
 
static void grouping() {
*/
//>= Global Variables

static void grouping(bool canAssign) {
//>= Compiling Expressions
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}
/*>= Types of Values <= Statements
 
static void nil() {
*/
//>= Global Variables

static void nil(bool canAssign) {
//>= Types of Values
  emitByte(OP_NIL);
}
/*>= Compiling Expressions <= Statements
 
static void number() {
*/
//>= Global Variables

static void number(bool canAssign) {
//>= Compiling Expressions
  double value = strtod(parser.previous.start, NULL);
/*== Compiling Expressions
  emitConstant(value);
*/
//>= Types of Values
  emitConstant(NUMBER_VAL(value));
//>= Compiling Expressions
}
//>= Jumping Forward and Back

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
/*>= Strings <= Statements
 
static void string() {
*/
//>= Global Variables

static void string(bool canAssign) {
//>= Strings
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}
//>= Global Variables

// Compiles a reference to a variable whose name is the given token.
static void namedVariable(Token name, bool canAssign) {
/*== Global Variables
  int arg = identifierConstant(&name);
*/
//>= Local Variables
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name, false);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
//>= Closures
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
//>= Local Variables
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }
  
//>= Global Variables
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
/*== Global Variables
    emitBytes(OP_SET_GLOBAL, (uint8_t)arg);
*/
//>= Local Variables
    emitBytes(setOp, (uint8_t)arg);
//>= Global Variables
  } else {
/*== Global Variables
    emitBytes(OP_GET_GLOBAL, (uint8_t)arg);
*/
//>= Local Variables
    emitBytes(getOp, (uint8_t)arg);
//>= Global Variables
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}
//>= Inheritance

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
//>= Methods and Initializers

static void this_(bool canAssign) {
  if (currentClass == NULL) {
    error("Cannot use 'this' outside of a class.");
  } else {
    variable(false);
  }
}
/*>= Types of Values <= Statements
 
static void true_() {
*/
//>= Global Variables

static void true_(bool canAssign) {
//>= Types of Values
  emitByte(OP_TRUE);
}
/*>= Compiling Expressions <= Statements
 
static void unary() {
*/
//>= Global Variables

static void unary(bool canAssign) {
//>= Compiling Expressions
  TokenType operatorType = parser.previous.type;
  
  // Compile the operand.
  parsePrecedence(PREC_CALL);
  
  // Emit the operator instruction.
  switch (operatorType) {
//>= Types of Values
    case TOKEN_BANG: emitByte(OP_NOT); break;
//>= Compiling Expressions
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default:
      assert(false); // Unreachable.
  }
}

ParseRule rules[] = {
/*>= Compiling Expressions <= Jumping Forward and Back
  { grouping, NULL,    PREC_CALL },       // TOKEN_LEFT_PAREN
*/
//>= Functions
  { grouping, call,    PREC_CALL },       // TOKEN_LEFT_PAREN
//>= Compiling Expressions
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
/*== Compiling Expressions
  { NULL,     NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     NULL,    PREC_EQUALITY },   // TOKEN_BANG_EQUAL
*/
//>= Types of Values
  { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
//>= Compiling Expressions
  { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
/*>= Compiling Expressions <= Garbage Collection
  { NULL,     NULL,    PREC_CALL },       // TOKEN_DOT
*/
//>= Classes and Instances
  { NULL,     dot,     PREC_CALL },       // TOKEN_DOT
//>= Compiling Expressions
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
/*== Compiling Expressions
  { NULL,     NULL,    PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_LESS_EQUAL
*/
//>= Types of Values
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
//>= Compiling Expressions
  { unary,    binary,  PREC_TERM },       // TOKEN_MINUS
  { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR
/*>= Compiling Expressions <= Statements
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
*/
//>= Global Variables
  { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
/*>= Compiling Expressions <= Types of Values
  { NULL,     NULL,    PREC_NONE },       // TOKEN_STRING
*/
//>= Strings
  { string,   NULL,    PREC_NONE },       // TOKEN_STRING
//>= Compiling Expressions
  { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
/*>= Compiling Expressions <= Local Variables
  { NULL,     NULL,    PREC_AND },        // TOKEN_AND
*/
//>= Jumping Forward and Back
  { NULL,     and_,    PREC_AND },        // TOKEN_AND
//>= Compiling Expressions
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
/*== Compiling Expressions
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FALSE
*/
//>= Types of Values
  { false_,   NULL,    PREC_NONE },       // TOKEN_FALSE
//>= Compiling Expressions
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
/*== Compiling Expressions
  { NULL,     NULL,    PREC_NONE },       // TOKEN_NIL
*/
//>= Types of Values
  { nil,      NULL,    PREC_NONE },       // TOKEN_NIL
/*>= Compiling Expressions <= Local Variables
  { NULL,     NULL,    PREC_OR },         // TOKEN_OR
*/
//>= Jumping Forward and Back
  { NULL,     or_,     PREC_OR },         // TOKEN_OR
//>= Compiling Expressions
  { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
/*>= Compiling Expressions <= Methods and Initializers
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER
*/
//>= Inheritance
  { super_,   NULL,    PREC_NONE },       // TOKEN_SUPER
/*>= Compiling Expressions <= Classes and Instances
  { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
*/
//>= Methods and Initializers
  { this_,    NULL,    PREC_NONE },       // TOKEN_THIS
/*== Compiling Expressions
  { NULL,     NULL,    PREC_NONE },       // TOKEN_TRUE
*/
//>= Types of Values
  { true_,    NULL,    PREC_NONE },       // TOKEN_TRUE
//>= Compiling Expressions
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
    error("Expect expression.");
    return;
  }
  
/*>= Compiling Expressions <= Statements
  prefixRule();
*/
//>= Global Variables
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);
//>= Compiling Expressions

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
/*>= Compiling Expressions <= Statements
    infixRule();
*/
//>= Global Variables
    infixRule(canAssign);
//>= Compiling Expressions
  }
//>= Global Variables
  
  if (canAssign && match(TOKEN_EQUAL)) {
    // If we get here, we didn't parse the "=" even though we could have, so
    // the LHS must not be a valid lvalue.
    error("Invalid assignment target.");
    expression();
  }
//>= Compiling Expressions
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}
//>= Statements

static void block() {
  consume(TOKEN_LEFT_BRACE, "Expect '{' before block.");

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }
  
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}
//>= Functions

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
  block();
  
  // Create the function object.
  endScope();
  ObjFunction* function = endCompiler();
/*== Functions
  emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
*/
//>= Closures
  
  // Capture the upvalues in the new closure object.
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));
  
  // Emit arguments for each upvalue to know whether to capture a local or
  // an upvalue.
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
//>= Functions
}
//>= Methods and Initializers

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
//>= Classes and Instances

static void classDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();
  
//>= Methods and Initializers
  ClassCompiler classCompiler;
  classCompiler.name = parser.previous;
//>= Inheritance
  classCompiler.hasSuperclass = false;
//>= Methods and Initializers
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;
  
//>= Inheritance
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
/*>= Classes and Instances <= Methods and Initializers
  emitBytes(OP_CLASS, nameConstant);
*/
//>= Classes and Instances

  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
//>= Methods and Initializers
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }
//>= Classes and Instances
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
//>= Inheritance
  
  if (classCompiler.hasSuperclass) {
    endScope();
  }
//>= Classes and Instances
  defineVariable(nameConstant);
//>= Methods and Initializers
  
  currentClass = currentClass->enclosing;
//>= Classes and Instances
}
//>= Functions

static void funDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  function(TYPE_FUNCTION);
  defineVariable(global);
}
//>= Global Variables

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
//>= Statements

static void expressionStatement() {
  expression();
  emitByte(OP_POP);
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  
}
//>= Jumping Forward and Back

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
//>= Statements

static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}
//>= Functions

static void returnStatement() {
  if (current->type == TYPE_TOP_LEVEL) {
    error("Cannot return from top-level code.");
  }
  
  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
//>= Methods and Initializers
    if (current->type == TYPE_INITIALIZER) {
      error("Cannot return a value from an initializer.");
    }
    
//>= Functions
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}
//>= Jumping Forward and Back

static void whileStatement() {
  int loopStart = currentChunk()->count;
  
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
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
//>= Statements

static void declaration() {
//>= Classes and Instances
  if (match(TOKEN_CLASS)) {
    classDeclaration();
/*>= Functions <= Garbage Collection
  if (match(TOKEN_FUN)) {
*/
//>= Classes and Instances
  } else if (match(TOKEN_FUN)) {
//>= Functions
    funDeclaration();
/*>= Global Variables <= Jumping Forward and Back
  if (match(TOKEN_VAR)) {
*/
//>= Functions
  } else if (match(TOKEN_VAR)) {
//>= Global Variables
    varDeclaration();
  } else {
    statement();
  }
/*== Statements
  statement();
*/
//>= Statements
}

static void statement() {
//>= Jumping Forward and Back
  if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_PRINT)) {
/*>= Statements <= Local Variables
  if (match(TOKEN_PRINT)) {
*/
//>= Statements
    printStatement();
//>= Functions
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
//>= Jumping Forward and Back
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
//>= Statements
  } else if (check(TOKEN_LEFT_BRACE)) {
//>= Local Variables
    beginScope();
//>= Statements
    block();
//>= Local Variables
    endScope();
//>= Statements
  } else {
    expressionStatement();
  }
}
/*== Scanning on Demand

void compile(const char* source) {
*/
/*>= Compiling Expressions <= Jumping Forward and Back
 
bool compile(const char* source, Chunk* chunk) {
*/
//>= Functions

ObjFunction* compile(const char* source) {
//>= Scanning on Demand
  initScanner(source);
/*== Scanning on Demand
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
/*>= Compiling Expressions <= Jumping Forward and Back
  compilingChunk = chunk;
*/
//>= Local Variables
  Compiler mainCompiler;
/*>= Local Variables <= Jumping Forward and Back
  initCompiler(&mainCompiler);
*/
//>= Functions
  initCompiler(&mainCompiler, 0, TYPE_TOP_LEVEL);
//>= Compiling Expressions
  
  // Prime the pump.
  parser.hadError = false;
  advance();

/*>= Compiling Expressions <= Hash Tables
  expression();
  consume(TOKEN_EOF, "Expect end of expression.");
*/
//>= Statements
  if (!match(TOKEN_EOF)) {
    do {
      declaration();
    } while (!match(TOKEN_EOF));
  }

/*>= Compiling Expressions <= Jumping Forward and Back
  endCompiler();
 
  // If there was a compile error, the code is not valid.
  return !parser.hadError;
*/
//>= Functions
  ObjFunction* function = endCompiler();
  
  // If there was a compile error, the code is not valid, so don't create a
  // function.
  return parser.hadError ? NULL : function;
//>= Scanning on Demand
}
//>= Garbage Collection

void grayCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    grayObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
