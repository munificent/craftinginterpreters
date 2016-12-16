//>= A Virtual Machine 1
#ifndef clox_vm_h
#define clox_vm_h

/*>= A Virtual Machine 1 < Calls and Functions 1
#include "chunk.h"
*/
//>= Calls and Functions 1
#include "object.h"
//>= Hash Tables 1
#include "table.h"
//>= A Virtual Machine 1
#include "value.h"

/*>= A Virtual Machine 1 < Calls and Functions 1
#define STACK_SIZE 256
*/
//>= Calls and Functions 1
// TODO: Don't depend on frame count for stack count since we have stack before
// frames?
#define FRAMES_SIZE 64
#define STACK_SIZE (FRAMES_SIZE * UINT8_COUNT)

typedef struct {
/*>= Calls and Functions 1 < Closures 1
  ObjFunction* function;
*/
//>= Closures 1
  ObjClosure* closure;
//>= Calls and Functions 1
  uint8_t* ip;
  Value* slots;
} CallFrame;
//>= A Virtual Machine 1

typedef struct {
  Value stack[STACK_SIZE];
  Value* stackTop;
/*>= A Virtual Machine 1 < Calls and Functions 1
  Chunk* chunk;
  uint8_t* ip;
*/
//>= Calls and Functions 1

  CallFrame frames[FRAMES_SIZE];
  int frameCount;

//>= Global Variables 1
  Table globals;
//>= Hash Tables 1
  Table strings;

//>= Methods and Initializers 1
  ObjString* initString;
//>= Closures 1
  ObjUpvalue* openUpvalues;
//>= Garbage Collection 1

  size_t bytesAllocated;
  size_t nextGC;
//>= Strings 1

  Obj* objects;
//>= Garbage Collection 1
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
//>= A Virtual Machine 1
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
/*>= A Virtual Machine 1 < Scanning on Demand 1
InterpretResult interpret(Chunk* chunk);
*/
//>= Scanning on Demand 1
InterpretResult interpret(const char* source);
//>= A Virtual Machine 1
void endVM();

#endif
