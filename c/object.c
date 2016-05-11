#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objType) (type*)allocateObj(sizeof(type), objType)

static Obj* allocateObj(size_t size, ObjType type) {
  Obj* obj = (Obj*)reallocate(NULL, size);
  obj->type = type;
  obj->isDark = false;
  
  obj->next = vm.objects;
  vm.objects = obj;

#ifdef DEBUG_TRACE_GC
  printf("%p allocate %ld for %d\n", obj, size, type);
#endif
  
  return obj;
}

ObjBool* newBool(bool value) {
  ObjBool* boolean = ALLOCATE_OBJ(ObjBool, OBJ_BOOL);
  boolean->value = value;
  return boolean;
}

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);

  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

ObjClass* newClass(ObjString* name, ObjClass* superclass) {
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name;
  klass->superclass = superclass;
  klass->constructor = NULL;
  initTable(&klass->methods);
  return klass;
}

ObjClosure* newClosure(ObjFunction* function) {
  // TODO: Flex array?
  // Allocate the upvalue array first so it doesn't cause the closure to get
  // collected.
  ObjUpvalue** upvalues = REALLOCATE(NULL, ObjUpvalue*, function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }
  
  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  return closure;
}

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  
  function->codeCount = 0;
  function->codeCapacity = 0;
  function->code = NULL;
  function->arity = 0;
  function->upvalueCount = 0;
  function->name = NULL;
  function->codeLines = NULL;
  initArray(&function->constants);
  return function;
}

ObjInstance* newInstance(ObjClass* klass) {
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  initTable(&instance->fields);
  return instance;
}

ObjNative* newNative(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

ObjNumber* newNumber(double value) {
  ObjNumber* number = ALLOCATE_OBJ(ObjNumber, OBJ_NUMBER);
  number->value = value;
  return number;
}

static ObjString* allocateString(const char* chars, int length) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  
  push((Value)string);
  tableSet(&vm.strings, string, NULL);
  pop();
  
  return string;
}

ObjString* takeString(const char* chars, int length) {
  ObjString* interned = tableFindString(&vm.strings, chars, length);
  if (interned != NULL) return interned;

  return allocateString(chars, length);
}

ObjString* copyString(const char* chars, int length) {
  ObjString* interned = tableFindString(&vm.strings, chars, length);
  if (interned != NULL) return interned;
  
  // Copy the characters to the heap so the object can own it.
  char* heapChars = REALLOCATE(NULL, char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  
  return allocateString(heapChars, length);
}

ObjUpvalue* newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NULL;
  upvalue->value = slot;
  upvalue->next = NULL;
  
  return upvalue;
}

bool valuesEqual(Value a, Value b) {
  // Identity.
  if (a == b) return true;
  
  if (IS_NULL(a) || IS_NULL(b)) return false;
  
  // No implicit conversions.
  if (a->type != b->type) return false;
  
  switch (a->type) {
    case OBJ_BOOL:
      // TODO: Canonicalize bools?
      return AS_BOOL(a) == AS_BOOL(b);
      
    case OBJ_NUMBER:
      return AS_NUMBER(a) == AS_NUMBER(b);
      
    case OBJ_STRING:
    case OBJ_BOUND_METHOD:
    case OBJ_CLASS:
    case OBJ_CLOSURE:
    case OBJ_FUNCTION:
    case OBJ_INSTANCE:
    case OBJ_NATIVE:
    case OBJ_UPVALUE:
      // These have reference equality.
      return false;
  }
}

void initArray(ValueArray* array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void growArray(ValueArray* array) {
  if (array->capacity > array->count) return;
  
  if (array->capacity == 0) {
    array->capacity = 4;
  } else {
    array->capacity *= 2;
  }
  
  array->values = REALLOCATE(array->values, Value, array->capacity);
}

void freeArray(ValueArray* array) {
  free(array->values);
}
