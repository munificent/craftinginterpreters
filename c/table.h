#ifndef cvox_table_h
#define cvox_table_h

#include "common.h"

typedef struct {
  ObjString* key;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);

bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
void tableAddAll(Table* from, Table* to);

ObjString* tableFindKey(Table* table, const uint8_t* chars, int length);
void tableRemoveWhite(Table* table);

void grayTable(Table* table);

#endif
