#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "object.h"
#include "vm.h"

#include "debug.h"

//#define DEBUG_STRESS_GC

#define ALLOCATE(type) (type*)allocate(sizeof(type))

#define ALLOCATE_FLEX(type, flexType, count) \
    (type*)allocate(sizeof(type) + sizeof(flexType) * (count))

static void* allocate(size_t size) {
  if (vm.fromEnd + size > vm.fromStart + MAX_HEAP) {
    collectGarbage();
    
    if (vm.fromEnd + size > vm.fromStart + MAX_HEAP) {
      fprintf(stderr, "Heap full. Need %ld bytes, but only %ld available.\n",
              size, MAX_HEAP - (vm.fromEnd - vm.fromStart));
      exit(1);
    }
  } else {
    #ifdef DEBUG_STRESS_GC
    collectGarbage();
    #endif
  }
  
//  printf("allocate %ld at %p\n", size, vm.fromEnd);
  
  void* result = vm.fromEnd;
  vm.fromEnd += size;
  return result;
}

ObjArray* newArray(int size) {
  ObjArray* array = ALLOCATE_FLEX(ObjArray, Value, size);
  array->obj.type = OBJ_ARRAY;
  
  array->size = size;
  for (int i = 0; i < size; i++) {
    array->elements[i] = NULL;
  }
  return array;
}

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE(ObjFunction);
  function->obj.type = OBJ_FUNCTION;
  
  function->codeSize = 0;
  function->code = NULL;
  function->numConstants = 0;
  function->constants = NULL;
  return function;
}

ObjNumber* newNumber(double value) {
  ObjNumber* number = ALLOCATE(ObjNumber);
  number->obj.type = OBJ_NUMBER;
  
  number->value = value;
  return number;
}

ObjString* newString(const uint8_t* chars, int length) {
  ObjString* string = newByteString(length + 1);
  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';
  return string;
}

ObjString* newByteString(int length) {
  ObjString* string = ALLOCATE_FLEX(ObjString, char, length);
  string->obj.type = OBJ_STRING;
  string->length = length;
  return string;
}

ObjTable* newTable() {
  ObjTable* table = ALLOCATE(ObjTable);
  table->obj.type = OBJ_TABLE;
  table->count = 0;
  table->entries = NULL;
  return table;
}

// From: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
static int powerOf2Ceil(int n)
{
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  
  return n;
}

ObjArray* ensureArraySize(ObjArray* array, int size) {
  int currentSize = array == NULL ? 0 : array->size;
  if (currentSize >= size) return array;
  
  size = powerOf2Ceil(size);
  ObjArray* array2 = newArray(size);
  
  // Copy the previous values over.
  if (array != NULL) {
    for (int i = 0; i < array->size; i++) {
      array2->elements[i] = array->elements[i];
    }
  }
  
  return array2;
}

// TODO: Lot of copy paste with above.
ObjString* ensureStringLength(ObjString* string, int length) {
  int currentLength = string == NULL ? 0 : string->length;
  if (currentLength >= length) return string;
  
  length = powerOf2Ceil(length);
  ObjString* string2 = newByteString(length);
  
  // Copy the previous values over.
  if (string != NULL) {
    memcpy(string2->chars, string->chars, string->length);
  }
  
  return string2;
}

static size_t objectSize(Obj* obj) {
  switch (obj->type) {
    case OBJ_ARRAY:
      return sizeof(ObjArray) +
          ((ObjArray*)obj)->size * sizeof(Value);
    case OBJ_FORWARD:
      // Shouldn't have forwarding pointers in the to space!
      assert(false);
      return 0;
    case OBJ_FUNCTION:
      return sizeof(ObjFunction);
    case OBJ_NUMBER:
      return sizeof(ObjNumber);
    case OBJ_STRING:
      return sizeof(ObjString) + ((ObjString*)obj)->length;
    case OBJ_TABLE:
      return sizeof(ObjTable);
    case OBJ_TABLE_ENTRIES:
      return sizeof(ObjTableEntries) +
          ((ObjTableEntries*)obj)->size * sizeof(TableEntry);
  }
  
  assert(false); // Unreachable.
}

// TODO: Instead of returning new value, have it take pointer to one and update
// directly?
Value moveObject(Value value) {
  if (value == NULL) return NULL;
  
  // If it's already been copied, return its new location.
  if (value->type == OBJ_FORWARD) return ((ObjForward*)value)->to;
  
//  printf("copy ");
//  printValue(value);
//  printf(" (%ld bytes) from %p to %p\n", objectSize(value), value, vm.toEnd);

  // Move it to the new semispace.
  size_t size = objectSize(value);
  memcpy(vm.toEnd, value, size);
  
  // And turn the original one into a forwarding pointer.
  ObjForward* old = (ObjForward*)value;
  old->obj.type = OBJ_FORWARD;
  old->to = (Obj*)vm.toEnd;
  
  Value newValue = (Value)vm.toEnd;
  vm.toEnd += size;
  
  return newValue;
}

static void traverseObject(Obj* obj) {
  switch (obj->type) {
    case OBJ_ARRAY: {
      ObjArray* array = (ObjArray*)obj;
      for (int i = 0; i < array->size; i++) {
        array->elements[i] = moveObject(array->elements[i]);
      }
      break;
    }

    case OBJ_FORWARD:
      // Shouldn't have forwarding pointers in the to space!
      assert(false);
      break;
      
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)obj;
      function->code = (ObjString*)moveObject((Obj*)function->code);
      function->constants = (ObjArray*)moveObject((Obj*)function->constants);
      break;
    }
      
    case OBJ_NUMBER:
    case OBJ_STRING:
      // No references.
      break;
      
    case OBJ_TABLE: {
      ObjTable* table = (ObjTable*)obj;
      table->entries = (ObjTableEntries*)moveObject((Obj*)table->entries);
      break;
    }
      
    case OBJ_TABLE_ENTRIES: {
      ObjTableEntries* entries = (ObjTableEntries*)obj;
      for (int i = 0; i < entries->size; i++) {
        TableEntry* entry = &entries->entries[i];
        entry->key = moveObject(entry->key);
        entry->value = moveObject(entry->value);
      }
      break;
    }
  }
}

void collectGarbage() {
  // Copy the roots over.
  for (int i = 0; i < vm.stackSize; i++) {
    vm.stack[i] = moveObject(vm.stack[i]);
  }
  
  traceCompilerRoots();
  
  // Traverse everything referenced by the roots.
  char* obj = vm.toStart;
  while (obj < vm.toEnd) {
    traverseObject((Obj*)obj);
    obj += objectSize((Obj*)obj);
  }
  
  // TODO: Temp for debugging.
  memset(vm.fromStart, 0xcc, MAX_HEAP);

  char* temp = vm.fromStart;
  vm.fromStart = vm.toStart;
  vm.fromEnd = vm.toEnd;
  vm.toStart = temp;
  vm.toEnd = temp;
}
