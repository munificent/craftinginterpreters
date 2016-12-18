//>= Scanning on Demand 99
#ifndef clox_compiler_h
#define clox_compiler_h

//>= Strings 99
#include "object.h"
//>= Scanning on Demand 99
#include "vm.h"

/*>= Scanning on Demand 99 < Compiling Expressions 99
void compile(const char* source);
*/
/*>= Compiling Expressions 99 < Calls and Functions 99
bool compile(const char* source, Chunk* chunk);
*/
//>= Calls and Functions 99
ObjFunction* compile(const char* source);
//>= Garbage Collection 99
void grayCompilerRoots();
//>= Scanning on Demand 99

#endif
