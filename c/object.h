//> Strings object-h
#ifndef clox_object_h
#define clox_object_h

#include "common.h"
//> Calls and Functions not-yet
#include "chunk.h"
//< Calls and Functions not-yet
//> Classes and Instances not-yet
#include "table.h"
//< Classes and Instances not-yet
#include "value.h"
//> obj-type-macro

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)
//< obj-type-macro
//> is-string

//> Methods and Initializers not-yet
#define IS_BOUND_METHOD(value)  isObjType(value, OBJ_BOUND_METHOD)
//< Methods and Initializers not-yet
//> Classes and Instances not-yet
#define IS_CLASS(value)         isObjType(value, OBJ_CLASS)
//< Classes and Instances not-yet
//> Closures not-yet
#define IS_CLOSURE(value)       isObjType(value, OBJ_CLOSURE)
//< Closures not-yet
//> Calls and Functions not-yet
#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
//< Calls and Functions not-yet
//> Classes and Instances not-yet
#define IS_INSTANCE(value)      isObjType(value, OBJ_INSTANCE)
//< Classes and Instances not-yet
//> Calls and Functions not-yet
#define IS_NATIVE(value)        isObjType(value, OBJ_NATIVE)
//< Calls and Functions not-yet
#define IS_STRING(value)        isObjType(value, OBJ_STRING)
//< is-string
//> as-string

//> Methods and Initializers not-yet
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)AS_OBJ(value))
//< Methods and Initializers not-yet
//> Classes and Instances not-yet
#define AS_CLASS(value)         ((ObjClass*)AS_OBJ(value))
//< Classes and Instances not-yet
//> Closures not-yet
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
//< Closures not-yet
//> Calls and Functions not-yet
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
//< Calls and Functions not-yet
//> Classes and Instances not-yet
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
//< Classes and Instances not-yet
//> Calls and Functions not-yet
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
//< Calls and Functions not-yet
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)
//< as-string
//> obj-type

typedef enum {
//> Methods and Initializers not-yet
  OBJ_BOUND_METHOD,
//< Methods and Initializers not-yet
//> Classes and Instances not-yet
  OBJ_CLASS,
//< Classes and Instances not-yet
//> Closures not-yet
  OBJ_CLOSURE,
//< Closures not-yet
//> Calls and Functions not-yet
  OBJ_FUNCTION,
//< Calls and Functions not-yet
//> Classes and Instances not-yet
  OBJ_INSTANCE,
//< Classes and Instances not-yet
//> Calls and Functions not-yet
  OBJ_NATIVE,
//< Calls and Functions not-yet
  OBJ_STRING,
//> Closures not-yet
  OBJ_UPVALUE
//< Closures not-yet
} ObjType;
//< obj-type

struct sObj {
  ObjType type;
//> Garbage Collection not-yet
  bool isDark;
//< Garbage Collection not-yet
//> next-field
  struct sObj* next;
//< next-field
};
//> Calls and Functions not-yet

typedef struct {
  Obj obj;
  int arity;
//> Closures not-yet
  int upvalueCount;
//< Closures not-yet
  Chunk chunk;
  ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;
//< Calls and Functions not-yet
//> obj-string

struct sObjString {
  Obj obj;
  int length;
  char* chars;
//> Hash Tables not-yet
  uint32_t hash;
//< Hash Tables not-yet
};
//< obj-string
//> Closures not-yet

typedef struct sUpvalue {
  Obj obj;

  // Pointer to the variable this upvalue is referencing.
  Value* value;

  // If the upvalue is closed (i.e. the local variable it was pointing
  // to has been popped off the stack) then the closed-over value is
  // hoisted out of the stack into here. [value] is then be changed to
  // point to this.
  Value closed;

  // Open upvalues are stored in a linked list. This points to the next
  // one in that list.
  struct sUpvalue* next;
} ObjUpvalue;

typedef struct {
  Obj obj;
  ObjFunction* function;
  ObjUpvalue** upvalues;
  int upvalueCount;
} ObjClosure;
//< Closures not-yet
//> Classes and Instances not-yet

typedef struct sObjClass {
  Obj obj;
  ObjString* name;
//> Superclasses not-yet
  struct sObjClass* superclass;
//< Superclasses not-yet
//> Methods and Initializers not-yet
  Table methods;
//< Methods and Initializers not-yet
} ObjClass;

typedef struct {
  Obj obj;
  ObjClass* klass;
  Table fields;
} ObjInstance;
//< Classes and Instances not-yet

//> Methods and Initializers not-yet
typedef struct {
  Obj obj;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
//< Methods and Initializers not-yet
/* Classes and Instances not-yet < Superclasses not-yet
ObjClass* newClass(ObjString* name);
*/
//> Superclasses not-yet
ObjClass* newClass(ObjString* name, ObjClass* superclass);
//< Superclasses not-yet
//> Closures not-yet
ObjClosure* newClosure(ObjFunction* function);
//< Closures not-yet
//> Calls and Functions not-yet
ObjFunction* newFunction();
//< Calls and Functions not-yet
//> Classes and Instances not-yet
ObjInstance* newInstance(ObjClass* klass);
//< Classes and Instances not-yet
//> Calls and Functions not-yet
ObjNative* newNative(NativeFn function);
//< Calls and Functions not-yet
//> take-string-h
ObjString* takeString(char* chars, int length);
//< take-string-h
//> copy-string-h
ObjString* copyString(const char* chars, int length);

//< copy-string-h
//> Closures not-yet
ObjUpvalue* newUpvalue(Value* slot);
//< Closures not-yet
//> print-object-h
void printObject(Value value);
//< print-object-h
//> is-obj-type
static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

//< is-obj-type
#endif
