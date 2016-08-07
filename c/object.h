//>= Strings
#ifndef cvox_object_h
#define cvox_object_h

#include "common.h"
#include "chunk.h"
//>= Uhh
#include "table.h"
//>= Strings

#define OBJ_TYPE(value)         ((value).as.object->type)

//>= Uhh
#define IS_BOUND_METHOD(value)  isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)         isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value)       isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)      isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value)        isObjType(value, OBJ_NATIVE)
//>= Strings
#define IS_STRING(value)        isObjType(value, OBJ_STRING)

//>= Uhh
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)         ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
//>= Strings
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
//>= Uhh
  OBJ_BOUND_METHOD,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
//>= Strings
  OBJ_STRING,
//>= Uhh
  OBJ_UPVALUE
//>= Strings
} ObjType;

struct sObj {
  ObjType type;
//>= Uhh
  // TODO: Stuff into low bit of next?
  bool isDark;
//>= Strings
  struct sObj* next;
};
//>= Uhh

typedef struct {
  Obj object;
  int arity;
  int upvalueCount;
  Chunk chunk;
  ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

// TODO: Make a value type?
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
//>= Uhh

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

typedef struct sObjClass {
  Obj object;
  ObjString* name;
  struct sObjClass* superclass;
  Table methods;
} ObjClass;

typedef struct {
  Obj object;
  ObjClass* klass;
  Table fields;
} ObjInstance;

typedef struct {
  Obj object;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
ObjClass* newClass(ObjString* name, ObjClass* superclass);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjInstance* newInstance(ObjClass* klass);
ObjNative* newNative(NativeFn function);
//>= Strings
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
//>= Uhh
ObjUpvalue* newUpvalue(Value* slot);
//>= Strings

// Returns true if [value] is an object of type [type]. Do not call this
// directly, instead use the [IS___] macro for the type in question.
static inline bool isObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
