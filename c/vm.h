#ifndef cvox_vm_h
#define cvox_vm_h

#include "object.h"

#define MAX_STACK  256
#define MAX_HEAP   (10 * 1024 * 1024)

typedef enum {
  OP_CONSTANT,
  OP_ADD
} OpCode;

struct sVM {
  Value stack[MAX_STACK];
  int stackSize;
  
  char* fromStart;
  char* fromEnd;
  char* toStart;
  char* toEnd;
};

void initVM(VM* vm);

// TODO: Temp.
void pushNumber(VM* vm, double value);
void printStack(VM* vm);
void pop(VM* vm);

#endif
