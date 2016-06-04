#include <stdlib.h>

#include "chunk.h"

#include "memory.h"
#include "value.h"
#include "vm.h"

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  initArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
  free(chunk->code);
  freeArray(&chunk->constants);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
  // TODO: growArray() has almost the exact same code. Reuse somehow?
  if (chunk->capacity < chunk->count + 1) {
    if (chunk->capacity == 0) {
      chunk->capacity = 4;
    } else {
      chunk->capacity *= 2;
    }
    
    chunk->code = REALLOCATE(chunk->code, uint8_t, chunk->capacity);
    chunk->lines = REALLOCATE(chunk->lines, int, chunk->capacity);
  }
  
  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count++] = line;
}

int addConstant(Chunk* chunk, Value value) {
  // See if we already have an equivalent constant.
  for (int i = 0; i < chunk->constants.count; i++) {
    if (valuesEqual(value, chunk->constants.values[i])) {
      return (uint8_t)i;
    }
  }
  
  if (chunk->constants.count == UINT8_COUNT) return -1;
  
  // Make sure the value doesn't get collected when resizing the array.
  push(value);
  growArray(&chunk->constants);
  
  chunk->constants.values[chunk->constants.count] = pop();
  return chunk->constants.count++;
}