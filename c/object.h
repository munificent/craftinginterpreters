//>= Strings 99
#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
//>= Classes and Instances 99
#include "table.h"
//>= Strings 99

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)

//>= Methods and Initializers 99
#define IS_BOUND_METHOD(value)  isObjType(value, OBJ_BOUND_METHOD)
//>= Classes and Instances 99
#define IS_CLASS(value)         isObjType(value, OBJ_CLASS)
//>= Closures 99
#define IS_CLOSURE(value)       isObjType(value, OBJ_CLOSURE)
//>= Calls and Functions 99
#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
//>= Classes and Instances 99
#define IS_INSTANCE(value)      isObjType(value, OBJ_INSTANCE)
//>= Calls and Functions 99
#define IS_NATIVE(value)        isObjType(value, OBJ_NATIVE)
//>= Strings 99
#define IS_STRING(value)        isObjType(value, OBJ_STRING)

//>= Methods and Initializers 99
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)AS_OBJ(value))
//>= Classes and Instances 99
#define AS_CLASS(value)         ((ObjClass*)AS_OBJ(value))
//>= Closures 99
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
//>= Calls and Functions 99
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
//>= Classes and Instances 99
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
//>= Calls and Functions 99
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
//>= Strings 99
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
//>= Methods and Initializers 99
  OBJ_BOUND_METHOD,
//>= Classes and Instances 99
  OBJ_CLASS,
//>= Closures 99
  OBJ_CLOSURE,
//>= Calls and Functions 99
  OBJ_FUNCTION,
//>= Classes and Instances 99
  OBJ_INSTANCE,
//>= Calls and Functions 99
  OBJ_NATIVE,
//>= Strings 99
  OBJ_STRING,
//>= Closures 99
  OBJ_UPVALUE
//>= Strings 99
} ObjType;

struct sObj {
  ObjType type;
//>= Garbage Collection 99
  bool isDark;
//>= Strings 99
  struct sObj* next;
};
//>= Calls and Functions 99

typedef struct {
  Obj object;
  int arity;
//>= Closures 99
  int upvalueCount;
//>= Calls and Functions 99
  Chunk chunk;
  ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj object;
  NativeFn function;
} ObjNative;
//>= Strings 99

struct sObjString {
  Obj object;
  int length;
  char* chars;
//>= Hash Tables 99
  uint32_t hash;
//>= Strings 99
};
//>= Closures 99

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
//>= Classes and Instances 99

typedef struct sObjClass {
  Obj object;
  ObjString* name;
//>= Superclasses 99
  struct sObjClass* superclass;
//>= Methods and Initializers 99
  Table methods;
//>= Classes and Instances 99
} ObjClass;

typedef struct {
  Obj object;
  ObjClass* klass;
  Table fields;
} ObjInstance;
//>= Methods and Initializers 99

typedef struct {
  Obj object;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
/*>= Classes and Instances 99 < Superclasses 99
ObjClass* newClass(ObjString* name);
*/
//>= Superclasses 99
ObjClass* newClass(ObjString* name, ObjClass* superclass);
//>= Closures 99
ObjClosure* newClosure(ObjFunction* function);
//>= Calls and Functions 99
ObjFunction* newFunction();
//>= Classes and Instances 99
ObjInstance* newInstance(ObjClass* klass);
//>= Calls and Functions 99
ObjNative* newNative(NativeFn function);
//>= Strings 99
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
//>= Closures 99
ObjUpvalue* newUpvalue(Value* slot);
//>= Strings 99

// Returns true if [value] is an object of type [type]. Do not call this
// directly, instead use the [IS___] macro for the type in question.
static inline bool isObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
