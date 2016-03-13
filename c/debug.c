#include <stdio.h>

#include "debug.h"

void printValue(Value value) {
  switch (value->type) {
    case OBJ_BOOL:
      printf(((ObjBool*)value)->value ? "true" : "false");
      break;
      
    case OBJ_FUNCTION:
      printf("function");
      break;
      
    case OBJ_NUMBER:
      printf("%g", ((ObjNumber*)value)->value);
      break;
      
    case OBJ_STRING:
      printf("%.*s", ((ObjString*)value)->length, ((ObjString*)value)->chars);
      break;
      
    case OBJ_TABLE:
      printf("table");
      break;
  }
}

void printStack() {
  for (int i = 0; i < vm.stackSize; i++) {
    printf("%d: ", i);
    printValue(vm.stack[i]);
    printf("\n");
  }
}

void printFunction(ObjFunction* function) {
  for (int i = 0; i < function->codeCount;) {
    switch (function->code[i++]) {
      case OP_CONSTANT: {
        uint8_t constant = function->code[i++];
        printf("%-16s %4d '", "OP_CONSTANT", constant);
        printValue(function->constants.values[constant]);
        printf("'\n");
        break;
      }
        
      case OP_GET_GLOBAL: {
        uint8_t name = function->code[i++];
        printf("%-16s %4d '", "OP_GET_GLOBAL", name);
        printValue(function->constants.values[name]);
        printf("'\n");
        break;
      }
        
      case OP_DEFINE_GLOBAL: {
        uint8_t name = function->code[i++];
        printf("%-16s %4d '", "OP_DEFINE_GLOBAL", name);
        printValue(function->constants.values[name]);
        printf("'\n");
        break;
      }
        
      case OP_ASSIGN_GLOBAL: {
        uint8_t name = function->code[i++];
        printf("%-16s %4d '", "OP_ASSIGN_GLOBAL", name);
        printValue(function->constants.values[name]);
        printf("'\n");
        break;
      }
        
      case OP_GREATER: printf("OP_GREATER\n"); break;
      case OP_GREATER_EQUAL: printf("OP_GREATER_EQUAL\n"); break;
      case OP_LESS: printf("OP_LESS\n"); break;
      case OP_LESS_EQUAL: printf("OP_LESS_EQUAL\n"); break;
      case OP_ADD: printf("OP_ADD\n"); break;
      case OP_SUBTRACT: printf("OP_SUBTRACT\n"); break;
      case OP_MULTIPLY: printf("OP_MULTIPLY\n"); break;
      case OP_NOT: printf("OP_NOT\n"); break;
      case OP_DIVIDE: printf("OP_DIVIDE\n"); break;
      case OP_NEGATE: printf("OP_NEGATE\n"); break;
      case OP_RETURN: printf("OP_RETURN\n"); break;
    }
  }
  
  // TODO: Dump constant table?
}
