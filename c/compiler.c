//>= Compiling Expressions 1
#include <assert.h>
//>= Scanning on Demand 1
#include <stdio.h>
//>= Compiling Expressions 1
#include <stdlib.h>
//>= Local Variables 1
#include <string.h>
//>= Scanning on Demand 1

#include "common.h"
#include "compiler.h"
//>= Garbage Collection 1
#include "memory.h"
//>= Scanning on Demand 1
#include "scanner.h"
//>= Compiling Expressions 1

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

/*>= Compiling Expressions 1 < Global Variables 1
typedef void (*ParseFn)();
*/
//>= Global Variables 1
typedef void (*ParseFn)(bool canAssign);
//>= Compiling Expressions 1

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;
//>= Local Variables 1

typedef struct {
  // The name of the local variable.
  Token name;
  
  // The depth in the scope chain that this variable was declared at. Zero is
  // the outermost scope--parameters for a method, or the first local block in
  // top level code. One is the scope within that, etc.
  int depth;
//>= Closures 1
  
  // True if this local variable is captured as an upvalue by a function.
  bool isUpvalue;
//>= Local Variables 1
} Local;
//>= Closures 1

typedef struct {
  // The index of the local variable or upvalue being captured from the
  // enclosing function.
  uint8_t index;
  
  // Whether the captured variable is a local or upvalue in the enclosing
  // function.
  bool isLocal;
} Upvalue;
//>= Calls and Functions 1

typedef enum {
  TYPE_FUNCTION,
//>= Methods and Initializers 1
  TYPE_INITIALIZER,
  TYPE_METHOD,
//>= Calls and Functions 1
  TYPE_TOP_LEVEL
} FunctionType;
//>= Local Variables 1

typedef struct Compiler {
//>= Calls and Functions 1
  // The compiler for the enclosing function, if any.
  struct Compiler* enclosing;

  // The function being compiled.
  ObjFunction* function;
  FunctionType type;
  
//>= Local Variables 1
  // The currently in scope local variables.
  Local locals[UINT8_COUNT];
  
  // The number of local variables currently in scope.
  int localCount;
//>= Closures 1
  Upvalue upvalues[UINT8_COUNT];
//>= Local Variables 1
  
  // The current level of block scope nesting. Zero is the outermost local
  // scope. 0 is global scope.
  int scopeDepth;
} Compiler;
//>= Methods and Initializers 1

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
  
  Token name;
//>= Superclasses 1
  bool hasSuperclass;
//>= Methods and Initializers 1
} ClassCompiler;
//>= Compiling Expressions 1

Parser parser;
//>= Local Variables 1

Compiler* current = NULL;
//>= Methods and Initializers 1

ClassCompiler* currentClass = NULL;
/*>= Compiling Expressions 1 < Calls and Functions 1

Chunk* compilingChunk;
 
static Chunk* currentChunk() {
  return compilingChunk;
}
*/
//>= Calls and Functions 1

static Chunk* currentChunk() {
  return &current->function->chunk;
}
//>= Compiling Expressions 1

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
/*>= Compiling Expressions 1 < Global Variables 1
  if (type == TOKEN_RIGHT_PAREN) {
*/
//>= Global Variables 1
  if (type == TOKEN_LEFT_BRACE ||
      type == TOKEN_RIGHT_BRACE ||
      type == TOKEN_RIGHT_PAREN ||
      type == TOKEN_EQUAL ||
      type == TOKEN_SEMICOLON) {
//>= Compiling Expressions 1
    while (parser.current.type != type &&
           parser.current.type != TOKEN_EOF) {
      advance();
    }
    
    advance();
  }
}
//>= Global Variables 1

static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}
//>= Compiling Expressions 1

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}
//>= Jumping Forward and Back 1

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
//>= Compiling Expressions 1

static void emitReturn() {
/*>= Calls and Functions 1 < Methods and Initializers 1
  emitByte(OP_NIL);
*/
//>= Methods and Initializers 1
  // An initializer automatically returns "this".
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }
  
//>= Compiling Expressions 1
  emitByte(OP_RETURN);
}
//>= Jumping Forward and Back 1

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
/*>= Local Variables 1 < Calls and Functions 1
 
static void initCompiler(Compiler* compiler) {
*/
//>= Calls and Functions 1

