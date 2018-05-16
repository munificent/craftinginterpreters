//> Scanning on Demand compiler-h
#ifndef clox_compiler_h
#define clox_compiler_h

//> Strings not-yet
#include "object.h"
//< Strings not-yet
//> Compiling Expressions not-yet
#include "vm.h"
//< Compiling Expressions not-yet

/* Scanning on Demand compiler-h < Compiling Expressions not-yet
void compile(const char* source);
*/
/* Compiling Expressions not-yet < Calls and Functions not-yet
bool compile(const char* source, Chunk* chunk);
*/
//> Calls and Functions not-yet
ObjFunction* compile(const char* source);
//< Calls and Functions not-yet
//> Garbage Collection not-yet
void grayCompilerRoots();
//< Garbage Collection not-yet

#endif
