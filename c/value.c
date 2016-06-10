#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "value.h"

#define ARRAY_MIN_CAPACITY 8
#define ARRAY_GROW_FACTOR 2

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
  if (array->capacity == 0) {
    array->capacity = ARRAY_MIN_CAPACITY;
  } else {
    array->capacity *= ARRAY_GROW_FACTOR;
  }
  
  array->values = GROW_ARRAY(array->values, Value,
                             oldCapacity, array->capacity);
}

void freeArray(ValueArray* array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initArray(array);
}

// TODO: Unify with strNative?
void printValue(Value value) {
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
      
    case VAL_NIL:
      printf("nil");
      break;
      
    case VAL_NUMBER:
      printf("%g", AS_NUMBER(value));
      break;
      
    case VAL_OBJ:
      // TODO: Nested switch is kind of lame.
      switch (OBJ_TYPE(value)) {
        case OBJ_CLASS:
          printf("%s", AS_CLASS(value)->name->chars);
          break;
          
        case OBJ_BOUND_METHOD:
        case OBJ_CLOSURE:
        case OBJ_FUNCTION:
          printf("<fn %p>", AS_FUNCTION(value));
          break;
          
        case OBJ_INSTANCE:
          printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
          break;
          
        case OBJ_NATIVE:
          printf("<native %p>", AS_NATIVE(value));
          break;
          
        case OBJ_STRING:
          printf("%s", AS_CSTRING(value));
          break;
          
        case OBJ_UPVALUE:
          printf("upvalue");
          break;
      }
      break;
  }
}
