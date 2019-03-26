//> Chunks of Bytecode debug-c
#include <stdio.h>
//> Calls and Functions not-yet
#include <string.h>
//< Calls and Functions not-yet

#include "debug.h"
//> Calls and Functions not-yet
#include "object.h"
//< Calls and Functions not-yet
//> debug-include-value
#include "value.h"
//< debug-include-value

void disassembleChunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);
  
  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}
//> constant-instruction
static int constantInstruction(const char* name, Chunk* chunk,
                               int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
//> return-after-operand
  return offset + 2;
//< return-after-operand
}
//< constant-instruction
//> Methods and Initializers not-yet
static int constantInstructionN(const char* name, int n, Chunk* chunk,
                                int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%s_%-*d %4d '", name, 15 - (int)strlen(name), n, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}
//< Methods and Initializers not-yet
//> simple-instruction
static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}
//< simple-instruction
//> Calls and Functions not-yet
static int simpleInstructionN(const char* name, int n, int offset) {
  printf("%s_%d\n", name, n);
  return offset + 1;
}
//< Calls and Functions not-yet
//> Local Variables byte-instruction
static int byteInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2; // [debug]
}
//< Local Variables byte-instruction
//> Jumping Forward and Back jump-instruction
static int jumpInstruction(const char* name, int sign, Chunk* chunk,
                           int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
}
//< Jumping Forward and Back jump-instruction
//> disassemble-instruction
int disassembleInstruction(Chunk* chunk, int offset) {
  printf("%04d ", offset);
//> show-location
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }
//< show-location
  
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
//> disassemble-constant
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);
//< disassemble-constant
//> Types of Values disassemble-literals
    case OP_NIL:
      return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:
      return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:
      return simpleInstruction("OP_FALSE", offset);
//< Types of Values disassemble-literals
//> Global Variables disassemble-pop
    case OP_POP:
      return simpleInstruction("OP_POP", offset);
//< Global Variables disassemble-pop
//> Local Variables disassemble-local
    case OP_GET_LOCAL:
      return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
      return byteInstruction("OP_SET_LOCAL", chunk, offset);
//< Local Variables disassemble-local
//> Global Variables disassemble-get-global
    case OP_GET_GLOBAL:
      return constantInstruction("OP_GET_GLOBAL", chunk, offset);
//< Global Variables disassemble-get-global
//> Global Variables disassemble-define-global
    case OP_DEFINE_GLOBAL:
      return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
//< Global Variables disassemble-define-global
//> Global Variables disassemble-set-global
    case OP_SET_GLOBAL:
      return constantInstruction("OP_SET_GLOBAL", chunk, offset);
//< Global Variables disassemble-set-global
//> Closures not-yet
    case OP_GET_UPVALUE:
      return byteInstruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
      return byteInstruction("OP_SET_UPVALUE", chunk, offset);
//< Closures not-yet
//> Classes and Instances not-yet
    case OP_GET_PROPERTY:
      return constantInstruction("OP_GET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY:
      return constantInstruction("OP_SET_PROPERTY", chunk, offset);
//< Classes and Instances not-yet
//> Superclasses not-yet
    case OP_GET_SUPER:
      return constantInstruction("OP_GET_SUPER", chunk, offset);
//< Superclasses not-yet
//> Types of Values disassemble-comparison
    case OP_EQUAL:
      return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER:
      return simpleInstruction("OP_GREATER", offset);
    case OP_LESS:
      return simpleInstruction("OP_LESS", offset);
//< Types of Values disassemble-comparison
//> A Virtual Machine disassemble-binary
    case OP_ADD:
      return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);
//> Types of Values disassemble-not
    case OP_NOT:
      return simpleInstruction("OP_NOT", offset);
//< Types of Values disassemble-not
//< A Virtual Machine disassemble-binary
//> A Virtual Machine disassemble-negate
    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);
//< A Virtual Machine disassemble-negate
//> Global Variables disassemble-print
    case OP_PRINT:
      return simpleInstruction("OP_PRINT", offset);
//< Global Variables disassemble-print
//> Jumping Forward and Back disassemble-jump
    case OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
//< Jumping Forward and Back disassemble-jump
//> Jumping Forward and Back disassemble-loop
    case OP_LOOP:
      return jumpInstruction("OP_LOOP", -1, chunk, offset);
//< Jumping Forward and Back disassemble-loop
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
      return simpleInstructionN("OP_CALL", instruction - OP_CALL_0,
                                offset);
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
          "OP_INVOKE_", instruction - OP_INVOKE_0, chunk, offset);
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
          "OP_SUPER_", instruction - OP_SUPER_0, chunk, offset);
//< Superclasses not-yet
//> Closures not-yet

    case OP_CLOSURE: {
      offset++;
      uint8_t constant = chunk->code[offset++];
      printf("%-16s %4d ", "OP_CLOSURE", constant);
      printValue(chunk->constants.values[constant]);
      printf("\n");

      ObjFunction* function = AS_FUNCTION(
          chunk->constants.values[constant]);
      for (int j = 0; j < function->upvalueCount; j++) {
        int isLocal = chunk->code[offset++];
        int index = chunk->code[offset++];
        printf("%04d   |                     %s %d\n",
               offset - 2, isLocal ? "local" : "upvalue", index);
      }
      
      return offset;
    }

    case OP_CLOSE_UPVALUE:
      return simpleInstruction("OP_CLOSE_UPVALUE", offset);
//< Closures not-yet
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
//> Classes and Instances not-yet
    case OP_CLASS:
      return constantInstruction("OP_CLASS", chunk, offset);
//< Classes and Instances not-yet
//> Superclasses not-yet
    case OP_INHERIT:
      return simpleInstruction("OP_INHERIT", offset);
//< Superclasses not-yet
//> Methods and Initializers not-yet
    case OP_METHOD:
      return constantInstruction("OP_METHOD", chunk, offset);
//< Methods and Initializers not-yet
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}
//< disassemble-instruction
