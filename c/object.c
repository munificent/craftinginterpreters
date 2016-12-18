//>> Strings 99
#include <string.h>

#include "memory.h"
#include "object.h"
//>> Hash Tables 99
#include "table.h"
//<< Hash Tables 99
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
//>> Garbage Collection 99
  object->isDark = false;
//<< Garbage Collection 99

  object->next = vm.objects;
  vm.objects = object;
//>> Garbage Collection 99

#ifdef DEBUG_TRACE_GC
  printf("%p allocate %ld for %d\n", object, size, type);
#endif

//<< Garbage Collection 99
  return object;
}
//>> Methods and Initializers 99

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);

  bound->receiver = receiver;
  bound->method = method;
  return bound;
}
//<< Methods and Initializers 99
//>> Classes and Instances 99

/*>= Classes and Instances 99 < Superclasses 99
ObjClass* newClass(ObjString* name) {
*/
//>> Superclasses 99
ObjClass* newClass(ObjString* name, ObjClass* superclass) {
//<< Superclasses 99
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name;
//>> Superclasses 99
  klass->superclass = superclass;
//<< Superclasses 99
//>> Methods and Initializers 99
  initTable(&klass->methods);
//<< Methods and Initializers 99
  return klass;
}
//<< Classes and Instances 99
//>> Closures 99

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
//<< Closures 99
//>> Calls and Functions 99

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

  function->arity = 0;
//>> Closures 99
  function->upvalueCount = 0;
//<< Closures 99
  function->name = NULL;
  initChunk(&function->chunk);
  return function;
}
//<< Calls and Functions 99
//>> Classes and Instances 99

ObjInstance* newInstance(ObjClass* klass) {
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  initTable(&instance->fields);
  return instance;
}
//<< Classes and Instances 99
//>> Calls and Functions 99

ObjNative* newNative(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}
//<< Calls and Functions 99

/*>= Strings 99 < Hash Tables 99
static ObjString* allocateString(char* chars, int length) {
*/
//>> Hash Tables 99
static ObjString* allocateString(char* chars, int length, uint32_t hash) {
//<< Hash Tables 99
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
//>> Hash Tables 99
  string->hash = hash;
//<< Hash Tables 99

//>> Garbage Collection 99
  push(OBJ_VAL(string));
//<< Garbage Collection 99
//>> Hash Tables 99
  tableSet(&vm.strings, string, NIL_VAL);
//>> Garbage Collection 99
  pop();
//<< Garbage Collection 99

//<< Hash Tables 99
  return string;
}
//>> Hash Tables 99

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
//<< Hash Tables 99

ObjString* takeString(char* chars, int length) {
/*>= Strings 99 < Hash Tables 99
  return allocateString(chars, length);
*/
//>> Hash Tables 99
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) return interned;

  return allocateString(chars, length, hash);
//<< Hash Tables 99
}

ObjString* copyString(const char* chars, int length) {
//>> Hash Tables 99
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) return interned;

//<< Hash Tables 99
  // Copy the characters to the heap so the object can own it.
  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';

/*>= Strings 99 < Hash Tables 99
  return allocateString(heapChars, length);
*/
//>> Hash Tables 99
  return allocateString(heapChars, length, hash);
//<< Hash Tables 99
}
//>> Closures 99

ObjUpvalue* newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->value = slot;
  upvalue->next = NULL;

  return upvalue;
}
//<< Closures 99
