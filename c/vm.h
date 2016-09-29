//>= A Virtual Machine
#ifndef clox_vm_h
#define clox_vm_h

/*>= A Virtual Machine < Calls and Functions
#include "chunk.h"
*/
//>= Calls and Functions
#include "object.h"
//>= Hash Tables
#include "table.h"
//>= A Virtual Machine
#include "value.h"

/*>= A Virtual Machine < Calls and Functions
#define STACK_SIZE 256
*/
//>= Calls and Functions
// TODO: Don't depend on frame count for stack count since we have stack before
// frames?
#define FRAMES_SIZE 64
#define STACK_SIZE (FRAMES_SIZE * UINT8_COUNT)

typedef struct {
/*== Calls and Functions
  ObjFunction* function;
*/
//>= Closures
  ObjClosure* closure;
//>= Calls and Functions
  uint8_t* ip;
  Value* slots;
} CallFrame;
//>= A Virtual Machine

typedef struct {
  Value stack[STACK_SIZE];
  Value* stackTop;
/*>= A Virtual Machine < Calls and Functions
  Chunk* chunk;
  uint8_t* ip;
*/
//>= Calls and Functions

  CallFrame frames[FRAMES_SIZE];
  int frameCount;

//>= Global Variables
  Table globals;
//>= Hash Tables
  Table strings;

//>= Methods and Initializers
  ObjString* initString;
//>= Closures
  ObjUpvalue* openUpvalues;
//>= Garbage Collection

  size_t bytesAllocated;
  size_t nextGC;
//>= Strings

  Obj* objects;
//>= Garbage Collection
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
