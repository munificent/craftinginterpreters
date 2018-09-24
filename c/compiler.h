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
/* Compiling Expressions compile-h < Calls and Functions not-yet
bool compile(const char* source, Chunk* chunk);
*/
//> Calls and Functions not-yet
ObjFunction* compile(const char* source);
//< Calls and Functions not-yet
//> Garbage Collection not-yet
void grayCompilerRoots();
//< Garbage Collection not-yet

#endif
