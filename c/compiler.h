//>= Scanning on Demand
#ifndef cvox_compiler_h
#define cvox_compiler_h

//>= Uhh
#include "object.h"
//>= Scanning on Demand
#include "vm.h"

/*== Scanning on Demand
void compile(const char* source);
*/
/*>= Compiling Expressions <= Types of Values
bool compile(const char* source, Chunk* chunk);
*/
//>= Uhh
ObjFunction* compile(const char* source);
void grayCompilerRoots();
//>= Scanning on Demand

#endif
