//>= Chunks of Bytecode
#include <stdlib.h>

#include "common.h"
//>= Garbage Collection
#include "compiler.h"
//>= Chunks of Bytecode
#include "memory.h"
//>= Uhh
#include "object.h"
//>= Strings
#include "vm.h"

#ifdef DEBUG_TRACE_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2
//>= Chunks of Bytecode

void* reallocate(void* previous, size_t oldSize, size_t newSize) {
//>= Garbage Collection
  vm.bytesAllocated += newSize - oldSize;
  
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif
    
    if (vm.bytesAllocated > vm.nextGC) {
      collectGarbage();
    }
  }
  
//>= Chunks of Bytecode
  return realloc(previous, newSize);
}
//>= Garbage Collection

void grayObject(Obj* object) {
  if (object == NULL) return;
  
  // Don't get caught in cycle.
  if (object->isDark) return;
  
#ifdef DEBUG_TRACE_GC
  printf("%p gray ", object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  
  object->isDark = true;
  
  if (vm.grayCapacity < vm.grayCount + 1) {
    vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);

    // Not using reallocate() here because we don't want to trigger the GC
    // inside a GC!
    vm.grayStack = realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
  }
  
  vm.grayStack[vm.grayCount++] = object;
}

void grayValue(Value value) {
  if (!IS_OBJ(value)) return;
  grayObject(AS_OBJ(value));
}

static void grayArray(ValueArray* array) {
  for (int i = 0; i < array->count; i++) {
    grayValue(array->values[i]);
  }
}

static void blackenObject(Obj* object) {
#ifdef DEBUG_TRACE_GC
  printf("%p blacken ", object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  
  switch (object->type) {
//>= Methods and Initializers
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      grayValue(bound->receiver);
      grayObject((Obj*)bound->method);
      break;
    }
//>= Classes and Instances
      
    case OBJ_CLASS: {
      ObjClass* klass = (ObjClass*)object;
      grayObject((Obj*)klass->name);
//>= Inheritance
      grayObject((Obj*)klass->superclass);
//>= Methods and Initializers
      grayTable(&klass->methods);
//>= Classes and Instances
      break;
    }
    
//>= Garbage Collection
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      grayObject((Obj*)closure->function);
      for (int i = 0; i < closure->upvalueCount; i++) {
        grayObject((Obj*)closure->upvalues[i]);
      }
      break;
    }
      
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      grayObject((Obj*)function->name);
      grayArray(&function->chunk.constants);
      break;
    }
      
//>= Classes and Instances
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      grayObject((Obj*)instance->klass);
      grayTable(&instance->fields);
      break;
    }
      
//>= Garbage Collection
    case OBJ_UPVALUE:
      grayValue(((ObjUpvalue*)object)->closed);
      break;
      
//>= Uhh
    case OBJ_NATIVE:
//>= Garbage Collection
    case OBJ_STRING:
      // No references.
      break;
  }
}
//>= Strings

static void freeObject(Obj* object) {
//>= Garbage Collection
#ifdef DEBUG_TRACE_GC
  printf("%p free ", object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  
//>= Strings
  switch (object->type) {
//>= Methods and Initializers
    case OBJ_BOUND_METHOD:
      FREE(ObjBoundMethod, object);
      break;
      
/*== Classes and Instances
    case OBJ_CLASS:
*/
//>= Methods and Initializers
    case OBJ_CLASS: {
      ObjClass* klass = (ObjClass*)object;
      freeTable(&klass->methods);
//>= Classes and Instances
      FREE(ObjClass, object);
      break;
//>= Methods and Initializers
    }
//>= Classes and Instances
      
//>= Closures
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      FREE_ARRAY(Value, closure->upvalues, closure->upvalueCount);
      FREE(ObjClosure, object);
      break;
    }
      
//>= Functions
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      freeChunk(&function->chunk);
      FREE(ObjFunction, object);
      break;
    }
      
//>= Classes and Instances
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      freeTable(&instance->fields);
      FREE(ObjInstance, object);
      break;
    }
//>= Uhh
      
    case OBJ_NATIVE:
      FREE(ObjNative, object);
      break;
      
//>= Strings
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }
      
//>= Closures
    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;
//>= Strings
  }
}
//>= Garbage Collection

void collectGarbage() {
#ifdef DEBUG_TRACE_GC
  printf("-- gc begin\n");
  size_t before = vm.bytesAllocated;
#endif
  
  // Mark the stack roots.
  for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
    grayValue(*slot);
  }
  
  for (int i = 0; i < vm.frameCount; i++) {
    grayObject((Obj*)vm.frames[i].closure);
  }
  
  // Mark the open upvalues.
  for (ObjUpvalue* upvalue = vm.openUpvalues;
       upvalue != NULL;
       upvalue = upvalue->next) {
    grayObject((Obj*)upvalue);
  }
  
  // Mark the global roots.
  grayTable(&vm.globals);
  grayCompilerRoots();
//>= Methods and Initializers
  grayObject((Obj*)vm.initString);
//>= Garbage Collection
  
  // Traverse the references.
  while (vm.grayCount > 0) {
    // Pop an item from the gray stack.
    Obj* object = vm.grayStack[--vm.grayCount];
    blackenObject(object);
  }
  
  // Delete unused interned strings.
  tableRemoveWhite(&vm.strings);
  
  // Collect the white objects.
  Obj** object = &vm.objects;
  while (*object != NULL) {
    if (!((*object)->isDark)) {
      // This object wasn't reached, so remove it from the list and free it.
      Obj* unreached = *object;
      *object = unreached->next;
      freeObject(unreached);
    } else {
      // This object was reached, so unmark it (for the next GC) and move on to
      // the next.
      (*object)->isDark = false;
      object = &(*object)->next;
    }
  }
  
  // Adjust the heap size based on live memory.
  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_TRACE_GC
  printf("-- gc collected %ld bytes (from %ld to %ld) next at %ld\n",
         before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
#endif
}
//>= Strings

void freeObjects() {
  // Free all objects.
  Obj* object = vm.objects;
  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }
//>= Garbage Collection
  
  free(vm.grayStack);
//>= Strings
}