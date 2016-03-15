#ifndef cvox_vm_h
#define cvox_vm_h

#include "object.h"

#define MAX_STACK  256
#define MAX_HEAP   (10 * 1024 * 1024)

typedef enum {
  OP_CONSTANT,
  OP_NULL,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_RETURN,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_CALL_0,
  OP_CALL_1,
  OP_CALL_2,
  OP_CALL_3,
  OP_CALL_4,
  OP_CALL_5,
  OP_CALL_6,
  OP_CALL_7,
  OP_CALL_8
} OpCode;

struct sVM {
  Value stack[MAX_STACK];
  int stackSize;
  
  ObjTable* globals;
  
  size_t bytesAllocated;
  size_t nextGC;
  
  Obj* objects;
  
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
};

// The singleton VM.
extern VM vm;

void initVM();
bool interpret(const char* source);
void endVM();

#endif
