//> Strings object-h
#ifndef clox_object_h
#define clox_object_h

#include "common.h"
//> Calls and Functions object-include-chunk
#include "chunk.h"
//< Calls and Functions object-include-chunk
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
//> Closures is-closure
#define IS_CLOSURE(value)       isObjType(value, OBJ_CLOSURE)
//< Closures is-closure
//> Calls and Functions is-function
#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
//< Calls and Functions is-function
//> Classes and Instances not-yet
#define IS_INSTANCE(value)      isObjType(value, OBJ_INSTANCE)
//< Classes and Instances not-yet
//> Calls and Functions is-native
#define IS_NATIVE(value)        isObjType(value, OBJ_NATIVE)
//< Calls and Functions is-native
#define IS_STRING(value)        isObjType(value, OBJ_STRING)
//< is-string
//> as-string

//> Methods and Initializers not-yet
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)AS_OBJ(value))
//< Methods and Initializers not-yet
//> Classes and Instances not-yet
#define AS_CLASS(value)         ((ObjClass*)AS_OBJ(value))
//< Classes and Instances not-yet
//> Closures as-closure
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
//< Closures as-closure
//> Calls and Functions as-function
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
//< Calls and Functions as-function
//> Classes and Instances not-yet
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
//< Classes and Instances not-yet
//> Calls and Functions as-native
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
//< Calls and Functions as-native
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
//> Closures obj-type-closure
  OBJ_CLOSURE,
//< Closures obj-type-closure
//> Calls and Functions obj-type-function
  OBJ_FUNCTION,
//< Calls and Functions obj-type-function
//> Classes and Instances not-yet
  OBJ_INSTANCE,
//< Classes and Instances not-yet
//> Calls and Functions obj-type-native
  OBJ_NATIVE,
//< Calls and Functions obj-type-native
  OBJ_STRING,
//> Closures obj-type-upvalue
  OBJ_UPVALUE
//< Closures obj-type-upvalue
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
//> Calls and Functions obj-function

typedef struct {
  Obj obj;
  int arity;
//> Closures upvalue-count
  int upvalueCount;
//< Closures upvalue-count
  Chunk chunk;
  ObjString* name;
} ObjFunction;
//< Calls and Functions obj-function
//> Calls and Functions obj-native

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;
//< Calls and Functions obj-native
//> obj-string

struct sObjString {
  Obj obj;
  int length;
  char* chars;
//> Hash Tables obj-string-hash
  uint32_t hash;
//< Hash Tables obj-string-hash
};
//< obj-string
//> Closures obj-upvalue
typedef struct sUpvalue {
  Obj obj;
  Value* value;
//> closed-field
  Value closed;
//< closed-field
//> next-field
  struct sUpvalue* next;
//< next-field
} ObjUpvalue;
//< Closures obj-upvalue
//> Closures obj-closure
typedef struct {
  Obj obj;
  ObjFunction* function;
//> upvalue-fields
  ObjUpvalue** upvalues;
  int upvalueCount;
//< upvalue-fields
} ObjClosure;
//< Closures obj-closure
//> Classes and Instances not-yet

typedef struct sObjClass {
  Obj obj;
  ObjString* name;
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
//> Classes and Instances not-yet
ObjClass* newClass(ObjString* name);
//< Classes and Instances not-yet
//> Closures new-closure-h
ObjClosure* newClosure(ObjFunction* function);
//< Closures new-closure-h
//> Calls and Functions new-function-h
ObjFunction* newFunction();
//< Calls and Functions new-function-h
//> Classes and Instances not-yet
ObjInstance* newInstance(ObjClass* klass);
//< Classes and Instances not-yet
//> Calls and Functions new-native-h
ObjNative* newNative(NativeFn function);
//< Calls and Functions new-native-h
//> take-string-h
ObjString* takeString(char* chars, int length);
//< take-string-h
//> copy-string-h
ObjString* copyString(const char* chars, int length);
//> Closures new-upvalue-h
ObjUpvalue* newUpvalue(Value* slot);
//< Closures new-upvalue-h
//> print-object-h
void printObject(Value value);
//< print-object-h

//< copy-string-h
//> is-obj-type
static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

//< is-obj-type
#endif
