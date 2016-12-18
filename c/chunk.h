//> Chunks of Bytecode 99
#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
//> Types of Values 99
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
//< Types of Values 99
//> Global Variables 99
  OP_POP,
//< Global Variables 99
//> Local Variables 99
  OP_GET_LOCAL,
  OP_SET_LOCAL,
//< Local Variables 99
//> Global Variables 99
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
//< Global Variables 99
//> Closures 99
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
//< Closures 99
//> Classes and Instances 99
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
//< Classes and Instances 99
//> Superclasses 99
  OP_GET_SUPER,
//< Superclasses 99
//> Types of Values 99
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
//< Types of Values 99
//> A Virtual Machine 99
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
//< A Virtual Machine 99
//> Types of Values 99
  OP_NOT,
//< Types of Values 99
//> A Virtual Machine 99
  OP_NEGATE,
//< A Virtual Machine 99
//> Global Variables 99
  OP_PRINT,
//< Global Variables 99
//> Jumping Forward and Back 99
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
//< Jumping Forward and Back 99
//> Calls and Functions 99
  OP_CALL_0,
  OP_CALL_1,
  OP_CALL_2,
  OP_CALL_3,
  OP_CALL_4,
  OP_CALL_5,
  OP_CALL_6,
  OP_CALL_7,
  OP_CALL_8,
//< Calls and Functions 99
//> Methods and Initializers 99
  OP_INVOKE_0,
  OP_INVOKE_1,
  OP_INVOKE_2,
  OP_INVOKE_3,
  OP_INVOKE_4,
  OP_INVOKE_5,
  OP_INVOKE_6,
  OP_INVOKE_7,
  OP_INVOKE_8,
//< Methods and Initializers 99
//> Superclasses 99
  OP_SUPER_0,
  OP_SUPER_1,
  OP_SUPER_2,
  OP_SUPER_3,
  OP_SUPER_4,
  OP_SUPER_5,
  OP_SUPER_6,
  OP_SUPER_7,
  OP_SUPER_8,
//< Superclasses 99
//> Closures 99
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
//< Closures 99
//> A Virtual Machine 99
  OP_RETURN,
//< A Virtual Machine 99
//> Classes and Instances 99
  OP_CLASS,
//< Classes and Instances 99
//> Superclasses 99
  OP_SUBCLASS,
//< Superclasses 99
//> Methods and Initializers 99
  OP_METHOD
//< Methods and Initializers 99
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
