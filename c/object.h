#ifndef cvox_object_h
#define cvox_object_h

#include <stdbool.h>
#include <stdint.h>

typedef struct sVM VM;

// TODO: Unboxed numbers?

typedef enum {
  OBJ_BOOL,
  OBJ_FUNCTION,
  OBJ_NUMBER,
  OBJ_STRING,
  OBJ_TABLE,
} ObjType;

typedef struct sObj {
  ObjType type;
  // TODO: Stuff into low bit of next?
  bool isDark;
  
  struct sObj* next;
} Obj;

typedef Obj* Value;

typedef struct {
  Obj obj;
  bool value;
} ObjBool;

typedef struct {
  Obj obj;

  int codeCount;
  int codeCapacity;
  uint8_t* code;

  int constantCount;
  int constantCapacity;
  Value* constants;
} ObjFunction;

typedef struct {
  Obj obj;
  double value;
} ObjNumber;

typedef struct {
  Obj obj;
  int length;
  char* chars;
} ObjString;

typedef struct {
  Value key;
  Value value;
} TableEntry;

typedef struct {
  Obj obj;
  int count;
  int capacity;
  TableEntry* entries;
} ObjTable;

// TODO: Move elsewhere?
void* reallocate(void* previous, size_t size);

ObjBool* newBool(bool value);
ObjFunction* newFunction();
ObjNumber* newNumber(double value);
// TODO: int or size_t for length?
ObjString* newString(const uint8_t* chars, int length);
ObjTable* newTable();

void grayValue(Value value);
void freeObject(Obj* obj);
void collectGarbage();

#endif
