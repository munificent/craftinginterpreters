//>= Chunks of Bytecode 1
#include <stdio.h>
#include <stdlib.h>
/*>= Strings 1 < Hash Tables 1
#include <string.h>
*/
//>= Chunks of Bytecode 1

#include "memory.h"
//>= Chunks of Bytecode 1
#include "value.h"

//>= Strings 1
static void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
//>= Classes and Instances 1
    case OBJ_CLASS:
      printf("%s", AS_CLASS(value)->name->chars);
      break;
//>= Methods and Initializers 1
    case OBJ_BOUND_METHOD:
//>= Closures 1
    case OBJ_CLOSURE:
//>= Calls and Functions 1
    case OBJ_FUNCTION:
      printf("<fn %p>", AS_FUNCTION(value));
      break;
//>= Classes and Instances 1
    case OBJ_INSTANCE:
      printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
      break;
//>= Calls and Functions 1
    case OBJ_NATIVE:
      printf("<native %p>", AS_NATIVE(value));
      break;
//>= Strings 1
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
//>= Closures 1
    case OBJ_UPVALUE:
      printf("upvalue");
      break;
//>= Strings 1
  }
}
//>= Chunks of Bytecode 1

void printValue(Value value) {
//>= Optimization 1
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
/*>= Chunks of Bytecode 1 < Types of Values 1
  printf("%g", value);
*/
//>= Types of Values 1
  switch (value.type) {
    case VAL_BOOL:   printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL:    printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
//>= Strings 1
    case VAL_OBJ:    printObject(value); break;
//>= Types of Values 1
  }
//>= Optimization 1
#endif
//>= Chunks of Bytecode 1
}

bool valuesEqual(Value a, Value b) {
//>= Optimization 1
#ifdef NAN_TAGGING
  return a == b;
#else
/*>= Chunks of Bytecode 1 < Types of Values 1
  return a == b;
*/
//>= Types of Values 1
  if (a.type != b.type) return false;
  
  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
//>= Strings 1
    case VAL_OBJ:
/*>= Strings 1 < Hash Tables 1
    {
      ObjString* aString = AS_STRING(a);
      ObjString* bString = AS_STRING(b);
      return aString->length == bString->length &&
          memcmp(aString->chars, bString->chars, aString->length) == 0;
    }
 */
//>= Hash Tables 1
      // Objects have reference equality.
      return AS_OBJ(a) == AS_OBJ(b);
//>= Types of Values 1
  }
//>= Optimization 1
#endif
//>= Chunks of Bytecode 1
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
