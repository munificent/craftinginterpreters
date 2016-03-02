#include <stdbool.h>
#include <stdio.h>

#include "compiler.h"
#include "vm.h"

#include "debug.h"

int main(int argc, const char * argv[]) {
  vmInit();
  
  ObjFunction* function = compile("1 + 2 * 3 / 4 - 5");
  printFunction(function);
  vmRun(function);
  collectGarbage();
  printStack();
  
  return 0;
}
