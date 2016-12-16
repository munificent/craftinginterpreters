//>= Hash Tables 1
#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

typedef struct {
  ObjString* key;
  Value value;
} Entry;

typedef struct {
  int count;
/*>= Hash Tables 1 < Optimization 1
  int capacity;
*/
//>= Optimization 1
  int capacityMask;
//>= Hash Tables 1
  Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);

bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);

ObjString* tableFindString(Table* table, const char* chars, int length,
                           uint32_t hash);
//>= Garbage Collection 1

void tableRemoveWhite(Table* table);
void grayTable(Table* table);
//>= Hash Tables 1

#endif
