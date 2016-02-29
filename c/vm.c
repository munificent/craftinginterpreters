#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

static void printValue(Value value) {
  switch (value->type) {
    case OBJ_FORWARD:
      printf("fwd->%p", ((ObjForward*)value)->to);
      break;
      
    case OBJ_NUMBER:
      printf("%g", ((ObjNumber*)value)->value);
      break;
      
    case OBJ_PAIR: {
      ObjPair* pair = (ObjPair*)value;
      printf("(");
      printValue(pair->a);
      printf(", ");
      printValue(pair->b);
      printf(")");
      break;
    }
  }
}

void printStack(VM* vm) {
  for (int i = 0; i < vm->stackSize; i++) {
    printf("%d: ", i);
    printValue(vm->stack[i]);
    printf("\n");
  }
}

void initVM(VM* vm) {
  vm->stackSize = 0;
  
  vm->fromStart = malloc(MAX_HEAP);
  vm->fromEnd = vm->fromStart;
  vm->toStart = malloc(MAX_HEAP);
  vm->toEnd = vm->toStart;
}

static size_t objectSize(Obj* obj) {
  switch (obj->type) {
    case OBJ_FORWARD: return sizeof(ObjForward);
    case OBJ_NUMBER:  return sizeof(ObjNumber);
    case OBJ_PAIR:    return sizeof(ObjPair);
  }
  
  assert(false); // Unreachable.
}

static Value copy(VM* vm, Value value) {
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

static void copyReferences(VM* vm, Obj* obj) {
  switch (obj->type) {
    case OBJ_FORWARD:
      // Shouldn't have forwarding pointers in the to space!
      assert(false);
    case OBJ_NUMBER:
      // No references.
      break;
      
    case OBJ_PAIR: {
      ObjPair* pair = (ObjPair*)obj;
      pair->a = copy(vm, pair->a);
      pair->b = copy(vm, pair->b);
    }
  }
}

void collectGarbage(VM* vm) {
  // Copy the roots over.
  for (int i = 0; i < vm->stackSize; i++) {
    vm->stack[i] = copy(vm, vm->stack[i]);
  }
  
  // Traverse everything referenced by the roots.
  Obj* obj = (Obj*)vm->toStart;
  while ((char*)obj < vm->toEnd) {
    copyReferences(vm, obj);
    obj += objectSize(obj);
  }
  
  char* temp = vm->fromStart;
  vm->fromStart = vm->toStart;
  vm->fromEnd = vm->toEnd;
  vm->toStart = temp;
  vm->toEnd = temp;
}

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

// TODO: Temp.
void pushNumber(VM* vm, double value) {
  ObjNumber* obj = allocate(vm, sizeof(ObjNumber));
  obj->obj.type = OBJ_NUMBER;
  obj->value = value;
  
  vm->stack[vm->stackSize++] = (Value)obj;
}

// TODO: Temp.
void pushPair(VM* vm) {
  ObjPair* obj = allocate(vm, sizeof(ObjPair));
  obj->obj.type = OBJ_PAIR;
  
  obj->b = vm->stack[--vm->stackSize];
  obj->a = vm->stack[--vm->stackSize];
  
  vm->stack[vm->stackSize++] = (Value)obj;
}

void pop(VM* vm) {
  vm->stackSize--;
}
