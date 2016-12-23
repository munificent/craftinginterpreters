//> Chunks of Bytecode not-yet
#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
//> Types of Values not-yet
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
//< Types of Values not-yet
//> Global Variables not-yet
  OP_POP,
//< Global Variables not-yet
//> Local Variables not-yet
  OP_GET_LOCAL,
  OP_SET_LOCAL,
//< Local Variables not-yet
//> Global Variables not-yet
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
//< Global Variables not-yet
//> Closures not-yet
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
//< Closures not-yet
//> Classes and Instances not-yet
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
//< Classes and Instances not-yet
//> Superclasses not-yet
  OP_GET_SUPER,
//< Superclasses not-yet
//> Types of Values not-yet
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
//< Types of Values not-yet
//> A Virtual Machine not-yet
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
//< A Virtual Machine not-yet
//> Types of Values not-yet
  OP_NOT,
//< Types of Values not-yet
//> A Virtual Machine not-yet
  OP_NEGATE,
//< A Virtual Machine not-yet
//> Global Variables not-yet
  OP_PRINT,
//< Global Variables not-yet
//> Jumping Forward and Back not-yet
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
//< Jumping Forward and Back not-yet
//> Calls and Functions not-yet
  OP_CALL_0,
  OP_CALL_1,
  OP_CALL_2,
  OP_CALL_3,
  OP_CALL_4,
  OP_CALL_5,
  OP_CALL_6,
  OP_CALL_7,
  OP_CALL_8,
//< Calls and Functions not-yet
//> Methods and Initializers not-yet
  OP_INVOKE_0,
  OP_INVOKE_1,
  OP_INVOKE_2,
  OP_INVOKE_3,
  OP_INVOKE_4,
  OP_INVOKE_5,
  OP_INVOKE_6,
  OP_INVOKE_7,
  OP_INVOKE_8,
//< Methods and Initializers not-yet
//> Superclasses not-yet
  OP_SUPER_0,
  OP_SUPER_1,
  OP_SUPER_2,
  OP_SUPER_3,
  OP_SUPER_4,
  OP_SUPER_5,
  OP_SUPER_6,
  OP_SUPER_7,
  OP_SUPER_8,
//< Superclasses not-yet
//> Closures not-yet
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
//< Closures not-yet
//> A Virtual Machine not-yet
  OP_RETURN,
//< A Virtual Machine not-yet
//> Classes and Instances not-yet
  OP_CLASS,
//< Classes and Instances not-yet
//> Superclasses not-yet
  OP_SUBCLASS,
//< Superclasses not-yet
//> Methods and Initializers not-yet
  OP_METHOD
//< Methods and Initializers not-yet
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;
  int* lines;
  ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);

void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

#endif
