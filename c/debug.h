#ifndef cvox_debug_h
#define cvox_debug_h

#include "object.h"
#include "vm.h"

void printValue(Value value);

void printStack(VM* vm);

void printFunction(ObjFunction* function);

#endif
