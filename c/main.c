#include <stdbool.h>
#include <stdio.h>

#include "compiler.h"
#include "vm.h"

#include "debug.h"

int main(int argc, const char * argv[]) {
  VM vm;
  initVM(&vm);
  
  ObjFunction* function = compile(&vm, "1 + 2 * 3 / 4 - 5");
  printFunction(function);
  run(&vm, function);
  collectGarbage(&vm);
  printStack(&vm);
  
  return 0;
}
