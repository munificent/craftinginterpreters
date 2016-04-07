#ifndef cvox_common_h
#define cvox_common_h

#include <stdbool.h>
#include <stdint.h>

#define MAX_HEAP (10 * 1024 * 1024)

//#define DEBUG_PRINT_CODE
//#define DEBUG_TRACE_EXECUTION

#define DEBUG_STRESS_GC
//#define DEBUG_TRACE_GC

typedef struct sObj Obj;
typedef Obj* Value;
typedef struct sObjString ObjString;

typedef struct sVM VM;

#endif
