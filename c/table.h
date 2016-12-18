//> Hash Tables 99
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
/* Hash Tables 99 < Optimization 99
  int capacity;
*/
//> Optimization 99
  int capacityMask;
//< Optimization 99
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
//> Garbage Collection 99

void tableRemoveWhite(Table* table);
void grayTable(Table* table);
//< Garbage Collection 99

#endif
