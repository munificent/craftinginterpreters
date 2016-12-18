//>= Hash Tables 99
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
  table->count = 0;
/*>= Hash Tables 99 < Optimization 99
  table->capacity = 0;
*/
//>= Optimization 99
  table->capacityMask = -1;
//>= Hash Tables 99
  table->entries = NULL;
}

void freeTable(Table* table) {
/*>= Hash Tables 99 < Optimization 99
  FREE_ARRAY(Value, table->entries, table->capacity);
*/
//>= Optimization 99
  FREE_ARRAY(Value, table->entries, table->capacityMask + 1);
//>= Hash Tables 99
  initTable(table);
}

// Finds the entry where [key] should be. If the key is not already present in
// the table, this will be an unused entry. Otherwise, it will be the existing
// entry for that key.
/*>= Hash Tables 99 < Optimization 99
static uint32_t findEntry(Entry* entries, int capacity, ObjString* key) {
*/
//>= Optimization 99
static uint32_t findEntry(Entry* entries, int capacityMask, ObjString* key) {
//>= Hash Tables 99
  // Figure out where to insert it in the table. Use open addressing and
  // basic linear probing.
/*>= Hash Tables 99 < Optimization 99
  uint32_t index = key->hash % capacity;
*/
//>= Optimization 99
  uint32_t index = key->hash & capacityMask;
//>= Hash Tables 99

  // We don't worry about an infinite loop here because resize() ensures
  // there are empty slots in the array.
  for (;;) {
    Entry* entry = &entries[index];

    if (entry->key == NULL || entry->key == key) return index;

    // Try the next slot.
/*>= Hash Tables 99 < Optimization 99
    index = (index + 1) % capacity;
*/
//>= Optimization 99
    index = (index + 1) & capacityMask;
//>= Hash Tables 99
  }
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  // If the table is empty, we definitely won't find it.
  if (table->entries == NULL) return false;

/*>= Hash Tables 99 < Optimization 99
  uint32_t index = findEntry(table->entries, table->capacity, key);
*/
//>= Optimization 99
  uint32_t index = findEntry(table->entries, table->capacityMask, key);
//>= Hash Tables 99
  Entry* entry = &table->entries[index];
  if (entry->key == NULL) return false;

  *value = entry->value;
  return true;
}

