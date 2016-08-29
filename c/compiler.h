//>= Scanning on Demand
#ifndef clox_compiler_h
#define clox_compiler_h

//>= Strings
#include "object.h"
//>= Scanning on Demand
#include "vm.h"

/*== Scanning on Demand
void compile(const char* source);
*/
/*>= Compiling Expressions < User-Defined Functions
bool compile(const char* source, Chunk* chunk);
*/
//>= User-Defined Functions
ObjFunction* compile(const char* source);
//>= Garbage Collection
void grayCompilerRoots();
//>= Scanning on Demand

#endif
