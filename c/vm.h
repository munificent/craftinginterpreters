//>= A Virtual Machine
#ifndef cvox_vm_h
#define cvox_vm_h

/*>= A Virtual Machine <= Local Variables
#include "chunk.h"
*/
//>= Uhh
#include "object.h"
//>= Hash Tables
#include "table.h"
//>= A Virtual Machine
#include "value.h"

/*>= A Virtual Machine <= Local Variables
#define STACK_SIZE 256
*/
//>= Uhh
// TODO: Don't depend on frame count for stack count since we have stack before
// frames?
#define FRAMES_SIZE 64
#define STACK_SIZE (FRAMES_SIZE * UINT8_COUNT)

typedef struct {
  ObjClosure* closure;
  uint8_t* ip;
  Value* slots;
} CallFrame;
//>= A Virtual Machine

typedef struct {
  Value stack[STACK_SIZE];
  Value* stackTop;
/*>= A Virtual Machine <= Local Variables
  Chunk* chunk;
  uint8_t* ip;
*/
//>= Uhh
  
  CallFrame frames[FRAMES_SIZE];
  int frameCount;
  
//>= Global Variables
  Table globals;
//>= Hash Tables
  Table strings;

//>= Uhh
  ObjString* initString;
  
  ObjUpvalue* openUpvalues;
  
  size_t bytesAllocated;
  size_t nextGC;
//>= Strings
  
  Obj* objects;
//>= Uhh
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
//>= A Virtual Machine
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

// The singleton VM.
extern VM vm;

void initVM();
void push(Value value);
Value pop();
/*== A Virtual Machine
InterpretResult interpret(Chunk* chunk);
*/
//>= Scanning on Demand
InterpretResult interpret(const char* source);
//>= A Virtual Machine
void endVM();

#endif
