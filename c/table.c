#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "table.h"

#define TABLE_MAX_LOAD  0.75
#define GROW_FACTOR     2
#define MIN_CAPACITY    8

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

static uint32_t hash(const uint8_t* key, int length) {
  // FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
  uint32_t hash = 2166136261u;
  
  // This is O(n) on the length of the string, but we only call this when a new
  // string is created. Since the creation is also O(n) (to copy/initialize all
  // the bytes), we allow this here.
  for (int i = 0; i < length; i++) {
    hash ^= key[i];
    hash *= 16777619;
  }
  
  return hash;
}

static bool keysMatch(ObjString* key, const uint8_t* chars, int length) {
  return key->length == length && memcmp(key->chars, chars, length) == 0;
}

static Entry* findEntry(Table* table, const uint8_t* key, int length) {
  // If the table is empty, we definitely won't find it.
  if (table->count == 0) return NULL;
  
  // Figure out where to insert it in the table. Use open addressing and
  // basic linear probing.
  uint32_t index = hash(key, length) % table->capacity;
  
  // We don't worry about an infinite loop here because resize() ensures
  // there are empty slots in the table.
  for (;;) {
    Entry* entry = &table->entries[index];
    
    if (entry->key == NULL) {
      // If the value is non-null, it's a tombstone and we have to keep looking.
      if (entry->value == NULL) return NULL;
    } else if (keysMatch(entry->key, key, length)) {
      // We found it.
      return entry;
    }
    
    // Try the next slot.
    index = (index + 1) % table->capacity;
  }
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  Entry* entry = findEntry(table, key->chars, key->length);
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
  uint32_t index = hash(key->chars, key->length) % capacity;

  // We don't worry about an infinite loop here because resizeTable() ensures
  // there are open slots in the array.
  for (;;) {
    Entry* entry = &entries[index];

    // If we found an open slot, the key is not in the table.
    if (entry->key == NULL) {
      entry->key = key;
      entry->value = value;
      return true;
    } else if (keysMatch(entry->key, key->chars, key->length)) {
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
  Entry* entries = REALLOCATE(NULL, Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NULL;
  }

  // Re-hash the existing entries into the new array.
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

    addEntry(entries, capacity, entry->key, entry->value);
  }

  // Replace the array.
  free(table->entries);
  table->entries = entries;
  table->capacity = capacity;
}

bool tableSet(Table* table, ObjString* key, Value value) {
  // If the table is getting too full, make room first.
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    // Figure out the new table size.
    int capacity = table->capacity * GROW_FACTOR;
    if (capacity < MIN_CAPACITY) capacity = MIN_CAPACITY;
    
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

ObjString* tableFindKey(Table* table, const uint8_t* chars, int length) {
  Entry* entry = findEntry(table, chars, length);
  if (entry == NULL) return NULL;
  
  return entry->key;
}

void tableRemoveWhite(Table* table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->obj.isDark) {
      // Turn the entry into a tombstone, identified as having no key but a
      // non-null value. We use the original key as the value since it's a
      // conveniently non-null value we have in hand.
      entry->value = (Value)entry->key;
      entry->key = NULL;
      table->count--;
    }
  }
}

void grayTable(Table* table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    grayValue((Value)entry->key);
    grayValue(entry->value);
  }
}
