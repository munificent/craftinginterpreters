//> Chunks of Bytecode common-h
#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stdint.h>
//> A Virtual Machine define-debug-trace

//> Optimization not-yet
#define NAN_TAGGING
//< Optimization not-yet
//> Compiling Expressions not-yet
//#define DEBUG_PRINT_CODE
//< Compiling Expressions not-yet
#define DEBUG_TRACE_EXECUTION
//> omit
// In the book, we always have it defined, but for working on it locally,
// we don't want it to be.
#undef DEBUG_TRACE_EXECUTION
//< omit
//< A Virtual Machine define-debug-trace
//> Garbage Collection not-yet

#define DEBUG_STRESS_GC
//#define DEBUG_TRACE_GC
//< Garbage Collection not-yet
//> Local Variables not-yet

#define UINT8_COUNT (UINT8_MAX + 1)
//< Local Variables not-yet

#endif