static void initCompiler(Compiler* compiler, int scopeDepth,
                         FunctionType type) {
//>= Calls and Functions 1
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
//>= Local Variables 1
  compiler->localCount = 0;
/*>= Local Variables 1 < Calls and Functions 1
  compiler->scopeDepth = 0;
*/
//>= Calls and Functions 1
  compiler->scopeDepth = scopeDepth;
  compiler->function = newFunction();
//>= Local Variables 1
  current = compiler;
//>= Calls and Functions 1
  
  switch (type) {
    case TYPE_FUNCTION:
      current->function->name = copyString(parser.previous.start,
                                           parser.previous.length);
      break;
      
//>= Methods and Initializers 1
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
//>= Calls and Functions 1
    case TYPE_TOP_LEVEL:
      current->function->name = NULL;
      break;
  }
  
  // The first slot is always implicitly declared.
  Local* local = &current->locals[current->localCount++];
  local->depth = current->scopeDepth;
//>= Closures 1
  local->isUpvalue = false;
/*>= Calls and Functions 1 < Methods and Initializers 1
  local->name.start = "";
  local->name.length = 0;
*/
//>= Methods and Initializers 1
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
//>= Local Variables 1
}
/*>= Compiling Expressions 1 < Calls and Functions 1

static void endCompiler() {
*/
//>= Calls and Functions 1

static ObjFunction* endCompiler() {
//>= Compiling Expressions 1
  emitReturn();
//>= Calls and Functions 1
  
  ObjFunction* function = current->function;
//>= Compiling Expressions 1
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
/*>= Compiling Expressions 1 < Calls and Functions 1
    disassembleChunk(currentChunk(), "code");
*/
//>= Calls and Functions 1
    disassembleChunk(currentChunk(), function->name->chars);
//>= Compiling Expressions 1
  }
#endif
//>= Calls and Functions 1
  current = current->enclosing;
  
  return function;
//>= Compiling Expressions 1
}
//>= Local Variables 1

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;
  
  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
/*>= Local Variables 1 < Closures 1
    emitByte(OP_POP);
*/
//>= Closures 1
    if (current->locals[current->localCount - 1].isUpvalue) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
//>= Local Variables 1
    current->localCount--;
  }
}
//>= Compiling Expressions 1

// Forward declarations since the grammar is recursive.
static void expression();
//>= Global Variables 1
static void statement();
static void declaration();
//>= Compiling Expressions 1
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
//>= Compiling Expressions 1

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant == -1) {
    error("Too many constants in one chunk.");
    return 0;
  }
  
  return (uint8_t)constant;
}
//>= Global Variables 1

// Creates a string constant for the given identifier token. Returns the
// index of the constant.
static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}
//>= Compiling Expressions 1

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}
//>= Local Variables 1

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}
//>= Local Variables 1

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
//>= Closures 1

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
//>= Local Variables 1

static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }
  
  Local* local = &current->locals[current->localCount];
  local->name = name;
  
  // The local is declared but not yet defined.
  local->depth = -1;
//>= Closures 1
  local->isUpvalue = false;
//>= Local Variables 1
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
//>= Global Variables 1

static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
/*>= Global Variables 1 < Local Variables 1
  return identifierConstant(&parser.previous);
*/
//>= Local Variables 1
  
  // If it's a global variable, create a string constant for it.
  if (current->scopeDepth == 0) {
    return identifierConstant(&parser.previous);
  }
  
  declareVariable();
  return 0;
//>= Global Variables 1
}

static void defineVariable(uint8_t global) {
/*>= Global Variables 1 < Local Variables 1
  emitBytes(OP_DEFINE_GLOBAL, global);
*/
//>= Local Variables 1
  if (current->scopeDepth == 0) {
    emitBytes(OP_DEFINE_GLOBAL, global);
  } else {
    // Mark the local as defined now.
    current->locals[current->localCount - 1].depth = current->scopeDepth;
  }
//>= Global Variables 1
}
//>= Calls and Functions 1

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
//>= Jumping Forward and Back 1

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
/*>= Compiling Expressions 1 < Global Variables 1

static void binary() {
*/
//>= Global Variables 1

