//>= Strings
#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
//>= Classes and Instances
#include "table.h"
//>= Strings

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)

//>= Methods and Initializers
#define IS_BOUND_METHOD(value)  isObjType(value, OBJ_BOUND_METHOD)
//>= Classes and Instances
#define IS_CLASS(value)         isObjType(value, OBJ_CLASS)
//>= Closures
#define IS_CLOSURE(value)       isObjType(value, OBJ_CLOSURE)
//>= Functions
#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
//>= Classes and Instances
#define IS_INSTANCE(value)      isObjType(value, OBJ_INSTANCE)
//>= Native Functions
#define IS_NATIVE(value)        isObjType(value, OBJ_NATIVE)
//>= Strings
#define IS_STRING(value)        isObjType(value, OBJ_STRING)

//>= Methods and Initializers
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)AS_OBJ(value))
//>= Classes and Instances
#define AS_CLASS(value)         ((ObjClass*)AS_OBJ(value))
//>= Closures
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
//>= Functions
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
//>= Classes and Instances
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
//>= Native Functions
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
//>= Strings
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
//>= Methods and Initializers
  OBJ_BOUND_METHOD,
//>= Classes and Instances
  OBJ_CLASS,
//>= Closures
  OBJ_CLOSURE,
//>= Functions
  OBJ_FUNCTION,
//>= Classes and Instances
  OBJ_INSTANCE,
//>= Native Functions
  OBJ_NATIVE,
//>= Strings
  OBJ_STRING,
//>= Closures
  OBJ_UPVALUE
//>= Strings
} ObjType;

struct sObj {
  ObjType type;
//>= Garbage Collection
  bool isDark;
//>= Strings
  struct sObj* next;
};
//>= Functions

typedef struct {
  Obj object;
  int arity;
//>= Closures
  int upvalueCount;
//>= Functions
  Chunk chunk;
  ObjString* name;
} ObjFunction;
//>= Native Functions

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj object;
  NativeFn function;
} ObjNative;
//>= Strings

struct sObjString {
  Obj object;
  int length;
  char* chars;
//>= Hash Tables
  uint32_t hash;
//>= Strings
};
//>= Closures

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
//>= Classes and Instances

typedef struct sObjClass {
  Obj object;
  ObjString* name;
//>= Inheritance
  struct sObjClass* superclass;
//>= Methods and Initializers
  Table methods;
//>= Classes and Instances
} ObjClass;

typedef struct {
  Obj object;
  ObjClass* klass;
  Table fields;
} ObjInstance;
//>= Methods and Initializers

typedef struct {
  Obj object;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
/*>= Classes and Instances <= Methods and Initializers
ObjClass* newClass(ObjString* name);
*/
//>= Inheritance
ObjClass* newClass(ObjString* name, ObjClass* superclass);
//>= Closures
ObjClosure* newClosure(ObjFunction* function);
//>= Functions
ObjFunction* newFunction();
//>= Classes and Instances
ObjInstance* newInstance(ObjClass* klass);
//>= Native Functions
ObjNative* newNative(NativeFn function);
//>= Strings
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
//>= Closures
ObjUpvalue* newUpvalue(Value* slot);
//>= Strings

// Returns true if [value] is an object of type [type]. Do not call this
// directly, instead use the [IS___] macro for the type in question.
static inline bool isObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
