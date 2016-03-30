#ifndef cvox_object_h
#define cvox_object_h

#include <stdbool.h>
#include <stdint.h>

typedef struct sVM VM;

#define IS_BOOL(value) isNonNullType((value), OBJ_BOOL)
#define IS_CLASS(value) isNonNullType((value), OBJ_CLASS)
#define IS_CLOSURE(value) isNonNullType((value), OBJ_CLOSURE)
#define IS_FUNCTION(value) isNonNullType((value), OBJ_FUNCTION)
#define IS_INSTANCE(value) isNonNullType((value), OBJ_INSTANCE)
#define IS_NUMBER(value) isNonNullType((value), OBJ_NUMBER)
#define IS_NULL(value) ((value) == NULL)
#define IS_NATIVE(value) isNonNullType((value), OBJ_NATIVE)
#define IS_STRING(value) isNonNullType((value), OBJ_STRING)
#define IS_TABLE(value) isNonNullType((value), OBJ_TABLE)

// TODO: Unboxed numbers?

typedef enum {
  OBJ_BOOL,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_NUMBER,
  OBJ_STRING,
  OBJ_TABLE,
  OBJ_UPVALUE
} ObjType;

typedef struct sObj {
  ObjType type;
  // TODO: Stuff into low bit of next?
  bool isDark;
  
  struct sObj* next;
} Obj;

typedef Obj* Value;

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
  int* codeLines;
  int arity;
  int upvalueCount;
  
  ValueArray constants;
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

typedef struct {
  Obj obj;
  int length;
  char* chars;
} ObjString;

// TODO: Bare tables are not first class in the language, so don't make this
// an Obj?
typedef struct {
  ObjString* key;
  Value value;
} TableEntry;

typedef struct {
  Obj obj;
  int count;
  int capacity;
  TableEntry* entries;
} ObjTable;

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

typedef struct {
  Obj obj;
  ObjString* name;
  ObjTable* methods;
} ObjClass;

typedef struct {
  Obj obj;
  ObjClass* klass;
  // TODO: Rename properties to fields in jvox.
  ObjTable* fields;
} ObjInstance;

// TODO: Move elsewhere?
void* reallocate(void* previous, size_t size);

ObjBool* newBool(bool value);
ObjClass* newClass(ObjString* name, Value superclass);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjInstance* newInstance(ObjClass* klass);
ObjNative* newNative(NativeFn function);
ObjNumber* newNumber(double value);
// TODO: int or size_t for length?
ObjString* newString(const uint8_t* chars, int length);
ObjTable* newTable();
ObjUpvalue* newUpvalue(Value* slot);
bool tableGet(ObjTable* table, ObjString* key, Value* value);
bool tableSet(ObjTable* table, ObjString* key, Value value);

bool valuesEqual(Value a, Value b);

void initArray(ValueArray* array);
// TODO: Better name. "growCapacity"?
void ensureArrayCapacity(ValueArray* array);
void freeArray(ValueArray* array);
  
void grayValue(Value value);
void freeObject(Obj* obj);
void collectGarbage();

// Returns true if [value] is an object of type [type]. Do not call this
// directly, instead use the [IS___] macro for the type in question.
static inline bool isNonNullType(Value value, ObjType type)
{
  return value != NULL && value->type == type;
}

#endif
