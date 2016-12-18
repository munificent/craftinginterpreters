//>= A Virtual Machine 99
#ifndef clox_vm_h
#define clox_vm_h

/*>= A Virtual Machine 99 < Calls and Functions 99
#include "chunk.h"
*/
//>= Calls and Functions 99
#include "object.h"
//>= Hash Tables 99
#include "table.h"
//>= A Virtual Machine 99
#include "value.h"

/*>= A Virtual Machine 99 < Calls and Functions 99
#define STACK_SIZE 256
*/
//>= Calls and Functions 99
// TODO: Don't depend on frame count for stack count since we have stack before
// frames?
#define FRAMES_SIZE 64
#define STACK_SIZE (FRAMES_SIZE * UINT8_COUNT)

typedef struct {
/*>= Calls and Functions 99 < Closures 99
  ObjFunction* function;
*/
//>= Closures 99
  ObjClosure* closure;
//>= Calls and Functions 99
  uint8_t* ip;
  Value* slots;
} CallFrame;
//>= A Virtual Machine 99

typedef struct {
  Value stack[STACK_SIZE];
  Value* stackTop;
/*>= A Virtual Machine 99 < Calls and Functions 99
  Chunk* chunk;
  uint8_t* ip;
*/
//>= Calls and Functions 99

  CallFrame frames[FRAMES_SIZE];
  int frameCount;

//>= Global Variables 99
  Table globals;
//>= Hash Tables 99
  Table strings;

//>= Methods and Initializers 99
  ObjString* initString;
//>= Closures 99
  ObjUpvalue* openUpvalues;
//>= Garbage Collection 99

  size_t bytesAllocated;
  size_t nextGC;
//>= Strings 99

  Obj* objects;
//>= Garbage Collection 99
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
//>= A Virtual Machine 99
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
/*>= A Virtual Machine 99 < Scanning on Demand 99
InterpretResult interpret(Chunk* chunk);
*/
//>= Scanning on Demand 99
InterpretResult interpret(const char* source);
//>= A Virtual Machine 99
void endVM();

#endif
