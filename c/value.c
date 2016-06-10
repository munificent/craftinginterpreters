#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "value.h"

bool valuesEqual(Value a, Value b) {
  if (a.type != b.type) return false;
  
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL: return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:
      // Objects have reference equality.
      return AS_OBJ(a) == AS_OBJ(b);
  }
}

void initArray(ValueArray* array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void growArray(ValueArray* array) {
  if (array->capacity > array->count) return;
  
  int oldCapacity = array->capacity;
  array->capacity *= GROW_FACTOR;
  if (array->capacity < MIN_CAPACITY) {
    array->capacity = MIN_CAPACITY;
  }
  
  array->values = GROW_ARRAY(array->values, Value,
                             oldCapacity, array->capacity);
}

void freeArray(ValueArray* array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initArray(array);
}
