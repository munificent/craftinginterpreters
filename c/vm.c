#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

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

void pop(VM* vm) {
  vm->stackSize--;
}
