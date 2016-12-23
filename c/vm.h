//> A Virtual Machine not-yet
#ifndef clox_vm_h
#define clox_vm_h

/* A Virtual Machine not-yet < Calls and Functions not-yet
#include "chunk.h"
*/
//> Calls and Functions not-yet
#include "object.h"
//< Calls and Functions not-yet
//> Hash Tables not-yet
#include "table.h"
//< Hash Tables not-yet
#include "value.h"

/* A Virtual Machine not-yet < Calls and Functions not-yet
#define STACK_SIZE 256
*/
//> Calls and Functions not-yet
// TODO: Don't depend on frame count for stack count since we have stack before
// frames?
#define FRAMES_SIZE 64
#define STACK_SIZE (FRAMES_SIZE * UINT8_COUNT)

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
  Value stack[STACK_SIZE];
  Value* stackTop;
/* A Virtual Machine not-yet < Calls and Functions not-yet
  Chunk* chunk;
  uint8_t* ip;
*/
//> Calls and Functions not-yet

  CallFrame frames[FRAMES_SIZE];
  int frameCount;

//< Calls and Functions not-yet
//> Global Variables not-yet
  Table globals;
//< Global Variables not-yet
//> Hash Tables not-yet
  Table strings;

//< Hash Tables not-yet
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
//> Strings not-yet

  Obj* objects;
//< Strings not-yet
//> Garbage Collection not-yet
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
//< Garbage Collection not-yet
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
/* A Virtual Machine not-yet < Scanning on Demand not-yet
InterpretResult interpret(Chunk* chunk);
*/
//> Scanning on Demand not-yet
InterpretResult interpret(const char* source);
//< Scanning on Demand not-yet
void endVM();

#endif