/*>= Hash Tables 99 < Optimization 99
static void resize(Table* table, int capacity) {
*/
//>= Optimization 99
static void resize(Table* table, int capacityMask) {
//>= Hash Tables 99
  // Create the new empty entry array.
/*>= Hash Tables 99 < Optimization 99
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
*/
//>= Optimization 99
  Entry* entries = ALLOCATE(Entry, capacityMask + 1);
  for (int i = 0; i <= capacityMask; i++) {
//>= Hash Tables 99
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  // Re-hash the existing entries into the new array.
  table->count = 0;
/*>= Hash Tables 99 < Optimization 99
  for (int i = 0; i < table->capacity; i++) {
*/
//>= Optimization 99
  for (int i = 0; i <= table->capacityMask; i++) {
//>= Hash Tables 99
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

/*>= Hash Tables 99 < Optimization 99
    uint32_t index = findEntry(entries, capacity, entry->key);
*/
//>= Optimization 99
    uint32_t index = findEntry(entries, capacityMask, entry->key);
//>= Hash Tables 99
    Entry* dest = &entries[index];
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  // Replace the array.
/*>= Hash Tables 99 < Optimization 99
  FREE_ARRAY(Value, table->entries, table->capacity);
*/
//>= Optimization 99
  FREE_ARRAY(Value, table->entries, table->capacityMask + 1);
//>= Hash Tables 99
  table->entries = entries;
/*>= Hash Tables 99 < Optimization 99
  table->capacity = capacity;
*/
//>= Optimization 99
  table->capacityMask = capacityMask;
//>= Hash Tables 99
}

bool tableSet(Table* table, ObjString* key, Value value) {
  // If the table is getting too full, make room first.
/*>= Hash Tables 99 < Optimization 99
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    // Figure out the new table size.
    int capacity = GROW_CAPACITY(table->capacity);
    resize(table, capacity);
*/
//>= Optimization 99
  if (table->count + 1 > (table->capacityMask + 1) * TABLE_MAX_LOAD) {
    // Figure out the new table size.
    int capacityMask = GROW_CAPACITY(table->capacityMask + 1) - 1;
    resize(table, capacityMask);
//>= Hash Tables 99
  }

/*>= Hash Tables 99 < Optimization 99
  uint32_t index = findEntry(table->entries, table->capacity, key);
*/
//>= Optimization 99
  uint32_t index = findEntry(table->entries, table->capacityMask, key);
//>= Hash Tables 99
  Entry* entry = &table->entries[index];
  bool isNewKey = entry->key == NULL;
  entry->key = key;
  entry->value = value;

  if (isNewKey) table->count++;
  return isNewKey;
}

bool tableDelete(Table* table, ObjString* key) {
  if (table->count == 0) return false;

  // Find the entry.
/*>= Hash Tables 99 < Optimization 99
  uint32_t index = findEntry(table->entries, table->capacity, key);
*/
//>= Optimization 99
  uint32_t index = findEntry(table->entries, table->capacityMask, key);
//>= Hash Tables 99
  Entry* entry = &table->entries[index];
  if (entry->key == NULL) return false;

  // Remove the entry.
  entry->key = NULL;
  entry->value = NIL_VAL;
  table->count--;

  // Later entries may have been pushed past this one and may need to be pushed
  // up to fill the hole. The simplest way to handle that is to just re-add
  // them all until we hit an empty entry.
  for (;;) {
/*>= Hash Tables 99 < Optimization 99
    index = (index + 1) % table->capacity;
*/
//>= Optimization 99
    index = (index + 1) & table->capacityMask;
//>= Hash Tables 99
    entry = &table->entries[index];

    if (entry->key == NULL) break;

    ObjString* tempKey = entry->key;
    Value tempValue = entry->value;
    entry->key = NULL;
    entry->value = NIL_VAL;
    table->count--;

    tableSet(table, tempKey, tempValue);
  }

  return true;
}

void tableAddAll(Table* from, Table* to) {
/*>= Hash Tables 99 < Optimization 99
  for (int i = 0; i < from->capacity; i++) {
*/
//>= Optimization 99
  for (int i = 0; i <= from->capacityMask; i++) {
//>= Hash Tables 99
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

ObjString* tableFindString(Table* table, const char* chars, int length,
                           uint32_t hash) {
  // If the table is empty, we definitely won't find it.
  if (table->entries == NULL) return NULL;

  // Figure out where to insert it in the table. Use open addressing and
  // basic linear probing.
/*>= Hash Tables 99 < Optimization 99
  uint32_t index = hash % table->capacity;
*/
//>= Optimization 99
  uint32_t index = hash & table->capacityMask;
//>= Hash Tables 99

  for (;;) {
    Entry* entry = &table->entries[index];

    if (entry->key == NULL) return NULL;
    if (entry->key->length == length &&
        memcmp(entry->key->chars, chars, length) == 0) {
      // We found it.
      return entry->key;
    }

    // Try the next slot.
/*>= Hash Tables 99 < Optimization 99
    index = (index + 1) % table->capacity;
*/
//>= Optimization 99
    index = (index + 1) & table->capacityMask;
//>= Hash Tables 99
  }

  return NULL;
}
//>= Garbage Collection 99

void tableRemoveWhite(Table* table) {
/*>= Garbage Collection 99 < Optimization 99
  for (int i = 0; i < table->capacity; i++) {
*/
//>= Optimization 99
  for (int i = 0; i <= table->capacityMask; i++) {
//>= Garbage Collection 99
    Entry* entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->object.isDark) {
      tableDelete(table, entry->key);
    }
  }
}

void grayTable(Table* table) {
/*>= Garbage Collection 99 < Optimization 99
  for (int i = 0; i < table->capacity; i++) {
*/
//>= Optimization 99
  for (int i = 0; i <= table->capacityMask; i++) {
//>= Garbage Collection 99
    Entry* entry = &table->entries[i];
    grayObject((Obj*)entry->key);
    grayValue(entry->value);
  }
}
