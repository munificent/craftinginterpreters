//> Chunks of Bytecode not-yet
#include <stdio.h>
#include <stdlib.h>
/* Strings not-yet < Hash Tables not-yet
#include <string.h>
*/

#include "memory.h"
#include "value.h"

//> Strings not-yet
static void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
//> Classes and Instances not-yet
    case OBJ_CLASS:
      printf("%s", AS_CLASS(value)->name->chars);
      break;
//< Classes and Instances not-yet
//> Methods and Initializers not-yet
    case OBJ_BOUND_METHOD:
//< Methods and Initializers not-yet
//> Closures not-yet
    case OBJ_CLOSURE:
//< Closures not-yet
//> Calls and Functions not-yet
    case OBJ_FUNCTION:
      printf("<fn %p>", AS_FUNCTION(value));
      break;
//< Calls and Functions not-yet
//> Classes and Instances not-yet
    case OBJ_INSTANCE:
      printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
      break;
//< Classes and Instances not-yet
//> Calls and Functions not-yet
    case OBJ_NATIVE:
      printf("<native %p>", AS_NATIVE(value));
      break;
//< Calls and Functions not-yet
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
//> Closures not-yet
    case OBJ_UPVALUE:
      printf("upvalue");
      break;
//< Closures not-yet
  }
}
//< Strings not-yet

void printValue(Value value) {
//> Optimization not-yet
#ifdef NAN_TAGGING
  if (IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_NUMBER(value)) {
    printf("%g", AS_NUMBER(value));
  } else if (IS_OBJ(value)) {
    printObject(value);
  }
#else
//< Optimization not-yet
/* Chunks of Bytecode not-yet < Types of Values not-yet
  printf("%g", value);
*/
//> Types of Values not-yet
  switch (value.type) {
    case VAL_BOOL:   printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL:    printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
//> Strings not-yet
    case VAL_OBJ:    printObject(value); break;
//< Strings not-yet
  }
//< Types of Values not-yet
//> Optimization not-yet
#endif
//< Optimization not-yet
}

bool valuesEqual(Value a, Value b) {
//> Optimization not-yet
#ifdef NAN_TAGGING
  return a == b;
#else
//< Optimization not-yet
/* Chunks of Bytecode not-yet < Types of Values not-yet
  return a == b;
*/
//> Types of Values not-yet
  if (a.type != b.type) return false;

  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
//> Strings not-yet
    case VAL_OBJ:
//< Strings not-yet
/* Strings not-yet < Hash Tables not-yet
    {
      ObjString* aString = AS_STRING(a);
      ObjString* bString = AS_STRING(b);
      return aString->length == bString->length &&
          memcmp(aString->chars, bString->chars, aString->length) == 0;
    }
 */
//> Hash Tables not-yet
      // Objects have reference equality.
      return AS_OBJ(a) == AS_OBJ(b);
//< Hash Tables not-yet
  }
//> Optimization not-yet
#endif
//< Optimization not-yet
//< Types of Values not-yet
}

void initArray(ValueArray* array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void growArray(ValueArray* array) {
  if (array->capacity > array->count) return;

  int oldCapacity = array->capacity;
  array->capacity = GROW_CAPACITY(oldCapacity);
  array->values = GROW_ARRAY(array->values, Value,
                             oldCapacity, array->capacity);
}

void freeArray(ValueArray* array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initArray(array);
}
