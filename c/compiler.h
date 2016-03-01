#ifndef cvox_compiler_h
#define cvox_compiler_h

#include "object.h"
#include "vm.h"

ObjFunction* compile(VM* vm, const char* source);

#endif