static void binary(bool canAssign) {
//>= Compiling Expressions 1
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);

  // Compile the right-hand operand.
  parsePrecedence((Precedence)(rule->precedence + 1));

  // Emit the operator instruction.
  switch (operatorType) {
//>= Types of Values 1
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
    case TOKEN_GREATER:       emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
//>= Compiling Expressions 1
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default:
      assert(false); // Unreachable.
  }
}
//>= Calls and Functions 1

static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitByte(OP_CALL_0 + argCount);
}
//>= Classes and Instances 1

static void dot(bool canAssign) {
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
  uint8_t name = identifierConstant(&parser.previous);
  
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
//>= Methods and Initializers 1
  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE_0 + argCount, name);
//>= Classes and Instances 1
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}
/*>= Types of Values 1 < Global Variables 1
 
static void false_() {
*/
//>= Global Variables 1

static void false_(bool canAssign) {
//>= Types of Values 1
  emitByte(OP_FALSE);
}
/*>= Compiling Expressions 1 < Global Variables 1
 
static void grouping() {
*/
//>= Global Variables 1

static void grouping(bool canAssign) {
//>= Compiling Expressions 1
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}
/*>= Types of Values 1 < Global Variables 1
 
static void nil() {
*/
//>= Global Variables 1

static void nil(bool canAssign) {
//>= Types of Values 1
  emitByte(OP_NIL);
}
/*>= Compiling Expressions 1 < Global Variables 1
 
static void number() {
*/
//>= Global Variables 1

static void number(bool canAssign) {
//>= Compiling Expressions 1
  double value = strtod(parser.previous.start, NULL);
/*>= Compiling Expressions 1 < Types of Values 1
  emitConstant(value);
*/
//>= Types of Values 1
  emitConstant(NUMBER_VAL(value));
//>= Compiling Expressions 1
}
//>= Jumping Forward and Back 1

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
/*>= Strings 1 < Global Variables 1
 
static void string() {
*/
//>= Global Variables 1

static void string(bool canAssign) {
//>= Strings 1
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}
//>= Global Variables 1

// Compiles a reference to a variable whose name is the given token.
static void namedVariable(Token name, bool canAssign) {
/*>= Global Variables 1 < Local Variables 1
  int arg = identifierConstant(&name);
*/
//>= Local Variables 1
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name, false);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
//>= Closures 1
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
//>= Local Variables 1
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }
  
//>= Global Variables 1
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
/*>= Global Variables 1 < Local Variables 1
    emitBytes(OP_SET_GLOBAL, (uint8_t)arg);
*/
//>= Local Variables 1
    emitBytes(setOp, (uint8_t)arg);
//>= Global Variables 1
  } else {
/*>= Global Variables 1 < Local Variables 1
    emitBytes(OP_GET_GLOBAL, (uint8_t)arg);
*/
//>= Local Variables 1
    emitBytes(getOp, (uint8_t)arg);
//>= Global Variables 1
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}
//>= Superclasses 1

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
//>= Methods and Initializers 1

static void this_(bool canAssign) {
  if (currentClass == NULL) {
    error("Cannot use 'this' outside of a class.");
  } else {
    variable(false);
  }
}
/*>= Types of Values 1 < Global Variables 1
 
static void true_() {
*/
//>= Global Variables 1

static void true_(bool canAssign) {
//>= Types of Values 1
  emitByte(OP_TRUE);
}
/*>= Compiling Expressions 1 < Global Variables 1
 
static void unary() {
*/
//>= Global Variables 1

static void unary(bool canAssign) {
//>= Compiling Expressions 1
  TokenType operatorType = parser.previous.type;
  
  // Compile the operand.
  parsePrecedence(PREC_CALL);
  
  // Emit the operator instruction.
  switch (operatorType) {
//>= Types of Values 1
    case TOKEN_BANG: emitByte(OP_NOT); break;
//>= Compiling Expressions 1
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default:
      assert(false); // Unreachable.
  }
}

