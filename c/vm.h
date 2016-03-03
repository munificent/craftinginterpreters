#ifndef cvox_vm_h
#define cvox_vm_h

#include "object.h"

#define MAX_STACK  256
#define MAX_HEAP   (10 * 1024 * 1024)

typedef enum {
  OP_CONSTANT,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_RETURN
} OpCode;

struct sVM {
  Value stack[MAX_STACK];
  int stackSize;
  
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
void interpret(const char* source);
void endVM();

#endif
