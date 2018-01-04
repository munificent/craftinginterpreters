//> Chunks of Bytecode debug-c
#include <stdio.h>
//> Calls and Functions not-yet
#include <string.h>
//< Calls and Functions not-yet

#include "debug.h"
//> Strings not-yet
#include "object.h"
//< Strings not-yet
//> debug-include-value
#include "value.h"
//< debug-include-value

void disassembleChunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);
  
  for (int i = 0; i < chunk->count;) {
    i = disassembleInstruction(chunk, i);
  }
}
//> constant-instruction

static int constantInstruction(const char* name, Chunk* chunk, int i) {
  uint8_t constant = chunk->code[i];
  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return i + 1;
}

//< constant-instruction
//> Methods and Initializers not-yet
static int constantInstructionN(const char* name, int n, Chunk* chunk, int i) {
  uint8_t constant = chunk->code[i];
  printf("%s_%-*d %4d '", name, 15 - (int)strlen(name), n, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return i + 1;
}
//< Methods and Initializers not-yet
//> simple-instruction
static int simpleInstruction(const char* name, int i) {
  printf("%s\n", name);
  return i;
}
//< simple-instruction
//> Calls and Functions not-yet
static int simpleInstructionN(const char* name, int n, int i) {
  printf("%s_%d\n", name, n);
  return i;
}
//< Calls and Functions not-yet
//> Local Variables not-yet
static int byteInstruction(const char* name, Chunk* chunk, int i) {
  uint8_t slot = chunk->code[i];
  printf("%-16s %4d\n", name, slot);
  return i + 1;
}
//< Local Variables not-yet
//> Jumping Forward and Back not-yet
static int jumpInstruction(const char* name, int sign, Chunk* chunk, int i) {
  uint16_t offset = (uint16_t)(chunk->code[i] << 8);
  offset |= chunk->code[i + 1];
  printf("%-16s %4d -> %d\n", name, offset, i + sign * offset);
  return i + 2;
}
//< Jumping Forward and Back not-yet
//> disassemble-instruction
int disassembleInstruction(Chunk* chunk, int i) {
  printf("%04d ", i);
//> show-location
  if (i > 0 && chunk->lines[i] == chunk->lines[i - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[i]);
  }
//< show-location
  
  uint8_t* code = chunk->code;
  uint8_t instruction = code[i++];
  switch (instruction) {
//> disassemble-constant
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, i);
//< disassemble-constant
//> Types of Values not-yet
    case OP_NIL:
      return simpleInstruction("OP_NIL", i);
    case OP_TRUE:
      return simpleInstruction("OP_TRUE", i);
    case OP_FALSE:
      return simpleInstruction("OP_FALSE", i);
//< Types of Values not-yet
//> Global Variables not-yet
    case OP_POP:
      return simpleInstruction("OP_POP", i);
//< Global Variables not-yet
//> Local Variables not-yet
    case OP_GET_LOCAL:
      return byteInstruction("OP_GET_LOCAL", chunk, i);
    case OP_SET_LOCAL:
      return byteInstruction("OP_SET_LOCAL", chunk, i);
//< Local Variables not-yet
//> Global Variables not-yet
    case OP_GET_GLOBAL:
      return constantInstruction("OP_GET_GLOBAL", chunk, i);
    case OP_DEFINE_GLOBAL:
      return constantInstruction("OP_DEFINE_GLOBAL", chunk, i);
    case OP_SET_GLOBAL:
      return constantInstruction("OP_SET_GLOBAL", chunk, i);
//< Global Variables not-yet
//> Closures not-yet
    case OP_GET_UPVALUE:
      return byteInstruction("OP_GET_UPVALUE", chunk, i);
    case OP_SET_UPVALUE:
      return byteInstruction("OP_SET_UPVALUE", chunk, i);
//< Closures not-yet
//> Classes and Instances not-yet
    case OP_GET_PROPERTY:
      return constantInstruction("OP_GET_PROPERTY", chunk, i);
    case OP_SET_PROPERTY:
      return constantInstruction("OP_SET_PROPERTY", chunk, i);
//< Classes and Instances not-yet
//> Superclasses not-yet
    case OP_GET_SUPER:
      return constantInstruction("OP_GET_SUPER", chunk, i);
//< Superclasses not-yet
//> Types of Values not-yet
    case OP_EQUAL:
      return simpleInstruction("OP_EQUAL", i);
    case OP_GREATER:
      return simpleInstruction("OP_GREATER", i);
    case OP_LESS:
      return simpleInstruction("OP_LESS", i);
//< Types of Values not-yet
//> A Virtual Machine not-yet
    case OP_ADD:
      return simpleInstruction("OP_ADD", i);
    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", i);
    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", i);
    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", i);
//< A Virtual Machine not-yet
//> Types of Values not-yet
    case OP_NOT:
      return simpleInstruction("OP_NOT", i);
//< Types of Values not-yet
//> A Virtual Machine not-yet
    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", i);
//< A Virtual Machine not-yet
//> Global Variables not-yet
    case OP_PRINT:
      return simpleInstruction("OP_PRINT", i);
//< Global Variables not-yet
//> Jumping Forward and Back not-yet
    case OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, i);
    case OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, i);
    case OP_LOOP:
      return jumpInstruction("OP_LOOP", -1, chunk, i);
//< Jumping Forward and Back not-yet
//> Calls and Functions not-yet
    case OP_CALL_0:
    case OP_CALL_1:
    case OP_CALL_2:
    case OP_CALL_3:
    case OP_CALL_4:
    case OP_CALL_5:
    case OP_CALL_6:
    case OP_CALL_7:
    case OP_CALL_8:
      return simpleInstructionN("OP_CALL", instruction - OP_CALL_0, i);
//< Calls and Functions not-yet
//> Methods and Initializers not-yet
    case OP_INVOKE_0:
    case OP_INVOKE_1:
    case OP_INVOKE_2:
    case OP_INVOKE_3:
    case OP_INVOKE_4:
    case OP_INVOKE_5:
    case OP_INVOKE_6:
    case OP_INVOKE_7:
    case OP_INVOKE_8:
      return constantInstructionN(
          "OP_INVOKE_", instruction - OP_INVOKE_0, chunk, i);
//< Methods and Initializers not-yet
//> Superclasses not-yet
    case OP_SUPER_0:
    case OP_SUPER_1:
    case OP_SUPER_2:
    case OP_SUPER_3:
    case OP_SUPER_4:
    case OP_SUPER_5:
    case OP_SUPER_6:
    case OP_SUPER_7:
    case OP_SUPER_8:
      return constantInstructionN(
          "OP_SUPER_", instruction - OP_SUPER_0, chunk, i);
//< Superclasses not-yet
//> Closures not-yet

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
      
      return i;
    }

    case OP_CLOSE_UPVALUE:
      return simpleInstruction("OP_CLOSE_UPVALUE", i);
//< Closures not-yet
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", i);
//> Classes and Instances not-yet
    case OP_CLASS:
      return constantInstruction("OP_CLASS", chunk, i);
//< Classes and Instances not-yet
//> Superclasses not-yet
    case OP_SUBCLASS:
      return constantInstruction("OP_SUBCLASS", chunk, i);
//< Superclasses not-yet
//> Methods and Initializers not-yet
    case OP_METHOD:
      return constantInstruction("OP_METHOD", chunk, i);
//< Methods and Initializers not-yet
  }

  return 0; // Unreachable.
}
//< disassemble-instruction
