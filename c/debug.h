//>> Chunks of Bytecode 99
#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

int disassembleInstruction(Chunk* chunk, int i);
void disassembleChunk(Chunk* chunk, const char* name);

#endif
