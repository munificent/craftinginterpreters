//> A Virtual Machine vm-h
#ifndef clox_vm_h
#define clox_vm_h

/* A Virtual Machine vm-h < Calls and Functions not-yet
#include "chunk.h"
*/
//> Calls and Functions not-yet
#include "object.h"
//< Calls and Functions not-yet
//> Hash Tables vm-include-table
#include "table.h"
//< Hash Tables vm-include-table
//> vm-include-value
#include "value.h"
//< vm-include-value
/* A Virtual Machine stack-max < Calls and Functions not-yet

#define STACK_MAX 256
*/
//> Calls and Functions not-yet
// TODO: Don't depend on frame count for stack count since we have
// stack before frames?
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
/* Calls and Functions not-yet < Closures not-yet
  ObjFunction* function;
*/
//> Closures not-yet
  ObjClosure* closure;
//< Closures not-yet
  uint8_t* ip;
  Value* slots;
} CallFrame;
//< Calls and Functions not-yet

typedef struct {
/* A Virtual Machine vm-h < Calls and Functions not-yet
  Chunk* chunk;
*/
/* A Virtual Machine ip < Calls and Functions not-yet
  uint8_t* ip;
*/
//> vm-stack
  Value stack[STACK_MAX];
  Value* stackTop;
//< vm-stack
//> Calls and Functions not-yet

  CallFrame frames[FRAMES_MAX];
  int frameCount;

//< Calls and Functions not-yet
//> Global Variables not-yet
  Table globals;
//< Global Variables not-yet
//> Hash Tables vm-strings
  Table strings;
//< Hash Tables vm-strings
//> Methods and Initializers not-yet
  ObjString* initString;
//< Methods and Initializers not-yet
//> Closures not-yet
  ObjUpvalue* openUpvalues;
//< Closures not-yet
//> Garbage Collection not-yet

  size_t bytesAllocated;
  size_t nextGC;
//< Garbage Collection not-yet
//> Strings objects-root

  Obj* objects;
//< Strings objects-root
//> Garbage Collection not-yet
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
//< Garbage Collection not-yet
} VM;

//> interpret-result
typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

//< interpret-result
//> Strings extern-vm
extern VM vm;

//< Strings extern-vm
void initVM();
void freeVM();
/* A Virtual Machine interpret-h < Scanning on Demand vm-interpret-h
InterpretResult interpret(Chunk* chunk);
*/
//> Scanning on Demand vm-interpret-h
InterpretResult interpret(const char* source);
//< Scanning on Demand vm-interpret-h
//> push-pop
void push(Value value);
Value pop();
//< push-pop

#endif
