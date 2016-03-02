#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "debug.h"
#include "vm.h"

VM vm;

void initVM() {
  vm.stackSize = 0;
  
  vm.fromStart = malloc(MAX_HEAP);
  vm.fromEnd = vm.fromStart;
  vm.toStart = malloc(MAX_HEAP);
  vm.toEnd = vm.toStart;
}

void endVM() {
  free(vm.fromStart);
  free(vm.toStart);
}

static void push(Value value) {
  vm.stack[vm.stackSize++] = value;
}

static Value pop() {
  return vm.stack[--vm.stackSize];
}

static void run(ObjFunction* function) {
  uint8_t* ip = function->code;
  for (;;) {
    switch (*ip++) {
      case OP_CONSTANT: {
        uint8_t constant = *ip++;
        push(function->constants->elements[constant]);
        break;
      }
        
      case OP_ADD: {
        // TODO: Test types. Handle strings.
        double b = ((ObjNumber*)pop())->value;
        double a = ((ObjNumber*)pop())->value;
        push((Value)newNumber(a + b));
        break;
      }
        
      case OP_SUBTRACT: {
        // TODO: Test types.
        double b = ((ObjNumber*)pop())->value;
        double a = ((ObjNumber*)pop())->value;
        push((Value)newNumber(a - b));
        break;
      }
        
      case OP_MULTIPLY: {
        // TODO: Test types.
        double b = ((ObjNumber*)pop())->value;
        double a = ((ObjNumber*)pop())->value;
        push((Value)newNumber(a * b));
        break;
      }
        
      case OP_DIVIDE: {
        // TODO: Test types.
        double b = ((ObjNumber*)pop())->value;
        double a = ((ObjNumber*)pop())->value;
        push((Value)newNumber(a / b));
        break;
      }
 
      case OP_RETURN:
        //printValue(vm->stack[vm->stackSize - 1]);
        return;
    }
  }
}

void interpret(const char* source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return;

//  printFunction(function);
  run(function);
  collectGarbage();
  printStack();
}
