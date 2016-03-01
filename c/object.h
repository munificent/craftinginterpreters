#ifndef cvox_object_h
#define cvox_object_h

#include <stdint.h>

typedef struct sObj Obj;
typedef struct sVM VM;

// TODO: Unboxed numbers?
typedef Obj* Value;

typedef enum {
  OBJ_ARRAY,
  OBJ_FORWARD,
  OBJ_FUNCTION,
  OBJ_NUMBER,
  OBJ_STRING,
  OBJ_TABLE,
  OBJ_TABLE_ENTRIES,
} ObjType;

struct sObj {
  ObjType type;
};

typedef struct {
  Obj obj;
  int size;
  Value elements[];
} ObjArray;

typedef struct {
  Obj obj;
  Obj* to;
} ObjForward;

typedef struct {
  Obj obj;
  ObjArray* constants;
  int codeSize;
  uint8_t code[];
} ObjFunction;

typedef struct {
  Obj obj;
  double value;
} ObjNumber;

typedef struct {
  Obj obj;
  int length;
  char chars[];
} ObjString;

typedef struct {
  Value key;
  Value value;
} TableEntry;

typedef struct {
  Obj obj;
  int size;
  TableEntry entries[];
} ObjTableEntries;

typedef struct {
  Obj obj;
  int count;
  ObjTableEntries* entries;
} ObjTable;

// TODO: int or size_t for size? String too?
ObjArray* newArray(VM* vm, int size);
ObjFunction* newFunction(VM* vm, uint8_t* code, int codeSize, ObjArray* constants);
ObjNumber* newNumber(VM* vm, double value);
ObjString* newString(VM* vm, const char* chars, int length);
ObjTable* newTable(VM* vm);

ObjArray* ensureArraySize(VM* vm, ObjArray* array, int size);

void collectGarbage(VM* vm);
Value moveObject(VM* vm, Value value);
size_t objectSize(Obj* obj);
void traverseObject(VM* vm, Obj* obj);

#endif
