//>= Scanning on Demand
#ifndef cvox_compiler_h
#define cvox_compiler_h

//>= Strings
#include "object.h"
//>= Scanning on Demand
#include "vm.h"

/*== Scanning on Demand
void compile(const char* source);
*/
/*>= Compiling Expressions <= Hash Tables
bool compile(const char* source, Chunk* chunk);
*/
//>= Uhh
ObjFunction* compile(const char* source);
void grayCompilerRoots();
//>= Scanning on Demand

#endif
