//>= Chunks of Bytecode
#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
//>= Types of Values
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
//>= Statements
  OP_POP,
//>= Local Variables
  OP_GET_LOCAL,
  OP_SET_LOCAL,
//>= Global Variables
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
//>= Closures
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
//>= Classes and Instances
  OP_GET_FIELD,
  OP_SET_FIELD,
//>= Inheritance
  OP_GET_SUPER,
//>= Types of Values
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
//>= A Virtual Machine
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
//>= Types of Values
  OP_NOT,
//>= A Virtual Machine
  OP_NEGATE,
//>= Statements
  OP_PRINT,
//>= Jumping Forward and Back
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
//>= Functions
  OP_CALL_0,
  OP_CALL_1,
  OP_CALL_2,
  OP_CALL_3,
  OP_CALL_4,
  OP_CALL_5,
  OP_CALL_6,
  OP_CALL_7,
  OP_CALL_8,
//>= Methods and Initializers
  OP_INVOKE_0,
  OP_INVOKE_1,
  OP_INVOKE_2,
  OP_INVOKE_3,
  OP_INVOKE_4,
  OP_INVOKE_5,
  OP_INVOKE_6,
  OP_INVOKE_7,
  OP_INVOKE_8,
//>= Inheritance
  OP_SUPER_0,
  OP_SUPER_1,
  OP_SUPER_2,
  OP_SUPER_3,
  OP_SUPER_4,
  OP_SUPER_5,
  OP_SUPER_6,
  OP_SUPER_7,
  OP_SUPER_8,
//>= Closures
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
//>= A Virtual Machine
  OP_RETURN,
//>= Classes and Instances
  OP_CLASS,
//>= Inheritance
  OP_SUBCLASS,
//>= Methods and Initializers
  OP_METHOD
//>= Chunks of Bytecode
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
