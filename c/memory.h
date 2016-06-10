#ifndef cvox_memory_h
#define cvox_memory_h

#include "object.h"

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * count)
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_ARRAY(previous, type, oldCount, count) \
    (type*)reallocate(previous, sizeof(type) * oldCount, sizeof(type) * count)
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * oldCount, 0)

void* reallocate(void* previous, size_t oldSize, size_t newSize);

void grayObject(Obj* object);
void grayValue(Value value);
void collectGarbage();
void freeObjects();

#endif
