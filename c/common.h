//>= Chunks of Bytecode
#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stdint.h>

//>= Optimization
#define NAN_TAGGING

//>= Compiling Expressions
//#define DEBUG_PRINT_CODE
//>= A Virtual Machine
//#define DEBUG_TRACE_EXECUTION
//>= Garbage Collection

#define DEBUG_STRESS_GC
//#define DEBUG_TRACE_GC
//>= Chunks of Bytecode

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
