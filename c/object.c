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
  initTable(&klass->methods);
  return klass;
}

ObjClosure* newClosure(ObjFunction* function) {
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

static ObjString* allocateString(const char* chars, int length, uint32_t hash) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  
  push(OBJ_VAL(string));
  tableSet(&vm.strings, string, NIL_VAL);
  pop();
  
  return string;
}

static uint32_t hashString(const char* key, int length) {
  // FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
  uint32_t hash = 2166136261u;
  
  // This is O(n) on the length of the string, but we only call this when a new
  // string is created. Since the creation is also O(n) (to copy/initialize all
  // the bytes), we allow this here.
  for (int i = 0; i < length; i++) {
    hash ^= key[i];
    hash *= 16777619;
  }
  
  return hash;
}

ObjString* takeString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) return interned;

  return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) return interned;
  
  // Copy the characters to the heap so the object can own it.
  char* heapChars = REALLOCATE(NULL, char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  
  return allocateString(heapChars, length, hash);
}

ObjUpvalue* newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->value = slot;
  upvalue->next = NULL;
  
  return upvalue;
}

bool valuesEqual(Value a, Value b) {
  if (a.type != b.type) return false;
  
  // TODO: Switch on value types.
  if (IS_NIL(a)) return true;
  if (IS_BOOL(a)) return AS_BOOL(a) == AS_BOOL(b);
  if (IS_NUMBER(a)) return AS_NUMBER(a) == AS_NUMBER(b);
  
  // Objects have reference equality.
  return AS_OBJ(a) == AS_OBJ(b);
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
