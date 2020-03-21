//> Hash Tables table-c
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

//> max-load
#define TABLE_MAX_LOAD 0.75

//< max-load
void initTable(Table* table) {
  table->count = 0;
/* Hash Tables table-c < Optimization init-capacity-mask
  table->capacity = 0;
*/
//> Optimization init-capacity-mask
  table->capacityMask = -1;
//< Optimization init-capacity-mask
  table->entries = NULL;
}
//> free-table
void freeTable(Table* table) {
/* Hash Tables free-table < Optimization free-table
  FREE_ARRAY(Entry, table->entries, table->capacity);
*/
//> Optimization free-table
  FREE_ARRAY(Entry, table->entries, table->capacityMask + 1);
//< Optimization free-table
  initTable(table);
}
//< free-table
//> find-entry
/* Hash Tables find-entry < Optimization find-entry
static Entry* findEntry(Entry* entries, int capacity,
                        ObjString* key) {
*/
//> Optimization find-entry
static Entry* findEntry(Entry* entries, int capacityMask,
                        ObjString* key) {
//< Optimization find-entry
/* Hash Tables find-entry < Optimization initial-index
  uint32_t index = key->hash % capacity;
*/
//> Optimization initial-index
  uint32_t index = key->hash & capacityMask;
//< Optimization initial-index
//> find-entry-tombstone
  Entry* tombstone = NULL;
  
//< find-entry-tombstone
  for (;;) {
    Entry* entry = &entries[index];

/* Hash Tables find-entry < Hash Tables find-tombstone
    if (entry->key == key || entry->key == NULL) {
      return entry;
    }
*/
//> find-tombstone
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // Empty entry.
        return tombstone != NULL ? tombstone : entry;
      } else {
        // We found a tombstone.
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      // We found the key.
      return entry;
    }
//< find-tombstone

/* Hash Tables find-entry < Optimization next-index
    index = (index + 1) % capacity;
*/
//> Optimization next-index
    index = (index + 1) & capacityMask;
//< Optimization next-index
  }
}
//< find-entry
//> table-get
bool tableGet(Table* table, ObjString* key, Value* value) {
  if (table->count == 0) return false;

/* Hash Tables table-get < Optimization get-find-entry
  Entry* entry = findEntry(table->entries, table->capacity, key);
*/
//> Optimization get-find-entry
  Entry* entry = findEntry(table->entries, table->capacityMask, key);
//< Optimization get-find-entry
  if (entry->key == NULL) return false;

  *value = entry->value;
  return true;
}
//< table-get
//> table-adjust-capacity
/* Hash Tables table-adjust-capacity < Optimization adjust-capacity
static void adjustCapacity(Table* table, int capacity) {
*/
//> Optimization adjust-capacity
static void adjustCapacity(Table* table, int capacityMask) {
//< Optimization adjust-capacity
/* Hash Tables table-adjust-capacity < Optimization adjust-alloc
  Entry* entries = ALLOCATE(Entry, capacity);
*/
//> Optimization adjust-alloc
Entry* entries = ALLOCATE(Entry, capacityMask + 1);
//< Optimization adjust-alloc
/* Hash Tables table-adjust-capacity < Optimization adjust-init
  for (int i = 0; i < capacity; i++) {
*/
//> Optimization adjust-init
  for (int i = 0; i <= capacityMask; i++) {
//< Optimization adjust-init
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }
  
//> re-hash
//> resize-init-count
  table->count = 0;
//< resize-init-count
/* Hash Tables re-hash < Optimization re-hash
  for (int i = 0; i < table->capacity; i++) {
*/
//> Optimization re-hash
  for (int i = 0; i <= table->capacityMask; i++) {
//< Optimization re-hash
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

/* Hash Tables re-hash < Optimization adjust-find-entry
    Entry* dest = findEntry(entries, capacity, entry->key);
*/
//> Optimization adjust-find-entry
    Entry* dest = findEntry(entries, capacityMask, entry->key);
//< Optimization adjust-find-entry
    dest->key = entry->key;
    dest->value = entry->value;
//> resize-increment-count
    table->count++;
//< resize-increment-count
  }
//< re-hash

/* Hash Tables free-old-array < Optimization adjust-free
  FREE_ARRAY(Entry, table->entries, table->capacity);
*/
//> Optimization adjust-free
  FREE_ARRAY(Entry, table->entries, table->capacityMask + 1);
//< Optimization adjust-free
  table->entries = entries;
/* Hash Tables table-adjust-capacity < Optimization adjust-set-capacity
  table->capacity = capacity;
*/
//> Optimization adjust-set-capacity
  table->capacityMask = capacityMask;
//< Optimization adjust-set-capacity
}
//< table-adjust-capacity
//> table-set
bool tableSet(Table* table, ObjString* key, Value value) {
/* Hash Tables table-set-grow < Optimization table-set-grow
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
*/
//> Optimization table-set-grow
  if (table->count + 1 > (table->capacityMask + 1) * TABLE_MAX_LOAD) {
    int capacityMask = GROW_CAPACITY(table->capacityMask + 1) - 1;
    adjustCapacity(table, capacityMask);
//< Optimization table-set-grow
//> table-set-grow
  }

//< table-set-grow
/* Hash Tables table-set < Optimization set-find-entry
  Entry* entry = findEntry(table->entries, table->capacity, key);
*/
//> Optimization set-find-entry
  Entry* entry = findEntry(table->entries, table->capacityMask, key);
//< Optimization set-find-entry
  
  bool isNewKey = entry->key == NULL;
/* Hash Tables table-set < Hash Tables set-increment-count
  if (isNewKey) table->count++;
*/
//> set-increment-count
  if (isNewKey && IS_NIL(entry->value)) table->count++;
//< set-increment-count

  entry->key = key;
  entry->value = value;
  return isNewKey;
}
//< table-set
//> table-delete
bool tableDelete(Table* table, ObjString* key) {
  if (table->count == 0) return false;

  // Find the entry.
/* Hash Tables table-delete < Optimization delete-find-entry
  Entry* entry = findEntry(table->entries, table->capacity, key);
*/
//> Optimization delete-find-entry
  Entry* entry = findEntry(table->entries, table->capacityMask, key);
//< Optimization delete-find-entry
  if (entry->key == NULL) return false;

  // Place a tombstone in the entry.
  entry->key = NULL;
  entry->value = BOOL_VAL(true);

  return true;
}
//< table-delete
//> table-add-all
void tableAddAll(Table* from, Table* to) {
/* Hash Tables table-add-all < Optimization add-all-loop
  for (int i = 0; i < from->capacity; i++) {
*/
//> Optimization add-all-loop
  for (int i = 0; i <= from->capacityMask; i++) {
//< Optimization add-all-loop
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}
//< table-add-all
//> table-find-string
ObjString* tableFindString(Table* table, const char* chars, int length,
                           uint32_t hash) {
  if (table->count == 0) return NULL;

/* Hash Tables table-find-string < Optimization find-string-index
  uint32_t index = hash % table->capacity;
*/
//> Optimization find-string-index
  uint32_t index = hash & table->capacityMask;
//< Optimization find-string-index

  for (;;) {
    Entry* entry = &table->entries[index];

    if (entry->key == NULL) {
      // Stop if we find an empty non-tombstone entry.
      if (IS_NIL(entry->value)) return NULL;
    } else if (entry->key->length == length &&
        entry->key->hash == hash &&
        memcmp(entry->key->chars, chars, length) == 0) {
      // We found it.
      return entry->key;
    }

/* Hash Tables table-find-string < Optimization find-string-next
    index = (index + 1) % table->capacity;
*/
//> Optimization find-string-next
    index = (index + 1) & table->capacityMask;
//< Optimization find-string-next
  }
}
//< table-find-string
//> Garbage Collection table-remove-white
void tableRemoveWhite(Table* table) {
/* Garbage Collection table-remove-white < Optimization remove-white
  for (int i = 0; i < table->capacity; i++) {
*/
//> Optimization remove-white
  for (int i = 0; i <= table->capacityMask; i++) {
//< Optimization remove-white
    Entry* entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->obj.isMarked) {
      tableDelete(table, entry->key);
    }
  }
}
//< Garbage Collection table-remove-white
//> Garbage Collection mark-table
void markTable(Table* table) {
/* Garbage Collection mark-table < Optimization mark-table
  for (int i = 0; i < table->capacity; i++) {
*/
//> Optimization mark-table
  for (int i = 0; i <= table->capacityMask; i++) {
//< Optimization mark-table
    Entry* entry = &table->entries[i];
    markObject((Obj*)entry->key);
    markValue(entry->value);
  }
}
//< Garbage Collection mark-table
