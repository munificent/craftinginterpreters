//>= A Virtual Machine
#ifndef cvox_vm_h
#define cvox_vm_h
//>= Uhh

#include "object.h"
#include "table.h"
//>= A Virtual Machine
#include "value.h"

/*>= A Virtual Machine <= Scanning Without Allocating
#define STACK_SIZE 256
*/
//>= Uhh
// TODO: Don't depend on frame count for stack count since we have stack before
// frames?
#define FRAMES_SIZE 64
#define STACK_SIZE (FRAMES_SIZE * UINT8_COUNT)
//>= A Virtual Machine

typedef enum {
  OP_CONSTANT,
//>= Uhh
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_FIELD,
  OP_SET_FIELD,
  OP_GET_SUPER,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
//>= A Virtual Machine
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
//>= Uhh
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL_0,
  OP_CALL_1,
  OP_CALL_2,
  OP_CALL_3,
  OP_CALL_4,
  OP_CALL_5,
  OP_CALL_6,
  OP_CALL_7,
  OP_CALL_8,
  OP_INVOKE_0,
  OP_INVOKE_1,
  OP_INVOKE_2,
  OP_INVOKE_3,
  OP_INVOKE_4,
  OP_INVOKE_5,
  OP_INVOKE_6,
  OP_INVOKE_7,
  OP_INVOKE_8,
  OP_SUPER_0,
  OP_SUPER_1,
  OP_SUPER_2,
  OP_SUPER_3,
  OP_SUPER_4,
  OP_SUPER_5,
  OP_SUPER_6,
  OP_SUPER_7,
  OP_SUPER_8,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
//>= A Virtual Machine
  OP_RETURN,
//>= Uhh
  OP_CLASS,
  OP_SUBCLASS,
  OP_METHOD
//>= A Virtual Machine
} OpCode;
//>= Uhh

typedef struct {
  ObjClosure* closure;
  uint8_t* ip;
  Value* slots;
} CallFrame;
//>= A Virtual Machine

typedef struct {
  Value stack[STACK_SIZE];
  Value* stackTop;
/*>= A Virtual Machine <= Scanning Without Allocating
  uint8_t* bytecode;
  double* constants;
*/
//>= Uhh
  
  CallFrame frames[FRAMES_SIZE];
  int frameCount;
  
  Table globals;
  Table strings;

  ObjString* initString;
  
  ObjUpvalue* openUpvalues;
  
  size_t bytesAllocated;
  size_t nextGC;
  
  Obj* objects;
  
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
InterpretResult interpret(uint8_t* bytecode, double* constants);
*/
//>= Scanning Without Allocating
InterpretResult interpret(const char* source);
//>= A Virtual Machine
void endVM();

#endif
