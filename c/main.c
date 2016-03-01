#include <stdbool.h>
#include <stdio.h>

#include "scanner.h"
#include "vm.h"

int main(int argc, const char * argv[]) {
//  const char* source = "([{)]},;.!";
//  const char* source = "+-*/% = == != < > <= >=";
//  const char* source = "1 // comment\n + // comment\n2";
//  const char* source = "an and andy";
//  const char* source = "1234 0 000 -123 12.34 .45 56. 1.2.3 4..5";
//  const char* source = "\"\" \"string\"";
//  Scanner scanner;
//  initScanner(&scanner, source);
//  
//  while (true) {
//    Token token = nextToken(&scanner);
//    printf("%d '%.*s'\n", token.type, token.length, token.start);
//    if (token.type == TOKEN_EOF) break;
//  }
  
  VM vm;
  initVM(&vm);
  
  uint8_t code[12];
  code[0] = OP_CONSTANT;
  code[1] = 0;
  code[2] = OP_CONSTANT;
  code[3] = 1;
  code[4] = OP_ADD;
  code[5] = OP_CONSTANT;
  code[6] = 0;
  code[7] = OP_CONSTANT;
  code[8] = 1;
  code[9] = OP_ADD;
  code[10] = OP_MULTIPLY;
  code[11] = OP_RETURN;
  
  ObjArray* constants = newArray(&vm, 2);
  constants->elements[0] = (Value)newNumber(&vm, 1);
  constants->elements[1] = (Value)newNumber(&vm, 2);
  
  ObjFunction* function = newFunction(&vm, code, 12, constants);
  run(&vm, function);
  collectGarbage(&vm);
  printStack(&vm);
  
  return 0;
}
