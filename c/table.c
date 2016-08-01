//>= Hash Tables
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
  FREE_ARRAY(Value, table->entries, table->capacity);
  initTable(table);
}

static Entry* findEntry(Table* table, ObjString* key) {
  // If the table is empty, we definitely won't find it.
  if (table->count == 0) return NULL;
  
  // Figure out where to insert it in the table. Use open addressing and
  // basic linear probing.
  uint32_t index = key->hash % table->capacity;

  // We don't worry about an infinite loop here because resize() ensures
  // there are empty slots in the table.
  for (;;) {
    Entry* entry = &table->entries[index];
    
    if (entry->key == NULL) {
      // If the value is nil, it's an empty entry so we know the key isn't
      // present. If it's non-nil, the entry is a tombstone and we have to
      // keep looking.
      if (IS_NIL(entry->value)) return NULL;
    } else if (key == entry->key) {
      // We found it.
      return entry;
    }
    
    // Try the next slot.
    index = (index + 1) % table->capacity;
  }
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  Entry* entry = findEntry(table, key);
  if (entry == NULL) return false;
  
  *value = entry->value;
  return true;
}

// Inserts [key] and [value] in the array of [entries] with the given
// [capacity].
//
// Returns `true` if this is the first time [key] was added to the map.
static bool addEntry(Entry* entries, int capacity,
                     ObjString* key, Value value) {
  // Figure out where to insert it in the table. Use open addressing and
  // basic linear probing.
  uint32_t index = key->hash % capacity;

  // We don't worry about an infinite loop here because resize() ensures
  // there are open slots in the array.
  // TODO: Probably need to check for infinite loop here if table is all
  // tombstones like we do below.
  for (;;) {
    Entry* entry = &entries[index];

    // If we found an open slot, the key is not in the table.
    if (entry->key == NULL) {
      entry->key = key;
      entry->value = value;
      return true;
    } else if (key == entry->key) {
      // If the key already exists, just replace the value.
      entry->value = value;
      return false;
    }

    // Try the next slot.
    index = (index + 1) % capacity;
  }
}

static void resize(Table* table, int capacity) {
  // Create the new empty entry array.
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  // Re-hash the existing entries into the new array.
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

    addEntry(entries, capacity, entry->key, entry->value);
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

  bool isNewKey = addEntry(table->entries, table->capacity, key, value);
  if (isNewKey) table->count++;
  return isNewKey;
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
  if (table->count == 0) return NULL;
  
  // Figure out where to insert it in the table. Use open addressing and
  // basic linear probing.
  uint32_t index = hash % table->capacity;
  
  // We have to check for a loop here because the table could be full of
  // tombstones.
  uint32_t startIndex = index;
  do {
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
  } while (index != startIndex);
  
  return NULL;
}

void tableRemoveWhite(Table* table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->object.isDark) {
      // Turn the entry into a tombstone, identified as having a NULL key but a
      // non-nil value (true).
      entry->value = BOOL_VAL(true);
      entry->key = NULL;
      table->count--;
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
