//> Chunks of Bytecode value-h
#ifndef clox_value_h
#define clox_value_h

#include "common.h"

//> Strings not-yet
typedef struct sObj Obj;
typedef struct sObjString ObjString;

//< Strings not-yet
//> Optimization not-yet

#ifdef NAN_TAGGING

// A mask that selects the sign bit.
#define SIGN_BIT ((uint64_t)1 << 63)

// The bits that must be set to indicate a quiet NaN.
#define QNAN ((uint64_t)0x7ffc000000000000)

// Tag values for the different singleton values.
#define TAG_NIL   1 // 01
#define TAG_FALSE 2 // 10
#define TAG_TRUE  3 // 11

typedef uint64_t Value;

#define IS_BOOL(value)    (((value) & (QNAN | TAG_FALSE)) == (QNAN | TAG_FALSE))
#define IS_NIL(value)     ((value) == NIL_VAL)
// If the NaN bits are set, it's not a number.
#define IS_NUMBER(value)  (((value) & QNAN) != QNAN)
#define IS_OBJ(value)     (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)    ((value) == TRUE_VAL)
#define AS_NUMBER(value)  valueToNum(value)
#define AS_OBJ(value)     ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define BOOL_VAL(boolean) ((boolean) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL         ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL          ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL           ((Value)(uint64_t)(QNAN | TAG_NIL))
#define NUMBER_VAL(num)   numToValue(num)
// The triple casting is necessary here to satisfy some compilers:
// 1. (uintptr_t) Convert the pointer to a number of the right size.
// 2. (uint64_t)  Pad it up to 64 bits in 32-bit builds.
// 3. Or in the bits to make a tagged Nan.
// 4. Cast to a typedef'd value.
#define OBJ_VAL(obj) (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

// A union to let us reinterpret a double as raw bits and back.
typedef union {
  uint64_t bits64;
  uint32_t bits32[2];
  double num;
} DoubleUnion;

static inline double valueToNum(Value value) {
  DoubleUnion data;
  data.bits64 = value;
  return data.num;
}

static inline Value numToValue(double num) {
  DoubleUnion data;
  data.num = num;
  return data.bits64;
}

#else

//< Optimization not-yet
/* Chunks of Bytecode value-h < Types of Values value
typedef double Value;
*/
//> Types of Values value-type
typedef enum {
  VAL_BOOL,
  VAL_NIL, // [user-types]
  VAL_NUMBER,
//> Strings not-yet
  VAL_OBJ
//< Strings not-yet
} ValueType;

//< Types of Values value-type
//> Types of Values value
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
//> Strings not-yet
    Obj* object;
//< Strings not-yet
  } as; // [as]
} Value;
//< Types of Values value
//> Types of Values is-macros

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
//> Strings not-yet
#define IS_OBJ(value)     ((value).type == VAL_OBJ)
//< Strings not-yet
//< Types of Values is-macros
//> Types of Values as-macros

//> Strings not-yet
#define AS_OBJ(value)     ((value).as.object)
//< Strings not-yet
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
//< Types of Values as-macros
//> Types of Values value-macros

#define BOOL_VAL(value)   ((Value){ VAL_BOOL, { .boolean = value } })
#define NIL_VAL           ((Value){ VAL_NIL, { .number = 0 } })
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })
//> Strings not-yet
#define OBJ_VAL(obj)      ((Value){ VAL_OBJ, { .object = (Obj*)obj } })
//< Strings not-yet
//< Types of Values value-macros
//> Optimization not-yet

#endif
//< Optimization not-yet
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
