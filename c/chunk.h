//>= Chunks of Bytecode
#ifndef cvox_chunk_h
#define cvox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
//>= Types of Values
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
//>= Uhh
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_FIELD,
  OP_SET_FIELD,
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
//>= Uhh
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL_0,
  OP_CALL_1,
  OP_CALL_2,
  OP_CALL_3,
  OP_CALL_4,
  OP_CALL_5,
  OP_CALL_6,
  OP_CALL_7,
  OP_CALL_8,
  OP_INVOKE_0,
  OP_INVOKE_1,
  OP_INVOKE_2,
  OP_INVOKE_3,
  OP_INVOKE_4,
  OP_INVOKE_5,
  OP_INVOKE_6,
  OP_INVOKE_7,
  OP_INVOKE_8,
  OP_SUPER_0,
  OP_SUPER_1,
  OP_SUPER_2,
  OP_SUPER_3,
  OP_SUPER_4,
  OP_SUPER_5,
  OP_SUPER_6,
  OP_SUPER_7,
  OP_SUPER_8,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
//>= A Virtual Machine
  OP_RETURN,
//>= Uhh
  OP_CLASS,
  OP_SUBCLASS,
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
