#ifndef cvox_object_h
#define cvox_object_h

#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "table.h"

#define IS_BOOL(value)          (OBJ_TYPE(value) == OBJ_BOOL)
#define IS_BOUND_METHOD(value)  (OBJ_TYPE(value) == OBJ_BOUND_METHOD)
#define IS_CLASS(value)         (OBJ_TYPE(value) == OBJ_CLASS)
#define IS_CLOSURE(value)       (OBJ_TYPE(value) == OBJ_CLOSURE)
#define IS_FUNCTION(value)      (OBJ_TYPE(value) == OBJ_FUNCTION)
#define IS_INSTANCE(value)      (OBJ_TYPE(value) == OBJ_INSTANCE)
#define IS_NATIVE(value)        (OBJ_TYPE(value) == OBJ_NATIVE)
#define IS_NIL(value)           ((value) == NULL)
#define IS_NUMBER(value)        (OBJ_TYPE(value) == OBJ_NUMBER)
#define IS_STRING(value)        (OBJ_TYPE(value) == OBJ_STRING)

#define AS_BOOL(val)            (((ObjBool*)val)->value)
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)value)
#define AS_CLASS(value)         ((ObjClass*)value)
#define AS_CLOSURE(value)       ((ObjClosure*)value)
#define AS_FUNCTION(value)      ((ObjFunction*)value)
#define AS_INSTANCE(value)      ((ObjInstance*)value)
#define AS_NUMBER(val)          (((ObjNumber*)val)->value)
#define AS_NATIVE(value)        (((ObjNative*)value)->function)
#define AS_STRING(value)        ((ObjString*)value)
#define AS_CSTRING(value)       (((ObjString*)value)->chars)

#define OBJ_TYPE(value) (objectType(value))

// TODO: Unboxed numbers?

typedef enum {
  OBJ_BOOL,
  OBJ_BOUND_METHOD,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_NIL,
  OBJ_NUMBER,
  OBJ_STRING,
  OBJ_UPVALUE
} ObjType;

struct sObj {
  ObjType type;
  // TODO: Stuff into low bit of next?
  bool isDark;
  
  struct sObj* next;
};

typedef struct {
  Value* values;
  int capacity;
  int count;
} ValueArray;

typedef struct {
  Obj obj;
  bool value;
} ObjBool;

typedef struct {
  Obj obj;

  int codeCount;
  int codeCapacity;
  uint8_t* code;
  int arity;
  int upvalueCount;
  ValueArray constants;

  // Debug information.
  ObjString* name;
  int* codeLines;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

typedef struct {
  Obj obj;
  double value;
} ObjNumber;

struct sObjString {
  Obj obj;
  int length;
  const char* chars;
  uint32_t hash;
};

typedef struct sUpvalue {
  Obj obj;
  
  // Pointer to the variable this upvalue is referencing.
  Value* value;
  
  // If the upvalue is closed (i.e. the local variable it was pointing too has
  // been popped off the stack) then the closed-over value is hoisted out of
  // the stack into here. [value] is then be changed to point to this.
  Value closed;
  
  // Open upvalues are stored in a linked list. This points to the next one in
  // that list.
  struct sUpvalue* next;
} ObjUpvalue;

typedef struct {
  Obj obj;
  ObjFunction* function;
  ObjUpvalue** upvalues;
} ObjClosure;

typedef struct sObjClass {
  Obj obj;
  ObjString* name;
  struct sObjClass* superclass;
  Table methods;
} ObjClass;

typedef struct {
  Obj obj;
  ObjClass* klass;
  Table fields;
} ObjInstance;

typedef struct {
  Obj obj;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;

ObjBool* newBool(bool value);
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
ObjClass* newClass(ObjString* name, ObjClass* superclass);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjInstance* newInstance(ObjClass* klass);
ObjNative* newNative(NativeFn function);
ObjNumber* newNumber(double value);
ObjString* takeString(const char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjUpvalue* newUpvalue(Value* slot);

bool valuesEqual(Value a, Value b);

void initArray(ValueArray* array);
void growArray(ValueArray* array);
void freeArray(ValueArray* array);

// Returns the type of [value]. Do not call this directly. Instead, use the
// `IS_X` macros or `OBJ_TYPE`.
static inline ObjType objectType(Value value) {
  return value == NULL ? OBJ_NIL : value->type;
}

#endif
