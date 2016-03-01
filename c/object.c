#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "vm.h"

#include "debug.h"

static void* allocate(VM* vm, size_t size) {
  if (vm->fromEnd + size > vm->fromStart + MAX_HEAP) {
    collectGarbage(vm);
    
    if (vm->fromEnd + size > vm->fromStart + MAX_HEAP) {
      fprintf(stderr, "Heap full. Need %ld bytes, but only %ld available.\n",
              size, MAX_HEAP - (vm->fromEnd - vm->fromStart));
      exit(1);
    }
  }
  
  printf("allocate %ld at %p\n", size, vm->fromEnd);
  
  void* result = vm->fromEnd;
  vm->fromEnd += size;
  return result;
}

ObjArray* newArray(VM* vm, int size) {
  ObjArray* array = (ObjArray*)allocate(vm,
      sizeof(ObjArray) + size * sizeof(Value));
  array->obj.type = OBJ_ARRAY;
  
  array->size = size;
  for (int i = 0; i < size; i++) {
    array->elements[i] = NULL;
  }
  return array;
}

ObjFunction* newFunction(VM* vm, uint8_t* code, int codeSize,
                         ObjArray* constants) {
  ObjFunction* function = (ObjFunction*)allocate(vm,
      sizeof(ObjFunction) + codeSize);
  function->obj.type = OBJ_FUNCTION;
  
  memcpy(function->code, code, codeSize);
  function->codeSize = codeSize;
  function->constants = constants;
  return function;
}

ObjNumber* newNumber(VM* vm, double value) {
  ObjNumber* number = (ObjNumber*)allocate(vm, sizeof(ObjNumber));
  number->obj.type = OBJ_NUMBER;
  
  number->value = value;
  return number;
}

ObjString* newString(VM* vm, const char* chars, int length) {
  ObjString* string = (ObjString*)allocate(vm, sizeof(ObjString) + length + 1);
  string->obj.type = OBJ_STRING;
  
  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';
  return string;
}

ObjTable* newTable(VM* vm) {
  ObjTable* table = (ObjTable*)allocate(vm, sizeof(ObjTable));
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

ObjArray* ensureArraySize(VM* vm, ObjArray* array, int size) {
  int currentSize = array == NULL ? 0 : array->size;
  if (currentSize >= size) return array;
  
  size = powerOf2Ceil(size);
  ObjArray* array2 = newArray(vm, size);
  
  // Copy the previous values over.
  if (array != NULL) {
    for (int i = 0; i < array->size; i++) {
      array2->elements[i] = array->elements[i];
    }
  }
  
  return array2;
}

void collectGarbage(VM* vm) {
  // Copy the roots over.
  for (int i = 0; i < vm->stackSize; i++) {
    vm->stack[i] = moveObject(vm, vm->stack[i]);
  }
  
  // Traverse everything referenced by the roots.
  Obj* obj = (Obj*)vm->toStart;
  while ((char*)obj < vm->toEnd) {
    traverseObject(vm, obj);
    obj += objectSize(obj);
  }
  
  char* temp = vm->fromStart;
  vm->fromStart = vm->toStart;
  vm->fromEnd = vm->toEnd;
  vm->toStart = temp;
  vm->toEnd = temp;
}

Value moveObject(VM* vm, Value value) {
  if (value == NULL) return NULL;
  
  // If it's already been copied, return its new location.
  if (value->type == OBJ_FORWARD) return ((ObjForward*)value)->to;
  
  printf("copy ");
  printValue(value);
  printf(" from %p to %p\n", value, vm->toEnd);
  
  // Move it to the new semispace.
  size_t size = objectSize(value);
  memcpy(vm->toEnd, value, size);
  
  // And turn the original one into a forwarding pointer.
  ObjForward* old = (ObjForward*)value;
  old->obj.type = OBJ_FORWARD;
  old->to = (Obj*)vm->toEnd;
  
  Value newValue = (Value)vm->toEnd;
  vm->toEnd += size;
  
  return newValue;
}

size_t objectSize(Obj* obj) {
  switch (obj->type) {
    case OBJ_ARRAY:
      return sizeof(ObjArray) +
          ((ObjArray*)obj)->size * sizeof(Value);
    case OBJ_FORWARD:
      // Shouldn't have forwarding pointers in the to space!
      assert(false);
      return 0;
    case OBJ_FUNCTION:
      return sizeof(ObjFunction) +
          ((ObjFunction*)obj)->codeSize;
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

void traverseObject(VM* vm, Obj* obj) {
  switch (obj->type) {
    case OBJ_ARRAY: {
      ObjArray* array = (ObjArray*)obj;
      for (int i = 0; i < array->size; i++) {
        array->elements[i] = moveObject(vm, array->elements[i]);
      }
      break;
    }

    case OBJ_FORWARD:
      // Shouldn't have forwarding pointers in the to space!
      assert(false);
      break;
      
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)obj;
      function->constants = (ObjArray*)moveObject(vm, (Obj*)function->constants);
      break;
    }
      
    case OBJ_NUMBER:
    case OBJ_STRING:
      // No references.
      break;
      
    case OBJ_TABLE: {
      ObjTable* table = (ObjTable*)obj;
      table->entries = (ObjTableEntries*)moveObject(vm, (Obj*)table->entries);
      break;
    }
      
    case OBJ_TABLE_ENTRIES: {
      ObjTableEntries* entries = (ObjTableEntries*)obj;
      for (int i = 0; i < entries->size; i++) {
        TableEntry* entry = &entries->entries[i];
        entry->key = moveObject(vm, entry->key);
        entry->value = moveObject(vm, entry->value);
      }
      break;
    }
  }
}
