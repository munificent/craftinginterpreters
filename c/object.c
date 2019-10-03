//> Strings object-c
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
//> Hash Tables object-include-table
#include "table.h"
//< Hash Tables object-include-table
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
//> Garbage Collection init-is-dark
  object->isDark = false;
//< Garbage Collection init-is-dark
//> add-to-list
  
  object->next = vm.objects;
  vm.objects = object;
//< add-to-list
//> Garbage Collection debug-log-allocate

#ifdef DEBUG_LOG_GC
  printf("%p allocate %ld for %d\n", object, size, type);
#endif

//< Garbage Collection debug-log-allocate
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
ObjClass* newClass(ObjString* name) {
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name;
//> Methods and Initializers not-yet
  initTable(&klass->methods);
//< Methods and Initializers not-yet
  return klass;
}
//< Classes and Instances not-yet
//> Closures new-closure
ObjClosure* newClosure(ObjFunction* function) {
//> allocate-upvalue-array
  ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }

//< allocate-upvalue-array
  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
//> init-upvalue-fields
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
//< init-upvalue-fields
  return closure;
}
//< Closures new-closure
//> Calls and Functions new-function
ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

  function->arity = 0;
//> Closures init-upvalue-count
  function->upvalueCount = 0;
//< Closures init-upvalue-count
  function->name = NULL;
  initChunk(&function->chunk);
  return function;
}
//< Calls and Functions new-function
//> Classes and Instances not-yet

ObjInstance* newInstance(ObjClass* klass) {
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  initTable(&instance->fields);
  return instance;
}
//< Classes and Instances not-yet
//> Calls and Functions new-native
ObjNative* newNative(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}
//< Calls and Functions new-native

/* Strings allocate-string < Hash Tables allocate-string
static ObjString* allocateString(char* chars, int length) {
*/
//> allocate-string
//> Hash Tables allocate-string
static ObjString* allocateString(char* chars, int length,
                                 uint32_t hash) {
//< Hash Tables allocate-string
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
//> Hash Tables allocate-store-hash
  string->hash = hash;
//< Hash Tables allocate-store-hash

//> Garbage Collection push-string
  push(OBJ_VAL(string));
//< Garbage Collection push-string
//> Hash Tables allocate-store-string
  tableSet(&vm.strings, string, NIL_VAL);
//> Garbage Collection pop-string
  pop();
//< Garbage Collection pop-string

//< Hash Tables allocate-store-string
  return string;
}
//< allocate-string
//> Hash Tables hash-string
static uint32_t hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;

  for (int i = 0; i < length; i++) {
    hash ^= key[i];
    hash *= 16777619;
  }

  return hash;
}
//< Hash Tables hash-string
//> take-string
ObjString* takeString(char* chars, int length) {
/* Strings take-string < Hash Tables take-string-hash
  return allocateString(chars, length);
*/
//> Hash Tables take-string-hash
  uint32_t hash = hashString(chars, length);
//> take-string-intern
  ObjString* interned = tableFindString(&vm.strings, chars, length,
                                        hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

//< take-string-intern
  return allocateString(chars, length, hash);
//< Hash Tables take-string-hash
}
//< take-string
ObjString* copyString(const char* chars, int length) {
//> Hash Tables copy-string-hash
  uint32_t hash = hashString(chars, length);
//> copy-string-intern
  ObjString* interned = tableFindString(&vm.strings, chars, length,
                                        hash);
  if (interned != NULL) return interned;
//< copy-string-intern

//< Hash Tables copy-string-hash
  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';

/* Strings object-c < Hash Tables copy-string-allocate
  return allocateString(heapChars, length);
*/
//> Hash Tables copy-string-allocate
  return allocateString(heapChars, length, hash);
//< Hash Tables copy-string-allocate
}
//> Closures new-upvalue
ObjUpvalue* newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
//> init-closed
  upvalue->closed = NIL_VAL;
//< init-closed
  upvalue->location = slot;
//> init-next
  upvalue->next = NULL;
//< init-next
  return upvalue;
}
//< Closures new-upvalue
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
      printf("<fn %s>",
             AS_BOUND_METHOD(value)->method->function->name->chars);
      break;
//< Methods and Initializers not-yet
//> Closures print-closure
    case OBJ_CLOSURE:
//> print-script
      if (AS_CLOSURE(value)->function->name == NULL) {
        printf("<script>");
        break;
      }
//< print-script
      printf("<fn %s>", AS_CLOSURE(value)->function->name->chars);
      break;
//< Closures print-closure
//> Calls and Functions print-function
    case OBJ_FUNCTION:
//> print-script
      if (AS_FUNCTION(value)->name == NULL) {
        printf("<script>");
        break;
      }
//< print-script
      printf("<fn %s>", AS_FUNCTION(value)->name->chars);
      break;
//< Calls and Functions print-function
//> Classes and Instances not-yet
    case OBJ_INSTANCE:
      printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
      break;
//< Classes and Instances not-yet
//> Calls and Functions print-native
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
//< Calls and Functions print-native
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
//> Closures print-upvalue
    case OBJ_UPVALUE:
      printf("upvalue");
      break;
//< Closures print-upvalue
  }
}
//< print-object
