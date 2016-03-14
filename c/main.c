#include <stdbool.h>
#include <stdio.h>

#include "vm.h"

#define MAX_LINE_LENGTH 1024

int main(int argc, const char * argv[]) {
  initVM();
  
  char line[MAX_LINE_LENGTH];
  for (;;) {
    printf("> ");
    
    if (!fgets(line, MAX_LINE_LENGTH, stdin)) {
      printf("\n");
      break;
    }
    
    interpret(line);
  }
  
//  interpret("false and true;");
  
  endVM();
  
  return 0;
}
