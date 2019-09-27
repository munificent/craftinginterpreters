//> Scanning on Demand compiler-h
#ifndef clox_compiler_h
#define clox_compiler_h

//> Strings compiler-include-object
#include "object.h"
//< Strings compiler-include-object
//> Compiling Expressions compile-h
#include "vm.h"

//< Compiling Expressions compile-h
/* Scanning on Demand compiler-h < Compiling Expressions compile-h
void compile(const char* source);
*/
/* Compiling Expressions compile-h < Calls and Functions compile-h
bool compile(const char* source, Chunk* chunk);
*/
//> Calls and Functions compile-h
ObjFunction* compile(const char* source);
//< Calls and Functions compile-h
//> Garbage Collection gray-roots-h
void grayCompilerRoots();
//< Garbage Collection gray-roots-h

#endif
