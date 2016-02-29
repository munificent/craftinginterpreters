#ifndef cvox_vm_h
#define cvox_vm_h

#define MAX_STACK  256
#define MAX_HEAP   (10 * 1024 * 1024)

typedef struct sObj Obj;

// TODO: Variant type.
typedef Obj* Value;

typedef enum {
  OBJ_FORWARD,
  OBJ_NUMBER,
  OBJ_PAIR // TODO: Temp.
} ObjType;

struct sObj {
  ObjType type;
};

typedef struct {
  Obj obj;
  Obj* to;
} ObjForward;

typedef struct {
  Obj obj;
  double value;
} ObjNumber;

// TODO: Temp.
typedef struct {
  Obj obj;
  Value a;
  Value b;
} ObjPair;

typedef struct {
  Value stack[MAX_STACK];
  int stackSize;
  
  char* fromStart;
  char* fromEnd;
  char* toStart;
  char* toEnd;
} VM;

void initVM(VM* vm);

void collectGarbage(VM* vm);

// TODO: Temp.
void pushNumber(VM* vm, double value);
void pushPair(VM* vm);
void printStack(VM* vm);
void pop(VM* vm);

#endif
