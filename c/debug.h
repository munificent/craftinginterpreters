//>= Chunks of Bytecode
#ifndef cvox_debug_h
#define cvox_debug_h

#include "chunk.h"
//>= Uhh
#include "vm.h"
//>= Chunks of Bytecode

int disassembleInstruction(Chunk* chunk, int i);

void disassembleChunk(Chunk* chunk, const char* name);

void printValue(Value value);

#endif
