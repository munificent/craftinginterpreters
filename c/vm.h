//> A Virtual Machine vm-h
#ifndef clox_vm_h
#define clox_vm_h

/* A Virtual Machine vm-h < Calls and Functions vm-include-object
#include "chunk.h"
*/
//> Calls and Functions vm-include-object
#include "object.h"
//< Calls and Functions vm-include-object
//> Hash Tables vm-include-table
#include "table.h"
//< Hash Tables vm-include-table
//> vm-include-value
#include "value.h"
//< vm-include-value
//> stack-max

//< stack-max
/* A Virtual Machine stack-max < Calls and Functions frame-max
#define STACK_MAX 256
*/
//> Calls and Functions frame-max
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
//< Calls and Functions frame-max
//> Calls and Functions call-frame

typedef struct {
/* Calls and Functions call-frame < Closures call-frame-closure
  ObjFunction* function;
*/
//> Closures call-frame-closure
  ObjClosure* closure;
//< Closures call-frame-closure
  uint8_t* ip;
  Value* slots;
} CallFrame;
//< Calls and Functions call-frame

typedef struct {
/* A Virtual Machine vm-h < Calls and Functions frame-array
  Chunk* chunk;
*/
/* A Virtual Machine ip < Calls and Functions frame-array
  uint8_t* ip;
*/
//> Calls and Functions frame-array
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  
//< Calls and Functions frame-array
//> vm-stack
  Value stack[STACK_MAX];
  Value* stackTop;
//< vm-stack
//> Global Variables vm-globals
  Table globals;
//< Global Variables vm-globals
//> Hash Tables vm-strings
  Table strings;
//< Hash Tables vm-strings
//> Methods and Initializers vm-init-string
  ObjString* initString;
//< Methods and Initializers vm-init-string
//> Closures open-upvalues-field
  ObjUpvalue* openUpvalues;
//< Closures open-upvalues-field
//> Garbage Collection vm-fields

  size_t bytesAllocated;
  size_t nextGC;
//< Garbage Collection vm-fields
//> Strings objects-root
  Obj* objects;
//< Strings objects-root
//> Garbage Collection vm-gray-stack
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
//< Garbage Collection vm-gray-stack
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