ParseRule rules[] = {
/*>= Compiling Expressions 1 < Calls and Functions 1
  { grouping, NULL,    PREC_CALL },       // TOKEN_LEFT_PAREN
*/
//>= Calls and Functions 1
  { grouping, call,    PREC_CALL },       // TOKEN_LEFT_PAREN
//>= Compiling Expressions 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
/*>= Compiling Expressions 1 < Types of Values 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     NULL,    PREC_EQUALITY },   // TOKEN_BANG_EQUAL
*/
//>= Types of Values 1
  { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
//>= Compiling Expressions 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
/*>= Compiling Expressions 1 < Classes and Instances 1
  { NULL,     NULL,    PREC_CALL },       // TOKEN_DOT
*/
//>= Classes and Instances 1
  { NULL,     dot,     PREC_CALL },       // TOKEN_DOT
//>= Compiling Expressions 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
/*>= Compiling Expressions 1 < Types of Values 1
  { NULL,     NULL,    PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_LESS_EQUAL
*/
//>= Types of Values 1
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
//>= Compiling Expressions 1
  { unary,    binary,  PREC_TERM },       // TOKEN_MINUS
  { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR
/*>= Compiling Expressions 1 < Global Variables 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
*/
//>= Global Variables 1
  { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
/*>= Compiling Expressions 1 < Strings 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_STRING
*/
//>= Strings 1
  { string,   NULL,    PREC_NONE },       // TOKEN_STRING
//>= Compiling Expressions 1
  { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
/*>= Compiling Expressions 1 < Jumping Forward and Back 1
  { NULL,     NULL,    PREC_AND },        // TOKEN_AND
*/
//>= Jumping Forward and Back 1
  { NULL,     and_,    PREC_AND },        // TOKEN_AND
//>= Compiling Expressions 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
/*>= Compiling Expressions 1 < Types of Values 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FALSE
*/
//>= Types of Values 1
  { false_,   NULL,    PREC_NONE },       // TOKEN_FALSE
//>= Compiling Expressions 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
/*>= Compiling Expressions 1 < Types of Values 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_NIL
*/
//>= Types of Values 1
  { nil,      NULL,    PREC_NONE },       // TOKEN_NIL
/*>= Compiling Expressions 1 < Jumping Forward and Back 1
  { NULL,     NULL,    PREC_OR },         // TOKEN_OR
*/
//>= Jumping Forward and Back 1
  { NULL,     or_,     PREC_OR },         // TOKEN_OR
//>= Compiling Expressions 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
/*>= Compiling Expressions 1 < Superclasses 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER
*/
//>= Superclasses 1
  { super_,   NULL,    PREC_NONE },       // TOKEN_SUPER
/*>= Compiling Expressions 1 < Methods and Initializers 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
*/
//>= Methods and Initializers 1
  { this_,    NULL,    PREC_NONE },       // TOKEN_THIS
/*>= Compiling Expressions 1 < Types of Values 1
  { NULL,     NULL,    PREC_NONE },       // TOKEN_TRUE
*/
//>= Types of Values 1
  { true_,    NULL,    PREC_NONE },       // TOKEN_TRUE
//>= Compiling Expressions 1
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
  
/*>= Compiling Expressions 1 < Global Variables 1
  prefixRule();
*/
//>= Global Variables 1
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);
//>= Compiling Expressions 1

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
/*>= Compiling Expressions 1 < Global Variables 1
    infixRule();
*/
//>= Global Variables 1
    infixRule(canAssign);
//>= Compiling Expressions 1
  }
//>= Global Variables 1
  
  if (canAssign && match(TOKEN_EQUAL)) {
    // If we get here, we didn't parse the "=" even though we could have, so
    // the LHS must not be a valid lvalue.
    error("Invalid assignment target.");
    expression();
  }
//>= Compiling Expressions 1
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}
//>= Local Variables 1

static void block() {
  consume(TOKEN_LEFT_BRACE, "Expect '{' before block.");

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }
  
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}
//>= Calls and Functions 1

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
/*>= Calls and Functions 1 < Closures 1
  emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
*/
//>= Closures 1
  
  // Capture the upvalues in the new closure object.
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));
  
  // Emit arguments for each upvalue to know whether to capture a local or
  // an upvalue.
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
//>= Calls and Functions 1
}
//>= Methods and Initializers 1

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
//>= Classes and Instances 1

static void classDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();
  
//>= Methods and Initializers 1
  ClassCompiler classCompiler;
  classCompiler.name = parser.previous;
//>= Superclasses 1
  classCompiler.hasSuperclass = false;
