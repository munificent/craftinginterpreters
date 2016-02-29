#ifndef cvox_object_h
#define cvox_object_h

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
  char code[];
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
Value newArray(VM* vm, int size);
Value newNumber(VM* vm, double value);
Value newString(VM* vm, const char* chars, int length);
Value newTable(VM* vm);

void collectGarbage(VM* vm);
Value moveObject(VM* vm, Value value);
size_t objectSize(Obj* obj);
void traverseObject(VM* vm, Obj* obj);

void printValue(Value value);

#endif
