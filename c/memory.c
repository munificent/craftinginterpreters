//> Chunks of Bytecode memory-c
#include <stdlib.h>

#include "common.h"
//> Garbage Collection memory-include-compiler
#include "compiler.h"
//< Garbage Collection memory-include-compiler
#include "memory.h"
//> Strings memory-include-vm
#include "vm.h"
//< Strings memory-include-vm
//> Garbage Collection debug-log-includes

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif
//< Garbage Collection debug-log-includes
//> Garbage Collection heap-grow-factor

#define GC_HEAP_GROW_FACTOR 2
//< Garbage Collection heap-grow-factor

void* reallocate(void* previous, size_t oldSize, size_t newSize) {
//> Garbage Collection reallocate-track
  vm.bytesAllocated += newSize - oldSize;

  if (newSize > oldSize) {
//> stress-gc
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif

//< stress-gc
    if (vm.bytesAllocated > vm.nextGC) {
      collectGarbage();
    }
  }

//< Garbage Collection reallocate-track
  if (newSize == 0) {
    free(previous);
    return NULL;
  }
  
  return realloc(previous, newSize);
}
//> Garbage Collection gray-object
void grayObject(Obj* object) {
  if (object == NULL) return;
//> check-is-dark
  if (object->isDark) return;
  
//< check-is-dark
//> log-gray-object
#ifdef DEBUG_LOG_GC
  printf("%p gray ", object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

//< log-gray-object
  object->isDark = true;
//> add-to-gray-stack

  if (vm.grayCapacity < vm.grayCount + 1) {
    vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
    vm.grayStack = realloc(vm.grayStack,
                           sizeof(Obj*) * vm.grayCapacity);
  }

  vm.grayStack[vm.grayCount++] = object;
//< add-to-gray-stack
}
//< Garbage Collection gray-object
//> Garbage Collection gray-value
void grayValue(Value value) {
  if (!IS_OBJ(value)) return;
  grayObject(AS_OBJ(value));
}
//< Garbage Collection gray-value
//> Garbage Collection gray-array
static void grayArray(ValueArray* array) {
  for (int i = 0; i < array->count; i++) {
    grayValue(array->values[i]);
  }
}
//< Garbage Collection gray-array
//> Garbage Collection blacken-object
static void blackenObject(Obj* object) {
//> log-blacken-object
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

//< log-blacken-object
  switch (object->type) {
//> Methods and Initializers not-yet
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      grayValue(bound->receiver);
      grayObject((Obj*)bound->method);
      break;
    }
//< Methods and Initializers not-yet
//> Classes and Instances not-yet

    case OBJ_CLASS: {
      ObjClass* klass = (ObjClass*)object;
      grayObject((Obj*)klass->name);
//> Methods and Initializers not-yet
      grayTable(&klass->methods);
//< Methods and Initializers not-yet
      break;
    }

//< Classes and Instances not-yet
//> blacken-closure
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      grayObject((Obj*)closure->function);
      for (int i = 0; i < closure->upvalueCount; i++) {
        grayObject((Obj*)closure->upvalues[i]);
      }
      break;
    }

//< blacken-closure
//> blacken-function
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      grayObject((Obj*)function->name);
      grayArray(&function->chunk.constants);
      break;
    }

//< blacken-function
//> Classes and Instances not-yet
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      grayObject((Obj*)instance->klass);
      grayTable(&instance->fields);
      break;
    }

//< Classes and Instances not-yet
//> blacken-upvalue
    case OBJ_UPVALUE:
      grayValue(((ObjUpvalue*)object)->closed);
      break;

