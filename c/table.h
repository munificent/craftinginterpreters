#ifndef cvox_table_h
#define cvox_table_h

#include "common.h"

typedef struct {
  ObjString* key;
  Value value;
} TableEntry;

typedef struct {
  int count;
  int capacity;
  TableEntry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);

bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);

void grayTable(Table* table);

#endif
