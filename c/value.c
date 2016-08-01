//>= Chunks of Bytecode
#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
//>= Uhh
#include "object.h"
//>= Chunks of Bytecode
#include "value.h"

bool valuesEqual(Value a, Value b) {
/*>= Chunks of Bytecode <= Compiling Expressions
  return a == b;
*/
//>= Types of Values
  if (a.type != b.type) return false;
  
  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
//>= Strings
    case VAL_OBJ:
      // TODO: Need to compare string chars until we intern them.
      // Objects have reference equality.
      return AS_OBJ(a) == AS_OBJ(b);
//>= Types of Values
  }
//>= Chunks of Bytecode
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
