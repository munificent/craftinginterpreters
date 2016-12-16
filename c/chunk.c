//>= Chunks of Bytecode 1
#include <stdlib.h>

#include "chunk.h"

#include "memory.h"
#include "value.h"
//>= Garbage Collection 1
#include "vm.h"
//>= Chunks of Bytecode 1

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  initArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  freeArray(&chunk->constants);
  initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code = GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);
    chunk->lines = GROW_ARRAY(chunk->lines, int, oldCapacity, chunk->capacity);
  }
  
  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count++] = line;
}

int addConstant(Chunk* chunk, Value value) {
  // See if we already have an equivalent constant.
  for (int i = 0; i < chunk->constants.count; i++) {
    if (valuesEqual(value, chunk->constants.values[i])) {
      return i;
    }
  }
  
  if (chunk->constants.count == UINT8_COUNT) return -1;
//>= Garbage Collection 1
  
  // Make sure the value doesn't get collected when resizing the array.
  push(value);
  
//>= Chunks of Bytecode 1
  growArray(&chunk->constants);
  chunk->constants.values[chunk->constants.count] = value;
//>= Garbage Collection 1
  pop();
//>= Chunks of Bytecode 1
  return chunk->constants.count++;
}