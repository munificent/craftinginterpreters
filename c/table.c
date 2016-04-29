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

TableEntry* findEntry(Table* table, const uint8_t* chars, int length) {
  // TODO: Actually hash it!
  for (int i = 0; i < table->count; i++) {
    TableEntry* entry = &table->entries[i];
    if (entry->key->length == length &&
        memcmp(entry->key->chars, chars, length) == 0) {
      return entry;
    }
  }
  
  return NULL;
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  TableEntry* entry = findEntry(table, key->chars, key->length);
  if (entry == NULL) return false;
  
  *value = entry->value;
  return true;
}

bool tableSet(Table* table, ObjString* key, Value value) {
  // Reuse the existing entry if the key is already present.
  TableEntry* entry = findEntry(table, key->chars, key->length);
  if (entry != NULL) {
    entry->value = value;
    return true;
  }
  
  growTable(table);
  entry = &table->entries[table->count++];
  entry->key = key;
  entry->value = value;
  return false;
}

ObjString* tableFindKey(Table* table, const uint8_t* chars, int length) {
  TableEntry* entry = findEntry(table, chars, length);
  if (entry == NULL) return NULL;
  
  return entry->key;
}

void tableRemoveWhite(Table* table) {
  int dest = 0;
  for (int i = 0; i < table->count; i++) {
    TableEntry* entry = &table->entries[i];
    if (entry->key->obj.isDark) table->entries[dest++] = *entry;
  }
  
  table->count = dest;
}

void grayTable(Table* table) {
  for (int i = 0; i < table->count; i++) {
    TableEntry* entry = &table->entries[i];
    grayValue((Value)entry->key);
    grayValue(entry->value);
  }
}
