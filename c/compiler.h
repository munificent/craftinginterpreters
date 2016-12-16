//>= Scanning on Demand 1
#ifndef clox_compiler_h
#define clox_compiler_h

//>= Strings 1
#include "object.h"
//>= Scanning on Demand 1
#include "vm.h"

/*>= Scanning on Demand 1 < Compiling Expressions 1
void compile(const char* source);
*/
/*>= Compiling Expressions 1 < Calls and Functions 1
bool compile(const char* source, Chunk* chunk);
*/
//>= Calls and Functions 1
ObjFunction* compile(const char* source);
//>= Garbage Collection 1
void grayCompilerRoots();
//>= Scanning on Demand 1

#endif
