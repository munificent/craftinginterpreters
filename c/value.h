#ifndef cvox_value_h
#define cvox_value_h

#include "common.h"

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_OBJ(value)     ((value).as.object)
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

#define BOOL_VAL(value)   ((Value){ VAL_BOOL, { .boolean = value } })
#define NIL_VAL           ((Value){ VAL_NIL, { .object = NULL } })
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })
#define OBJ_VAL(obj)      ((Value){ VAL_OBJ, { .object = (Obj*)obj } })

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ
} ValueType;

typedef struct sObj Obj;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj* object;
  } as;
} Value;

typedef struct {
  Value* values;
  int capacity;
  int count;
} ValueArray;

bool valuesEqual(Value a, Value b);

void initArray(ValueArray* array);
void growArray(ValueArray* array);
void freeArray(ValueArray* array);

#endif
