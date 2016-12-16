//>= Chunks of Bytecode 1
#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stdint.h>

//>= Optimization 1
#define NAN_TAGGING

//>= Compiling Expressions 1
//#define DEBUG_PRINT_CODE
//>= A Virtual Machine 1
//#define DEBUG_TRACE_EXECUTION
//>= Garbage Collection 1

#define DEBUG_STRESS_GC
//#define DEBUG_TRACE_GC
//>= Chunks of Bytecode 1

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
