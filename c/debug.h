#ifndef cvox_debug_h
#define cvox_debug_h

#include "object.h"
#include "vm.h"

void printValue(Value value);

int disassembleInstruction(Chunk* chunk, int i);

void disassembleChunk(Chunk* chunk, const char* name);

#endif
