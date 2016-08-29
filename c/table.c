//>= Hash Tables
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
  table->count = 0;
/*>= Hash Tables < Optimization
  table->capacity = 0;
*/
//>= Optimization
  table->capacityMask = -1;
//>= Hash Tables
  table->entries = NULL;
}

void freeTable(Table* table) {
/*>= Hash Tables < Optimization
  FREE_ARRAY(Value, table->entries, table->capacity);
*/
//>= Optimization
  FREE_ARRAY(Value, table->entries, table->capacityMask + 1);
//>= Hash Tables
  initTable(table);
}

// Finds the entry where [key] should be. If the key is not already present in
// the table, this will be an unused entry. Otherwise, it will be the existing
// entry for that key.
/*>= Hash Tables < Optimization
static Entry* findEntry(Entry* entries, int capacity, ObjString* key,
*/
//>= Optimization
static Entry* findEntry(Entry* entries, int capacityMask, ObjString* key,
//>= Hash Tables
                        bool stopAtTombstone) {
  // Figure out where to insert it in the table. Use open addressing and
  // basic linear probing.
/*>= Hash Tables < Optimization
  uint32_t index = key->hash % capacity;
*/
//>= Optimization
  uint32_t index = key->hash & capacityMask;
//>= Hash Tables

  // We don't worry about an infinite loop here because resize() ensures
  // there are empty slots in the array.
  for (;;) {
    Entry* entry = &entries[index];
    
    if (entry->key == NULL) {
      // Stop on either an empty entry (the value is nil) or a tombstone (the
      // value is non-nil) if we are supposed to.
      if (IS_NIL(entry->value) || stopAtTombstone) return entry;
    } else if (key == entry->key) {
      // We found it.
      return entry;
    }
    
    // Try the next slot.
/*>= Hash Tables < Optimization
    index = (index + 1) % capacity;
*/
//>= Optimization
    index = (index + 1) & capacityMask;
//>= Hash Tables
  }
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  // If the table is empty, we definitely won't find it.
  if (table->entries == NULL) return false;
  
/*>= Hash Tables < Optimization
  Entry* entry = findEntry(table->entries, table->capacity, key, false);
*/
//>= Optimization
  Entry* entry = findEntry(table->entries, table->capacityMask, key, false);
//>= Hash Tables
  if (entry->key == NULL) return false;
  
  *value = entry->value;
  return true;
}

#include <stdio.h>

/*>= Hash Tables < Optimization
static void resize(Table* table, int capacity) {
*/
//>= Optimization
static void resize(Table* table, int capacityMask) {
//>= Hash Tables
  // Create the new empty entry array.
/*>= Hash Tables < Optimization
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
*/
//>= Optimization
  Entry* entries = ALLOCATE(Entry, capacityMask + 1);
  for (int i = 0; i <= capacityMask; i++) {
//>= Hash Tables
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  // Re-hash the existing entries into the new array.
  table->count = 0;
/*>= Hash Tables < Optimization
  for (int i = 0; i < table->capacity; i++) {
*/
//>= Optimization
  for (int i = 0; i <= table->capacityMask; i++) {
//>= Hash Tables
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

/*>= Hash Tables < Optimization
    Entry* dest = findEntry(entries, capacity, entry->key, false);
*/
//>= Optimization
    Entry* dest = findEntry(entries, capacityMask, entry->key, false);
//>= Hash Tables
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  // Replace the array.
/*>= Hash Tables < Optimization
  FREE_ARRAY(Value, table->entries, table->capacity);
*/
//>= Optimization
  FREE_ARRAY(Value, table->entries, table->capacityMask + 1);
//>= Hash Tables
  table->entries = entries;
/*>= Hash Tables < Optimization
  table->capacity = capacity;
*/
//>= Optimization
  table->capacityMask = capacityMask;
//>= Hash Tables
}

bool tableSet(Table* table, ObjString* key, Value value) {
  // If the table is getting too full, make room first.
/*>= Hash Tables < Optimization
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    // Figure out the new table size.
    int capacity = GROW_CAPACITY(table->capacity);
    resize(table, capacity);
*/
//>= Optimization
  if (table->count + 1 > (table->capacityMask + 1) * TABLE_MAX_LOAD) {
    // Figure out the new table size.
    int capacityMask = GROW_CAPACITY(table->capacityMask + 1) - 1;
    resize(table, capacityMask);
//>= Hash Tables
  }

/*>= Hash Tables < Optimization
  Entry* entry = findEntry(table->entries, table->capacity, key, false);
*/
//>= Optimization
  Entry* entry = findEntry(table->entries, table->capacityMask, key, false);
//>= Hash Tables
  bool isNewKey = entry->key == NULL;
  entry->key = key;
  entry->value = value;

  if (isNewKey) table->count++;
  return isNewKey;
}

// TODO: This isn't actually used by anything. It's here just to show a more
// complete hash table implementation. Can we use it?
bool tableDelete(Table* table, ObjString* key) {
/*>= Hash Tables < Optimization
  Entry* entry = findEntry(table->entries, table->capacity, key, false);
*/
//>= Optimization
  Entry* entry = findEntry(table->entries, table->capacityMask, key, false);
//>= Hash Tables
  if (entry->key == NULL) return false;
  
  // Leave a tombstone.
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

void tableAddAll(Table* from, Table* to) {
/*>= Hash Tables < Optimization
  for (int i = 0; i < from->capacity; i++) {
*/
//>= Optimization
  for (int i = 0; i <= from->capacityMask; i++) {
//>= Hash Tables
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
/*>= Hash Tables < Optimization
  uint32_t index = hash % table->capacity;
*/
//>= Optimization
  uint32_t index = hash & table->capacityMask;
//>= Hash Tables
  
  for (;;) {
    Entry* entry = &table->entries[index];
    
    if (entry->key == NULL) {
      // If the value is non-nil, it's a tombstone and we have to keep looking.
      if (IS_NIL(entry->value)) return NULL;
    } else if (entry->key->length == length &&
               memcmp(entry->key->chars, chars, length) == 0) {
      // We found it.
      return entry->key;
    }
    
    // Try the next slot.
/*>= Hash Tables < Optimization
    index = (index + 1) % table->capacity;
*/
//>= Optimization
    index = (index + 1) & table->capacityMask;
//>= Hash Tables
  }
  
  return NULL;
}
//>= Garbage Collection

void tableRemoveWhite(Table* table) {
/*>= Garbage Collection < Optimization
  for (int i = 0; i < table->capacity; i++) {
*/
//>= Optimization
  for (int i = 0; i <= table->capacityMask; i++) {
//>= Garbage Collection
    Entry* entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->object.isDark) {
      // Turn the entry into a tombstone, identified as having a NULL key but a
      // non-nil value (true). Don't adjust the count. We want to treat
      // tombstone entries as used so that we don't end up with an array full
      // of tombstones.
      entry->key = NULL;
      entry->value = BOOL_VAL(true);
    }
  }
}

void grayTable(Table* table) {
/*>= Garbage Collection < Optimization
  for (int i = 0; i < table->capacity; i++) {
*/
//>= Optimization
  for (int i = 0; i <= table->capacityMask; i++) {
//>= Garbage Collection
    Entry* entry = &table->entries[i];
    grayObject((Obj*)entry->key);
    grayValue(entry->value);
  }
}
