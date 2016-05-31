#ifndef cvox_memory_h
#define cvox_memory_h

#include "object.h"

#define REALLOCATE(previous, type, count) \
    (type*)reallocate(previous, sizeof(type) * count)

void* reallocate(void* previous, size_t size);

void grayObject(Obj* object);
void grayValue(Value value);
void collectGarbage();
void freeObjects();

#endif
