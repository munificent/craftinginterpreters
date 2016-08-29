//>= Strings
#include <string.h>

#include "memory.h"
#include "object.h"
//>= Hash Tables
#include "table.h"
//>= Strings
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
//>= Garbage Collection
  object->isDark = false;
//>= Strings
  
  object->next = vm.objects;
  vm.objects = object;
//>= Garbage Collection
  
#ifdef DEBUG_TRACE_GC
  printf("%p allocate %ld for %d\n", object, size, type);
#endif
  
//>= Strings
  return object;
}
//>= Methods and Initializers

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);

  bound->receiver = receiver;
  bound->method = method;
  return bound;
}
//>= Classes and Instances

/*>= Classes and Instances <= Methods and Initializers
ObjClass* newClass(ObjString* name) {
*/
//>= Inheritance
ObjClass* newClass(ObjString* name, ObjClass* superclass) {
//>= Classes and Instances
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name;
//>= Inheritance
  klass->superclass = superclass;
//>= Methods and Initializers
  initTable(&klass->methods);
//>= Classes and Instances
  return klass;
}
//>= Closures

ObjClosure* newClosure(ObjFunction* function) {
  // Allocate the upvalue array first so it doesn't cause the closure to get
  // collected.
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
//>= User-Defined Functions

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  
  function->arity = 0;
//>= Closures
  function->upvalueCount = 0;
//>= User-Defined Functions
  function->name = NULL;
  initChunk(&function->chunk);
  return function;
}
//>= Classes and Instances

ObjInstance* newInstance(ObjClass* klass) {
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  initTable(&instance->fields);
  return instance;
}
//>= Function Calls

ObjNative* newNative(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}
/*== Strings

static ObjString* allocateString(char* chars, int length) {
*/
//>= Hash Tables

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
//>= Strings
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
//>= Hash Tables
  string->hash = hash;

//>= Garbage Collection
  push(OBJ_VAL(string));
//>= Hash Tables
  tableSet(&vm.strings, string, NIL_VAL);
//>= Garbage Collection
  pop();
//>= Strings

  return string;
}
//>= Hash Tables

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
//>= Strings

ObjString* takeString(char* chars, int length) {
/*== Strings
  return allocateString(chars, length);
*/
//>= Hash Tables
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) return interned;

  return allocateString(chars, length, hash);
//>= Strings
}

ObjString* copyString(const char* chars, int length) {
//>= Hash Tables
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) return interned;
  
//>= Strings
  // Copy the characters to the heap so the object can own it.
  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';

/*== Strings
  return allocateString(heapChars, length);
*/
//>= Hash Tables
  return allocateString(heapChars, length, hash);
//>= Strings
}
//>= Closures

ObjUpvalue* newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->value = slot;
  upvalue->next = NULL;
  
  return upvalue;
}
