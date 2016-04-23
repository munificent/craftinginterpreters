#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "table.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  free(table->entries);
}

static void growTable(Table* table) {
  if (table->capacity * TABLE_MAX_LOAD > table->count) return;
  
  if (table->capacity == 0) {
    table->capacity = 4;
  } else {
    table->capacity *= 2;
  }
  
  // TODO: Rehash everything.
  table->entries = REALLOCATE(table->entries, TableEntry, table->capacity);
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  // TODO: Actually hash it!
  for (int i = 0; i < table->count; i++) {
    TableEntry* entry = &table->entries[i];
    if (entry->key->length == key->length &&
        memcmp(entry->key->chars, key->chars, key->length) == 0) {
      *value = table->entries[i].value;
      return true;
    }
  }
  
  return false;
}

bool tableSet(Table* table, ObjString* key, Value value) {
  // TODO: Actually hash it!
  for (int i = 0; i < table->count; i++) {
    TableEntry* entry = &table->entries[i];
    if (entry->key->length == key->length &&
        memcmp(entry->key->chars, key->chars, key->length) == 0) {
      table->entries[i].value = value;
      return true;
    }
  }
  
  growTable(table);
  TableEntry* entry = &table->entries[table->count++];
  entry->key = key;
  entry->value = value;
  return false;
}

void grayTable(Table* table) {
  for (int i = 0; i < table->count; i++) {
    TableEntry* entry = &table->entries[i];
    grayValue((Value)entry->key);
    grayValue(entry->value);
  }
}
