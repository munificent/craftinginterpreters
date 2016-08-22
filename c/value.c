//>= Chunks of Bytecode
#include <stdio.h>
#include <stdlib.h>
/*== Strings
#include <string.h>
*/
//>= Chunks of Bytecode

#include "memory.h"
//>= Chunks of Bytecode
#include "value.h"

void printValue(Value value) {
/*>= Chunks of Bytecode <= Compiling Expressions
  printf("%g", value);
*/
//>= Types of Values
  switch (value.type) {
    case VAL_BOOL:   printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL:    printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
//>= Strings
    case VAL_OBJ:
      switch (OBJ_TYPE(value)) {
//>= Classes and Instances
        case OBJ_CLASS:
          printf("%s", AS_CLASS(value)->name->chars);
          break;
//>= Methods and Initializers
        case OBJ_BOUND_METHOD:
//>= Closures
        case OBJ_CLOSURE:
//>= Functions
        case OBJ_FUNCTION:
          printf("<fn %p>", AS_FUNCTION(value));
          break;
//>= Classes and Instances
        case OBJ_INSTANCE:
          printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
          break;
//>= Native Functions
        case OBJ_NATIVE:
          printf("<native %p>", AS_NATIVE(value));
          break;
//>= Strings
        case OBJ_STRING:
          printf("%s", AS_CSTRING(value));
          break;
//>= Closures
        case OBJ_UPVALUE:
          printf("upvalue");
          break;
//>= Strings
      }
      break;
//>= Types of Values
  }
//>= Chunks of Bytecode
}

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
/*== Strings
    {
      ObjString* aString = AS_STRING(a);
      ObjString* bString = AS_STRING(b);
      return aString->length == bString->length &&
          memcmp(aString->chars, bString->chars, aString->length) == 0;
    }
 */
//>= Hash Tables
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
