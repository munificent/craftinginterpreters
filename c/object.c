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

#define TABLE_MAX_LOAD 0.75

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
  function->codeLines = NULL;
  
  initArray(&function->constants);
  return function;
}

ObjNative* newNative(NativeFn function) {
  ObjNative* native = ALLOCATE(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
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
  table->capacity = 0;
  table->count = 0;
  table->entries = NULL;
  return table;
}

void ensureTableCapacity(ObjTable* table) {
  if (table->capacity * TABLE_MAX_LOAD > table->count) return;
  
  if (table->capacity == 0) {
    table->capacity = 4;
  } else {
    table->capacity *= 2;
  }
  
  // TODO: Rehash everything.
  table->entries = realloc(table->entries, sizeof(TableEntry) * table->capacity);
}

bool tableGet(ObjTable* table, ObjString* key, Value* value) {
  // TODO: Actually hash it!
  for (int i = 0; i < table->count; i++) {
    TableEntry* entry = &table->entries[i];
    if (entry->key->length == key->length &&
        memcmp(entry->key->chars, key->chars, key->length) == 0) {
      *value = table->entries[i].value;
      return true;
    }
  }
  
  return false;
}

bool tableSet(ObjTable* table, ObjString* key, Value value) {
  // TODO: Actually hash it!
  for (int i = 0; i < table->count; i++) {
    TableEntry* entry = &table->entries[i];
    if (entry->key->length == key->length &&
        memcmp(entry->key->chars, key->chars, key->length) == 0) {
      table->entries[i].value = value;
      return true;
    }
  }
  
  ensureTableCapacity(table);
  TableEntry* entry = &table->entries[table->count++];
  entry->key = key;
  entry->value = value;
  return false;
}

bool valuesEqual(Value a, Value b) {
  // Identity.
  if (a == b) return true;
  
  if (IS_NULL(a) || IS_NULL(b)) return false;
  
  // No implicit conversions.
  if (a->type != b->type) return false;
  
  switch (a->type) {
    case OBJ_BOOL:
      // TODO: Canonicalize bools?
      return ((ObjBool*)a)->value == ((ObjBool*)b)->value;
      
    case OBJ_NUMBER:
      return ((ObjNumber*)a)->value == ((ObjNumber*)b)->value;
      
    case OBJ_STRING: {
      ObjString* aString = (ObjString*)a;
      ObjString* bString = (ObjString*)b;
      return aString->length == bString->length &&
             memcmp(aString->chars, bString->chars, aString->length) == 0;
    }
      
    case OBJ_FUNCTION:
    case OBJ_NATIVE:
    case OBJ_TABLE:
      // These have reference equality.
      return false;
  }
}

void initArray(ValueArray* array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void ensureArrayCapacity(ValueArray* array) {
  if (array->capacity > array->count) return;
  
  if (array->capacity == 0) {
    array->capacity = 4;
  } else {
    array->capacity *= 2;
  }
  
  array->values = realloc(array->values, sizeof(Value) * array->capacity);
}

void freeArray(ValueArray* array) {
  free(array->values);
}

void grayValue(Value value) {
  if (IS_NULL(value)) return;
  
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
  }

  vm.grayStack[vm.grayCount++] = value;
}

static void grayArray(ValueArray* array) {
  for (int i = 0; i < array->count; i++) {
    grayValue(array->values[i]);
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
      grayArray(&function->constants);
      break;
    }
      
    case OBJ_TABLE: {
      ObjTable* table = (ObjTable*)obj;
      for (int i = 0; i < table->count; i++) {
        TableEntry* entry = &table->entries[i];
        grayValue((Value)entry->key);
        grayValue(entry->value);
      }
      break;
    }
      
    case OBJ_BOOL:
    case OBJ_NATIVE:
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
      freeArray(&function->constants);
      break;
    }
      
    case OBJ_TABLE: {
      ObjTable* table = (ObjTable*)obj;
      free(table->entries);
      break;
    }
      
    case OBJ_BOOL:
    case OBJ_NATIVE:
    case OBJ_NUMBER:
    case OBJ_STRING:
      // No references.
      break;
  }
  
  free(obj);
}

// TODO: Move to vm.c?
void collectGarbage() {
#ifdef DEBUG_TRACE_GC
  printf("-- gc --\n");
#endif
  
  // Mark the stack roots.
  for (CallFrame* frame = vm.frame; frame != NULL; frame = frame->caller) {
    grayValue((Value)frame->function);
    for (int i = 0; i < frame->stackSize; i++) {
      grayValue(frame->stack[i]);
    }
  }
  
  // Mark the global roots.
  grayValue((Value)vm.globals);
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
