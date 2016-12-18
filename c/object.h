//> Strings 99
#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
//> Classes and Instances 99
#include "table.h"
//< Classes and Instances 99

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)

//> Methods and Initializers 99
#define IS_BOUND_METHOD(value)  isObjType(value, OBJ_BOUND_METHOD)
//< Methods and Initializers 99
//> Classes and Instances 99
#define IS_CLASS(value)         isObjType(value, OBJ_CLASS)
//< Classes and Instances 99
//> Closures 99
#define IS_CLOSURE(value)       isObjType(value, OBJ_CLOSURE)
//< Closures 99
//> Calls and Functions 99
#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
//< Calls and Functions 99
//> Classes and Instances 99
#define IS_INSTANCE(value)      isObjType(value, OBJ_INSTANCE)
//< Classes and Instances 99
//> Calls and Functions 99
#define IS_NATIVE(value)        isObjType(value, OBJ_NATIVE)
//< Calls and Functions 99
#define IS_STRING(value)        isObjType(value, OBJ_STRING)

//> Methods and Initializers 99
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)AS_OBJ(value))
//< Methods and Initializers 99
//> Classes and Instances 99
#define AS_CLASS(value)         ((ObjClass*)AS_OBJ(value))
//< Classes and Instances 99
//> Closures 99
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
//< Closures 99
//> Calls and Functions 99
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
//< Calls and Functions 99
//> Classes and Instances 99
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
//< Classes and Instances 99
//> Calls and Functions 99
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
//< Calls and Functions 99
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
//> Methods and Initializers 99
  OBJ_BOUND_METHOD,
//< Methods and Initializers 99
//> Classes and Instances 99
  OBJ_CLASS,
//< Classes and Instances 99
//> Closures 99
  OBJ_CLOSURE,
//< Closures 99
//> Calls and Functions 99
  OBJ_FUNCTION,
//< Calls and Functions 99
//> Classes and Instances 99
  OBJ_INSTANCE,
//< Classes and Instances 99
//> Calls and Functions 99
  OBJ_NATIVE,
//< Calls and Functions 99
  OBJ_STRING,
//> Closures 99
  OBJ_UPVALUE
//< Closures 99
} ObjType;

struct sObj {
  ObjType type;
//> Garbage Collection 99
  bool isDark;
//< Garbage Collection 99
  struct sObj* next;
};
//> Calls and Functions 99

typedef struct {
  Obj object;
  int arity;
//> Closures 99
  int upvalueCount;
//< Closures 99
  Chunk chunk;
  ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj object;
  NativeFn function;
} ObjNative;
//< Calls and Functions 99

struct sObjString {
  Obj object;
  int length;
  char* chars;
//> Hash Tables 99
  uint32_t hash;
//< Hash Tables 99
};
//> Closures 99

typedef struct sUpvalue {
  Obj object;

  // Pointer to the variable this upvalue is referencing.
  Value* value;

  // If the upvalue is closed (i.e. the local variable it was pointing to has
  // been popped off the stack) then the closed-over value is hoisted out of
  // the stack into here. [value] is then be changed to point to this.
  Value closed;

  // Open upvalues are stored in a linked list. This points to the next one in
  // that list.
  struct sUpvalue* next;
} ObjUpvalue;

typedef struct {
  Obj object;
  ObjFunction* function;
  ObjUpvalue** upvalues;
  int upvalueCount;
} ObjClosure;
//< Closures 99
//> Classes and Instances 99

typedef struct sObjClass {
  Obj object;
  ObjString* name;
//> Superclasses 99
  struct sObjClass* superclass;
//< Superclasses 99
//> Methods and Initializers 99
  Table methods;
//< Methods and Initializers 99
} ObjClass;

typedef struct {
  Obj object;
  ObjClass* klass;
  Table fields;
} ObjInstance;
//< Classes and Instances 99
//> Methods and Initializers 99

typedef struct {
  Obj object;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
//< Methods and Initializers 99
/* Classes and Instances 99 < Superclasses 99
ObjClass* newClass(ObjString* name);
*/
//> Superclasses 99
ObjClass* newClass(ObjString* name, ObjClass* superclass);
//< Superclasses 99
//> Closures 99
ObjClosure* newClosure(ObjFunction* function);
//< Closures 99
//> Calls and Functions 99
ObjFunction* newFunction();
//< Calls and Functions 99
//> Classes and Instances 99
ObjInstance* newInstance(ObjClass* klass);
//< Classes and Instances 99
//> Calls and Functions 99
ObjNative* newNative(NativeFn function);
//< Calls and Functions 99
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
//> Closures 99
ObjUpvalue* newUpvalue(Value* slot);
//< Closures 99

// Returns true if [value] is an object of type [type]. Do not call this
// directly, instead use the [IS___] macro for the type in question.
static inline bool isObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
