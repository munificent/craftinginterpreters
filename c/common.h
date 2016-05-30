#ifndef cvox_common_h
#define cvox_common_h

#include <stdbool.h>
#include <stdint.h>

#define MAX_HEAP (10 * 1024 * 1024)

//#define DEBUG_PRINT_CODE
//#define DEBUG_TRACE_EXECUTION

#define DEBUG_STRESS_GC
//#define DEBUG_TRACE_GC

// TODO: Other unboxed types.
typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_OBJ
} ValueType;

typedef struct sObj Obj;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    Obj* obj;
  } as;
} Value;

typedef struct sObjString ObjString;

typedef struct sVM VM;

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