//>= Methods and Initializers 1
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;
  
/*>= Classes and Instances 1 < Superclasses 1
  emitBytes(OP_CLASS, nameConstant);
*/
//>= Superclasses 1
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
//>= Classes and Instances 1

  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
//>= Methods and Initializers 1
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }
//>= Classes and Instances 1
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
//>= Superclasses 1
  
  if (classCompiler.hasSuperclass) {
    endScope();
  }
//>= Classes and Instances 1
  defineVariable(nameConstant);
//>= Methods and Initializers 1
  
  currentClass = currentClass->enclosing;
//>= Classes and Instances 1
}
//>= Calls and Functions 1

static void funDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  function(TYPE_FUNCTION);
  defineVariable(global);
}
//>= Global Variables 1

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
//>= Global Variables 1

static void expressionStatement() {
  expression();
  emitByte(OP_POP);
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  
}
//>= Jumping Forward and Back 1

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
//>= Global Variables 1

static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}
//>= Calls and Functions 1

static void returnStatement() {
  if (current->type == TYPE_TOP_LEVEL) {
    error("Cannot return from top-level code.");
  }
  
  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
//>= Methods and Initializers 1
    if (current->type == TYPE_INITIALIZER) {
      error("Cannot return a value from an initializer.");
    }
    
//>= Calls and Functions 1
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}
//>= Jumping Forward and Back 1

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
//>= Global Variables 1

static void declaration() {
//>= Classes and Instances 1
  if (match(TOKEN_CLASS)) {
    classDeclaration();
/*>= Calls and Functions 1 < Classes and Instances 1
  if (match(TOKEN_FUN)) {
*/
//>= Classes and Instances 1
  } else if (match(TOKEN_FUN)) {
//>= Calls and Functions 1
    funDeclaration();
/*>= Global Variables 1 < Calls and Functions 1
  if (match(TOKEN_VAR)) {
*/
//>= Calls and Functions 1
  } else if (match(TOKEN_VAR)) {
//>= Global Variables 1
    varDeclaration();
  } else {
    statement();
  }
}

static void statement() {
/*>= Global Variables 1 < Jumping Forward and Back 1
  if (match(TOKEN_PRINT)) {
*/
//>= Jumping Forward and Back 1
  if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_PRINT)) {
//>= Global Variables 1
    printStatement();
//>= Calls and Functions 1
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
//>= Jumping Forward and Back 1
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
//>= Local Variables 1
  } else if (check(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
//>= Global Variables 1
  } else {
    expressionStatement();
  }
}
/*>= Scanning on Demand 1 < Compiling Expressions 1

void compile(const char* source) {
*/
/*>= Compiling Expressions 1 < Calls and Functions 1
 
bool compile(const char* source, Chunk* chunk) {
*/
//>= Calls and Functions 1

ObjFunction* compile(const char* source) {
//>= Scanning on Demand 1
  initScanner(source);
/*>= Scanning on Demand 1 < Compiling Expressions 1
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
//>= Local Variables 1
  Compiler mainCompiler;
/*>= Local Variables 1 < Calls and Functions 1
  initCompiler(&mainCompiler);
*/
//>= Calls and Functions 1
  initCompiler(&mainCompiler, 0, TYPE_TOP_LEVEL);
/*>= Compiling Expressions 1 < Calls and Functions 1
  compilingChunk = chunk;
*/
//>= Compiling Expressions 1
  
  // Prime the pump.
  parser.hadError = false;
  advance();

/*>= Compiling Expressions 1 < Global Variables 1
  expression();
  consume(TOKEN_EOF, "Expect end of expression.");
*/
//>= Global Variables 1
  if (!match(TOKEN_EOF)) {
    do {
      declaration();
    } while (!match(TOKEN_EOF));
  }

/*>= Compiling Expressions 1 < Calls and Functions 1
  endCompiler();
 
  // If there was a compile error, the code is not valid.
  return !parser.hadError;
*/
//>= Calls and Functions 1
  ObjFunction* function = endCompiler();
  
  // If there was a compile error, the code is not valid, so don't create a
  // function.
  return parser.hadError ? NULL : function;
//>= Scanning on Demand 1
}
//>= Garbage Collection 1

void grayCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    grayObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
