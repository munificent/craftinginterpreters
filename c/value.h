//>= Chunks of Bytecode
#ifndef cvox_value_h
#define cvox_value_h

#include "common.h"

/*>= Chunks of Bytecode <= Compiling Expressions
typedef double Value;
*/
//>= Types of Values
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
//>= Uhh
#define IS_OBJ(value)     ((value).type == VAL_OBJ)
//>= Types of Values

//>= Uhh
#define AS_OBJ(value)     ((value).as.object)
//>= Types of Values
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

#define BOOL_VAL(value)   ((Value){ VAL_BOOL, { .boolean = value } })
#define NIL_VAL           ((Value){ VAL_NIL, { .number = 0 } })
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })
//>= Uhh
#define OBJ_VAL(obj)      ((Value){ VAL_OBJ, { .object = (Obj*)obj } })
//>= Types of Values

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
//>= Uhh
  VAL_OBJ
//>= Types of Values
} ValueType;
//>= Uhh

typedef struct sObj Obj;
//>= Types of Values

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
//>= Uhh
    Obj* object;
//>= Types of Values
  } as;
} Value;
//>= Chunks of Bytecode

typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);

void initArray(ValueArray* array);
void growArray(ValueArray* array);
void freeArray(ValueArray* array);

#endif
