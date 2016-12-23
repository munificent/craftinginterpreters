//> Hash Tables not-yet
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
/* Hash Tables not-yet < Optimization not-yet
  int capacity;
*/
//> Optimization not-yet
  int capacityMask;
//< Optimization not-yet
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
//> Garbage Collection not-yet

void tableRemoveWhite(Table* table);
void grayTable(Table* table);
//< Garbage Collection not-yet

#endif
