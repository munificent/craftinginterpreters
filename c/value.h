//> Chunks of Bytecode value-h
#ifndef clox_value_h
#define clox_value_h

#include "common.h"

//> Strings forward-declare-obj
typedef struct sObj Obj;
//> forward-declare-obj-string
typedef struct sObjString ObjString;
//< forward-declare-obj-string

//< Strings forward-declare-obj
//> Optimization nan-tagging
#ifdef NAN_TAGGING
//> sign-bit

#define SIGN_BIT ((uint64_t)1 << 63)
//< sign-bit
//> qnan

#define QNAN ((uint64_t)0x7ffc000000000000)
//< qnan
//> tags

#define TAG_NIL   1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.
//< tags

typedef uint64_t Value;
//> is-number

//> is-bool
#define IS_BOOL(v)      (((v) & FALSE_VAL) == FALSE_VAL)
//< is-bool
//> is-nil
#define IS_NIL(v)       ((v) == NIL_VAL)
//< is-nil
#define IS_NUMBER(v)    (((v) & QNAN) != QNAN)
//< is-number
//> is-obj
#define IS_OBJ(v)       (((v) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
//< is-obj
//> as-number

//> as-bool
#define AS_BOOL(v)      ((v) == TRUE_VAL)
//< as-bool
#define AS_NUMBER(v)    valueToNum(v)
//< as-number
//> as-obj
#define AS_OBJ(v)       ((Obj*)(uintptr_t)((v) & ~(SIGN_BIT | QNAN)))
//< as-obj
//> number-val

//> bool-val
#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)
//< bool-val
//> false-true-vals
#define FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))
//< false-true-vals
//> nil-val
#define NIL_VAL         ((Value)(uint64_t)(QNAN | TAG_NIL))
//< nil-val
#define NUMBER_VAL(num) numToValue(num)
//< number-val
//> obj-val
#define OBJ_VAL(obj) \
    (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))
//< obj-val
//> double-union

typedef union {
  uint64_t bits;
  double num;
} DoubleUnion;
//< double-union
//> value-to-num

static inline double valueToNum(Value value) {
  DoubleUnion data;
  data.bits = value;
  return data.num;
}
//< value-to-num
//> num-to-value

static inline Value numToValue(double num) {
  DoubleUnion data;
  data.num = num;
  return data.bits;
}
//< num-to-value

#else

//< Optimization nan-tagging
//> Types of Values value-type
typedef enum {
  VAL_BOOL,
  VAL_NIL, // [user-types]
  VAL_NUMBER,
//> Strings val-obj
  VAL_OBJ
//< Strings val-obj
} ValueType;

//< Types of Values value-type
/* Chunks of Bytecode value-h < Types of Values value
typedef double Value;
*/
//> Types of Values value
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
//> Strings union-object
    Obj* obj;
//< Strings union-object
  } as; // [as]
} Value;
//< Types of Values value
//> Types of Values is-macros

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
//> Strings is-obj
#define IS_OBJ(value)     ((value).type == VAL_OBJ)
//< Strings is-obj
//< Types of Values is-macros
//> Types of Values as-macros

//> Strings as-obj
#define AS_OBJ(value)     ((value).as.obj)
//< Strings as-obj
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
//< Types of Values as-macros
//> Types of Values value-macros

#define BOOL_VAL(value)   ((Value){ VAL_BOOL, { .boolean = value } })
#define NIL_VAL           ((Value){ VAL_NIL, { .number = 0 } })
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })
//> Strings obj-val
#define OBJ_VAL(object)   ((Value){ VAL_OBJ, { .obj = (Obj*)object } })
//< Strings obj-val
//< Types of Values value-macros
//> Optimization end-if-nan-tagging

#endif
//< Optimization end-if-nan-tagging
//> value-array

typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;
//< value-array
//> array-fns-h

//> Types of Values values-equal-h
bool valuesEqual(Value a, Value b);
//< Types of Values values-equal-h
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
//< array-fns-h
//> print-value-h
void printValue(Value value);
//< print-value-h

#endif
