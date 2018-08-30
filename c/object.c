//> Strings object-c
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
//> Hash Tables not-yet
#include "table.h"
//< Hash Tables not-yet
#include "value.h"
#include "vm.h"
//> allocate-obj

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)
//< allocate-obj
//> allocate-object

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
//> Garbage Collection not-yet
  object->isDark = false;
//< Garbage Collection not-yet
//> add-to-list
  
  object->next = vm.objects;
  vm.objects = object;
//< add-to-list
//> Garbage Collection not-yet

#ifdef DEBUG_TRACE_GC
  printf("%p allocate %ld for %d\n", object, size, type);
#endif

//< Garbage Collection not-yet
  return object;
}
//< allocate-object
//> Methods and Initializers not-yet

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod,
                                       OBJ_BOUND_METHOD);

  bound->receiver = receiver;
  bound->method = method;
  return bound;
}
//< Methods and Initializers not-yet
//> Classes and Instances not-yet

/* Classes and Instances not-yet < Superclasses not-yet
ObjClass* newClass(ObjString* name) {
*/
//> Superclasses not-yet
ObjClass* newClass(ObjString* name, ObjClass* superclass) {
//< Superclasses not-yet
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name;
//> Superclasses not-yet
  klass->superclass = superclass;
//< Superclasses not-yet
//> Methods and Initializers not-yet
  initTable(&klass->methods);
//< Methods and Initializers not-yet
  return klass;
}
//< Classes and Instances not-yet
//> Closures not-yet

ObjClosure* newClosure(ObjFunction* function) {
  // Allocate the upvalue array first so it doesn't cause the closure
  // to get collected.
  ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }

  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}
//< Closures not-yet
//> Calls and Functions not-yet

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

  function->arity = 0;
//> Closures not-yet
  function->upvalueCount = 0;
//< Closures not-yet
  function->name = NULL;
  initChunk(&function->chunk);
  return function;
}
//< Calls and Functions not-yet
//> Classes and Instances not-yet

ObjInstance* newInstance(ObjClass* klass) {
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  initTable(&instance->fields);
  return instance;
}
//< Classes and Instances not-yet
//> Calls and Functions not-yet

ObjNative* newNative(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}
//< Calls and Functions not-yet

/* Strings allocate-string < Hash Tables not-yet
static ObjString* allocateString(char* chars, int length) {
*/
//> allocate-string
//> Hash Tables not-yet
static ObjString* allocateString(char* chars, int length,
                                 uint32_t hash) {
//< Hash Tables not-yet
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
//> Hash Tables not-yet
  string->hash = hash;
//< Hash Tables not-yet

//> Garbage Collection not-yet
  push(OBJ_VAL(string));
//< Garbage Collection not-yet
//> Hash Tables not-yet
  tableSet(&vm.strings, string, NIL_VAL);
//> Garbage Collection not-yet
  pop();
//< Garbage Collection not-yet

//< Hash Tables not-yet
  return string;
}
//< allocate-string
//> Hash Tables not-yet
static uint32_t hashString(const char* key, int length) {
  // FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
  uint32_t hash = 2166136261u;

  // This is O(n) on the length of the string, but we only call this
  // when a new string is created. Since the creation is also O(n) (to
  // copy/initialize all the bytes), we allow this here.
  for (int i = 0; i < length; i++) {
    hash ^= key[i];
    hash *= 16777619;
  }

  return hash;
}
//< Hash Tables not-yet
//> take-string
ObjString* takeString(char* chars, int length) {
/* Strings take-string < Hash Tables not-yet
  return allocateString(chars, length);
*/
//> Hash Tables not-yet
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&vm.strings, chars, length,
                                        hash);
  if (interned != NULL) return interned;

  return allocateString(chars, length, hash);
//< Hash Tables not-yet
}
//< take-string
ObjString* copyString(const char* chars, int length) {
//> Hash Tables not-yet
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&vm.strings, chars, length,
                                        hash);
  if (interned != NULL) return interned;

//< Hash Tables not-yet
  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';

/* Strings object-c < Hash Tables not-yet
  return allocateString(heapChars, length);
*/
//> Hash Tables not-yet
  return allocateString(heapChars, length, hash);
//< Hash Tables not-yet
}
//> Closures not-yet

ObjUpvalue* newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->value = slot;
  upvalue->next = NULL;

  return upvalue;
}
//< Closures not-yet
//> print-object
void printObject(Value value) {
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
      printf("<fn %s>", AS_CLOSURE(value)->function->name->chars);
      break;
//< Closures not-yet
//> Calls and Functions not-yet
    case OBJ_FUNCTION:
      printf("<fn %s>", AS_FUNCTION(value)->name->chars);
      break;
//< Calls and Functions not-yet
//> Classes and Instances not-yet
    case OBJ_INSTANCE:
      printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
      break;
//< Classes and Instances not-yet
//> Calls and Functions not-yet
    case OBJ_NATIVE:
      printf("<native fn>");
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
//< print-object
