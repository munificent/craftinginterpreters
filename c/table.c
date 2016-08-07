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
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(Table* table) {
  FREE_ARRAY(Value, table->entries, table->capacity);
  initTable(table);
}

// Finds the entry where [key] should be. If the key is not already present in
// the table, this will be an unused entry. Otherwise, it will be the existing
// entry for that key.
static Entry* findEntry(Entry* entries, int capacity, ObjString* key,
                        bool stopAtTombstone) {
  // Figure out where to insert it in the table. Use open addressing and
  // basic linear probing.
  uint32_t index = key->hash % capacity;
  
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
    index = (index + 1) % capacity;
  }
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  // If the table is empty, we definitely won't find it.
  if (table->entries == NULL) return false;
  
  Entry* entry = findEntry(table->entries, table->capacity, key, false);
  if (entry->key == NULL) return false;
  
  *value = entry->value;
  return true;
}

static void resize(Table* table, int capacity) {
  // Create the new empty entry array.
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  // Re-hash the existing entries into the new array.
  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

    Entry* dest = findEntry(entries, capacity, entry->key, false);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  // Replace the array.
  FREE_ARRAY(Value, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool tableSet(Table* table, ObjString* key, Value value) {
  // If the table is getting too full, make room first.
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    // Figure out the new table size.
    int capacity = GROW_CAPACITY(table->capacity);
    resize(table, capacity);
  }

  Entry* entry = findEntry(table->entries, table->capacity, key, false);
  bool isNewKey = entry->key == NULL;
  entry->key = key;
  entry->value = value;

  if (isNewKey) table->count++;
  return isNewKey;
}

// TODO: This isn't actually used by anything. It's here just to show a more
// complete hash table implementation. Can we use it?
bool tableDelete(Table* table, ObjString* key) {
  Entry* entry = findEntry(table->entries, table->capacity, key, false);
  if (entry->key == NULL) return false;
  
  // Leave a tombstone.
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
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
  uint32_t index = hash % table->capacity;
  
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
    index = (index + 1) % table->capacity;
  }
  
  return NULL;
}
//>= Uhh

void tableRemoveWhite(Table* table) {
  for (int i = 0; i < table->capacity; i++) {
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
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    grayObject((Obj*)entry->key);
    grayValue(entry->value);
  }
}
