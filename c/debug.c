//>= Chunks of Bytecode
#include <stdio.h>

#include "debug.h"
//>= Strings
#include "object.h"
//>= Chunks of Bytecode
#include "value.h"

int disassembleInstruction(Chunk* chunk, int i) {
  printf("%04d ", i);
  if (i > 0 && chunk->lines[i] == chunk->lines[i - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[i]);
  }
  
  uint8_t* code = chunk->code;
  uint8_t instruction = code[i++];

//>= A Virtual Machine
#define INST_ZERO(name) \
    case name: printf(#name "\n"); break;
  
//>= Uhh
#define INST_BYTE(name) \
    case name: { \
      uint8_t slot = code[i++]; \
      printf("%-16s %4d\n", #name, slot); \
      break; \
    }
  
#define INST_JUMP(name, sign) \
    case name: { \
      uint16_t offset = (uint16_t)(code[i++] << 8); \
      offset |= code[i++]; \
      printf("%-16s %4d -> %d\n", #name, offset, i sign offset); \
      break; \
    }
  
//>= Chunks of Bytecode
#define INST_CONSTANT(name) \
    case name: {\
      uint8_t constant = chunk->code[i++]; \
      printf("%-16s %4d '", #name, constant); \
      printValue(chunk->constants.values[constant]); \
      printf("'\n"); \
      break; \
    }
  
  switch (instruction) {
    INST_CONSTANT(OP_CONSTANT)
//>= Types of Values
    INST_ZERO(OP_NIL)
    INST_ZERO(OP_TRUE)
    INST_ZERO(OP_FALSE)
//>= Uhh
    INST_ZERO(OP_POP)
    INST_BYTE(OP_GET_LOCAL)
    INST_BYTE(OP_SET_LOCAL)
    INST_CONSTANT(OP_GET_GLOBAL)
    INST_CONSTANT(OP_DEFINE_GLOBAL)
    INST_CONSTANT(OP_SET_GLOBAL)
    INST_BYTE(OP_GET_UPVALUE)
    INST_BYTE(OP_SET_UPVALUE)
    INST_CONSTANT(OP_GET_FIELD)
    INST_CONSTANT(OP_SET_FIELD)
    INST_CONSTANT(OP_GET_SUPER)
//>= Types of Values
    INST_ZERO(OP_EQUAL)
    INST_ZERO(OP_GREATER)
    INST_ZERO(OP_LESS)
//>= A Virtual Machine
    INST_ZERO(OP_ADD)
    INST_ZERO(OP_SUBTRACT)
    INST_ZERO(OP_MULTIPLY)
    INST_ZERO(OP_DIVIDE)
//>= Types of Values
    INST_ZERO(OP_NOT)
//>= A Virtual Machine
    INST_ZERO(OP_NEGATE)
//>= Uhh
    INST_ZERO(OP_PRINT)
    INST_JUMP(OP_JUMP, +)
    INST_JUMP(OP_JUMP_IF_FALSE, +)
    INST_JUMP(OP_LOOP, -)
    INST_ZERO(OP_CALL_0)
    INST_ZERO(OP_CALL_1)
    INST_ZERO(OP_CALL_2)
    INST_ZERO(OP_CALL_3)
    INST_ZERO(OP_CALL_4)
    INST_ZERO(OP_CALL_5)
    INST_ZERO(OP_CALL_6)
    INST_ZERO(OP_CALL_7)
    INST_ZERO(OP_CALL_8)
    INST_CONSTANT(OP_INVOKE_0)
    INST_CONSTANT(OP_INVOKE_1)
    INST_CONSTANT(OP_INVOKE_2)
    INST_CONSTANT(OP_INVOKE_3)
    INST_CONSTANT(OP_INVOKE_4)
    INST_CONSTANT(OP_INVOKE_5)
    INST_CONSTANT(OP_INVOKE_6)
    INST_CONSTANT(OP_INVOKE_7)
    INST_CONSTANT(OP_INVOKE_8)
    INST_CONSTANT(OP_SUPER_0)
    INST_CONSTANT(OP_SUPER_1)
    INST_CONSTANT(OP_SUPER_2)
    INST_CONSTANT(OP_SUPER_3)
    INST_CONSTANT(OP_SUPER_4)
    INST_CONSTANT(OP_SUPER_5)
    INST_CONSTANT(OP_SUPER_6)
    INST_CONSTANT(OP_SUPER_7)
    INST_CONSTANT(OP_SUPER_8)
      
    case OP_CLOSURE: {
      uint8_t constant = code[i++];
      printf("%-16s %4d ", "OP_CLOSURE", constant);
      printValue(chunk->constants.values[constant]);
      printf("\n");
      
      ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
      for (int j = 0; j < function->upvalueCount; j++) {
        int isLocal = code[i++];
        int index = code[i++];
        printf("%04d   |                     %s %d\n",
               i - 2, isLocal ? "local" : "upvalue", index);
      }
      break;
    }
      
    INST_ZERO(OP_CLOSE_UPVALUE)
//>= A Virtual Machine
    INST_ZERO(OP_RETURN)
//>= Uhh
    INST_CONSTANT(OP_CLASS)
    INST_CONSTANT(OP_SUBCLASS)
    INST_CONSTANT(OP_METHOD)
//>= Chunks of Bytecode
  }
  
  return i;
}

void disassembleChunk(Chunk* chunk, const char* name) {
  printf("-- %s --\n", name);

  for (int i = 0; i < chunk->count;) {
    i = disassembleInstruction(chunk, i);
  }
}

void printValue(Value value) {
/*>= Chunks of Bytecode <= Compiling Expressions
  printf("%g", value);
*/
//>= Types of Values
  switch (value.type) {
    case VAL_BOOL:   printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL:    printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
//>= Strings
    case VAL_OBJ:
      switch (OBJ_TYPE(value)) {
//>= Uhh
        case OBJ_CLASS:
          printf("%s", AS_CLASS(value)->name->chars);
          break;
          
        case OBJ_BOUND_METHOD:
        case OBJ_CLOSURE:
        case OBJ_FUNCTION:
          printf("<fn %p>", AS_FUNCTION(value));
          break;
          
        case OBJ_INSTANCE:
          printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
          break;
          
        case OBJ_NATIVE:
          printf("<native %p>", AS_NATIVE(value));
          break;
//>= Strings
        case OBJ_STRING:
          printf("%s", AS_CSTRING(value));
          break;
//>= Uhh
        case OBJ_UPVALUE:
          printf("upvalue");
          break;
//>= Strings
      }
      break;
//>= Types of Values
  }
//>= Chunks of Bytecode
}
