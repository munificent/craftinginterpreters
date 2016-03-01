#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

void initVM(VM* vm) {
  vm->stackSize = 0;
  
  vm->fromStart = malloc(MAX_HEAP);
  vm->fromEnd = vm->fromStart;
  vm->toStart = malloc(MAX_HEAP);
  vm->toEnd = vm->toStart;
}

static void push(VM* vm, Value value) {
  vm->stack[vm->stackSize++] = value;
}

static Value pop(VM* vm) {
  return vm->stack[--vm->stackSize];
}

void run(VM* vm, ObjFunction* function) {
  uint8_t* ip = function->code;
  for (;;) {
    switch (*ip++) {
      case OP_CONSTANT: {
        uint8_t constant = *ip++;
        push(vm, function->constants->elements[constant]);
        break;
      }
        
      case OP_ADD: {
        // TODO: Test types. Handle strings.
        double b = ((ObjNumber*)pop(vm))->value;
        double a = ((ObjNumber*)pop(vm))->value;
        push(vm, (Value)newNumber(vm, a + b));
        break;
      }
        
      case OP_SUBTRACT: {
        // TODO: Test types.
        double b = ((ObjNumber*)pop(vm))->value;
        double a = ((ObjNumber*)pop(vm))->value;
        push(vm, (Value)newNumber(vm, a - b));
        break;
      }
        
      case OP_MULTIPLY: {
        // TODO: Test types.
        double b = ((ObjNumber*)pop(vm))->value;
        double a = ((ObjNumber*)pop(vm))->value;
        push(vm, (Value)newNumber(vm, a * b));
        break;
      }
        
      case OP_DIVIDE: {
        // TODO: Test types.
        double b = ((ObjNumber*)pop(vm))->value;
        double a = ((ObjNumber*)pop(vm))->value;
        push(vm, (Value)newNumber(vm, a / b));
        break;
      }
 
      case OP_RETURN:
        //printValue(vm->stack[vm->stackSize - 1]);
        return;
    }
  }
}