//< blacken-upvalue
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}
//< Garbage Collection blacken-object
//> Strings free-object
static void freeObject(Obj* object) {
//> Garbage Collection log-free-object
#ifdef DEBUG_LOG_GC
  printf("%p free ", object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

//< Garbage Collection log-free-object
  switch (object->type) {
//> Methods and Initializers not-yet
    case OBJ_BOUND_METHOD:
      FREE(ObjBoundMethod, object);
      break;

//< Methods and Initializers not-yet
/* Classes and Instances not-yet < Methods and Initializers not-yet
    case OBJ_CLASS:
*/
//> Classes and Instances not-yet
//> Methods and Initializers not-yet
    case OBJ_CLASS: {
      ObjClass* klass = (ObjClass*)object;
      freeTable(&klass->methods);
//< Methods and Initializers not-yet
      FREE(ObjClass, object);
      break;
//> Methods and Initializers not-yet
    }
//< Methods and Initializers not-yet

//< Classes and Instances not-yet
//> Closures free-closure
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
//> free-upvalues
      FREE_ARRAY(Value, closure->upvalues, closure->upvalueCount);
//< free-upvalues
      FREE(ObjClosure, object);
      break;
    }

//< Closures free-closure
//> Calls and Functions free-function
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      freeChunk(&function->chunk);
      FREE(ObjFunction, object);
      break;
    }

//< Calls and Functions free-function
//> Classes and Instances not-yet
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      freeTable(&instance->fields);
      FREE(ObjInstance, object);
      break;
    }

//< Classes and Instances not-yet
//> Calls and Functions free-native
    case OBJ_NATIVE:
      FREE(ObjNative, object);
      break;

//< Calls and Functions free-native
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }
//> Closures free-upvalue

    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;
//< Closures free-upvalue
  }
}
//< Strings free-object
//> Garbage Collection mark-roots
static void markRoots() {
  for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
    grayValue(*slot);
  }
//> mark-closures

  for (int i = 0; i < vm.frameCount; i++) {
    grayObject((Obj*)vm.frames[i].closure);
  }
//< mark-closures
//> mark-open-upvalues

  for (ObjUpvalue* upvalue = vm.openUpvalues;
       upvalue != NULL;
       upvalue = upvalue->next) {
    grayObject((Obj*)upvalue);
  }
//< mark-open-upvalues
//> mark-globals

  grayTable(&vm.globals);
//< mark-globals
//> mark-compiler-roots
  grayCompilerRoots();
//< mark-compiler-roots
//> Methods and Initializers not-yet
  grayObject((Obj*)vm.initString);
//< Methods and Initializers not-yet
}
//< Garbage Collection mark-roots
//> Garbage Collection trace-references
static void traceReferences() {
  while (vm.grayCount > 0) {
    // Pop an item from the gray stack.
    Obj* object = vm.grayStack[--vm.grayCount];
    blackenObject(object);
  }
}
//< Garbage Collection trace-references
//> Garbage Collection sweep
static void sweep() {
//> sweep-strings
  // Delete unused interned strings.
  tableRemoveWhite(&vm.strings);

//< sweep-strings
  Obj** object = &vm.objects;
  while (*object != NULL) {
    if (!((*object)->isDark)) {
      // This object wasn't reached, so remove it from the list and
      // free it.
      Obj* unreached = *object;
      *object = unreached->next;
      freeObject(unreached);
    } else {
      // This object was reached, so unmark it (for the next GC) and
      // move on to the next.
      (*object)->isDark = false;
      object = &(*object)->next;
    }
  }
}
//< Garbage Collection sweep
//> Garbage Collection collect-garbage
void collectGarbage() {
//> log-before-collect
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = vm.bytesAllocated;
#endif

//< log-before-collect
//> call-mark-roots
  markRoots();
//< call-mark-roots
//> call-trace-references
  traceReferences();
//< call-trace-references
//> call-sweep
  sweep();
//< call-sweep
//> update-next-gc

  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
//< update-next-gc
//> log-after-collect

#ifdef DEBUG_LOG_GC
  printf("-- gc collected %ld bytes (from %ld to %ld) next at %ld\n",
         before - vm.bytesAllocated, before, vm.bytesAllocated,
         vm.nextGC);
#endif
//< log-after-collect
}
//< Garbage Collection collect-garbage
//> Strings free-objects
void freeObjects() {
  Obj* object = vm.objects;
  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }
//> Garbage Collection free-gray-stack

  free(vm.grayStack);
//< Garbage Collection free-gray-stack
}
//< Strings free-objects
