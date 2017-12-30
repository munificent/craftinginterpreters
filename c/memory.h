//> Chunks of Bytecode memory-h
#ifndef clox_memory_h
#define clox_memory_h

//> Strings not-yet
#include "object.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) \
    reallocate(pointer, sizeof(type), 0)
//< Strings not-yet
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(previous, type, oldCount, count) \
    (type*)reallocate(previous, sizeof(type) * (oldCount), \
        sizeof(type) * (count))
//> free-array

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)
//< free-array

void* reallocate(void* previous, size_t oldSize, size_t newSize);
//> Garbage Collection not-yet

void grayObject(Obj* object);
void grayValue(Value value);
void collectGarbage();
//< Garbage Collection not-yet
//> Strings not-yet
void freeObjects();
//< Strings not-yet

#endif
