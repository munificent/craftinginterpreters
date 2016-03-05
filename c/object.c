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
//#define DEBUG_TRACE_GC

#define ALLOCATE(type, objType) (type*)allocateObj(sizeof(type), objType)

void* reallocate(void* previous, size_t size) {
#ifdef DEBUG_STRESS_GC
  collectGarbage();
#endif
  
  // TODO: Collect after certain amount.
  
  return realloc(previous, size);
}

Obj* allocateObj(size_t size, ObjType type) {
  Obj* obj = (Obj*)reallocate(NULL, size);
  obj->type = type;
  obj->isDark = false;
  
  obj->next = vm.objects;
  vm.objects = obj;

#ifdef DEBUG_TRACE_GC
  printf("%p allocate %ld for %d\n", obj, size, type);
#endif
  
  return obj;
}

ObjBool* newBool(bool value) {
  ObjBool* boolean = ALLOCATE(ObjBool, OBJ_BOOL);
  boolean->value = value;
  return boolean;
}

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE(ObjFunction, OBJ_FUNCTION);
  
  function->codeCount = 0;
  function->codeCapacity = 0;
  function->code = NULL;
  function->constantCount = 0;
  function->constantCapacity = 0;
  function->constants = NULL;
  return function;
}

ObjNumber* newNumber(double value) {
  ObjNumber* number = ALLOCATE(ObjNumber, OBJ_NUMBER);
  number->value = value;
  return number;
}

ObjString* newString(const uint8_t* chars, int length) {
  // Copy the string to the heap so the object can own it.
  char* stringChars = reallocate(NULL, length + 1);
  stringChars[length] = '\0';
  
  if (chars != NULL) {
    memcpy(stringChars, chars, length);
  }
  
  ObjString* string = ALLOCATE(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = stringChars;
  return string;
}

ObjTable* newTable() {
  ObjTable* table = ALLOCATE(ObjTable, OBJ_TABLE);
  table->count = 0;
  table->entries = NULL;
  return table;
}

void grayValue(Value value) {
  if (value == NULL) return;
  
  // Don't get caught in cycle.
  if (value->isDark) return;
  
#ifdef DEBUG_TRACE_GC
  printf("%p gray ", value);
  printValue(value);
  printf("\n");
#endif
  
  value->isDark = true;
  
  if (vm.grayCapacity < vm.grayCount + 1) {
    vm.grayCapacity = vm.grayCapacity == 0 ? 4 : vm.grayCapacity * 2;
    vm.grayStack = realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
    
    vm.grayStack[vm.grayCount++] = value;
  }
}

static void blackenObject(Obj* obj) {
#ifdef DEBUG_TRACE_GC
  printf("%p blacken ", obj);
  printValue((Value)obj);
  printf("\n");
#endif

  switch (obj->type) {
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)obj;
      for (int i = 0; i < function->constantCount; i++) {
        grayValue(function->constants[i]);
      }
      break;
    }
      
    case OBJ_TABLE: {
      ObjTable* table = (ObjTable*)obj;
      for (int i = 0; i < table->capacity; i++) {
        TableEntry* entry = &table->entries[i];
        grayValue(entry->key);
        grayValue(entry->value);
      }
      break;
    }
      
    case OBJ_BOOL:
    case OBJ_NUMBER:
    case OBJ_STRING:
      // No references.
      break;
  }
}

void freeObject(Obj* obj) {
#ifdef DEBUG_TRACE_GC
  printf("%p free ", obj);
  printValue((Value)obj);
  printf("\n");
#endif

  switch (obj->type) {
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)obj;
      free(function->code);
      free(function->constants);
      break;
    }
      
    case OBJ_TABLE: {
      ObjTable* table = (ObjTable*)obj;
      free(table->entries);
      break;
    }
      
    case OBJ_BOOL:
    case OBJ_NUMBER:
    case OBJ_STRING:
      // No references.
      break;
  }
}

void collectGarbage() {
#ifdef DEBUG_TRACE_GC
  printf("-- gc --\n");
#endif
  
  // Mark the roots.
  for (int i = 0; i < vm.stackSize; i++) {
    grayValue(vm.stack[i]);
  }
  
  grayCompilerRoots();

  while (vm.grayCount > 0) {
    // Pop an item from the gray stack.
    Obj* obj = vm.grayStack[--vm.grayCount];
    blackenObject(obj);
  }
  
  // Collect the white objects.
  Obj** obj = &vm.objects;
  while (*obj != NULL) {
    if (!((*obj)->isDark)) {
      // This object wasn't reached, so remove it from the list and free it.
      Obj* unreached = *obj;
      *obj = unreached->next;
      freeObject(unreached);
    } else {
      // This object was reached, so unmark it (for the next GC) and move on to
      // the next.
      (*obj)->isDark = false;
      obj = &(*obj)->next;
    }
  }
}
