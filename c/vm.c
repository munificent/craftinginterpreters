#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "debug.h"
#include "vm.h"

//#define DEBUG_TRACE_EXECUTION

VM vm;

static Value printNative(int argCount, Value* args) {
  printValue(args[0]);
  printf("\n");
  return args[0];
}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  
  fputs("\n", stderr);
  
  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    size_t instruction = frame->ip - frame->function->code;
    int line = frame->function->codeLines[instruction];
    // TODO: Include function name.
    fprintf(stderr, "[line %d]\n", line);
  }
}

static bool call(ObjFunction* function, int argCount) {
  if (argCount < function->arity) {
    runtimeError("Not enough arguments.");
    return false;
  }

  // TODO: Check for overflow.
  CallFrame* frame = &vm.frames[vm.frameCount++];
  frame->function = function;
  frame->ip = function->code;
  // TODO: Should the frame's stack start include the called function or not?
  // If so, we need the compiler to set aside slot 0 for it. Also need to figure
  // out how we want to handle methods.
  frame->stackStart = vm.stackSize - function->arity;
  return true;
}

void initVM() {
  vm.frameCount = 0;
  vm.objects = NULL;
  
  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;

  vm.globals = newTable();
  
  // TODO: Clean up.
  vm.stack[vm.stackSize++] = (Value)newString((uint8_t*)"print", 5);
  vm.stack[vm.stackSize++] = (Value)newNative(printNative);
  tableSet(vm.globals, (ObjString*)vm.stack[0], vm.stack[1]);
  vm.stackSize = 0;
}

void endVM() {
  vm.globals = NULL;
  
  // Free all objects.
  Obj* obj = vm.objects;
  while (obj != NULL) {
    Obj* next = obj->next;
    freeObject(obj);
    obj = next;
  }
  
  free(vm.grayStack);
}

static void push(Value value) {
  vm.stack[vm.stackSize++] = value;
}

static Value pop() {
  return vm.stack[--vm.stackSize];
}

static Value peek(int distance) {
  return vm.stack[vm.stackSize - distance - 1];
}

// TODO: Lots of duplication here.
static bool popNumbers(double* a, double* b) {
  if (!IS_NUMBER(vm.stack[vm.stackSize - 1])) {
    runtimeError("Right operand must be a number.");
    return false;
  }

  if (!IS_NUMBER(vm.stack[vm.stackSize - 2])) {
    runtimeError("Left operand must be a number.");
    return false;
  }

  *b = ((ObjNumber*)pop())->value;
  *a = ((ObjNumber*)pop())->value;
  return true;
}

static bool popBool(bool* a) {
  if (!IS_BOOL(vm.stack[vm.stackSize - 1])) {
    runtimeError("Operand must be a boolean.");
    return false;
  }
  
  *a = ((ObjBool*)pop())->value;
  return true;
}

static bool popNumber(double* a) {
  if (!IS_NUMBER(vm.stack[vm.stackSize - 1])) {
    runtimeError("Operand must be a number.");
    return false;
  }
  
  *a = ((ObjNumber*)pop())->value;
  return true;
}

static void concatenate() {
  ObjString* b = (ObjString*)peek(0);
  ObjString* a = (ObjString*)peek(1);
  
  ObjString* result = newString(NULL, a->length + b->length);
  memcpy(result->chars, a->chars, a->length);
  memcpy(result->chars + a->length, b->chars, b->length);
  pop();
  pop();
  push((Value)result);
}

static bool run() {
  CallFrame* frame = &vm.frames[vm.frameCount - 1];
  uint8_t* ip = frame->ip;
  
#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    for (int i = 0; i < vm.stackSize; i++) {
      printf("| ");
      printValue(vm.stack[i]);
      printf(" ");
    }
    printf("\n");
    printInstruction(frame->function, (int)(ip - frame->function->code));
#endif
    
    uint8_t instruction;
    switch (instruction = *ip++) {
      case OP_CONSTANT: {
        uint8_t constant = READ_BYTE();
        push(frame->function->constants.values[constant]);
        break;
      }
        
      case OP_NULL:
        push(NULL);
        break;

      case OP_POP:
        pop();
        break;
        
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(vm.stack[frame->stackStart + slot]);
        break;
      }
        
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        vm.stack[frame->stackStart + slot] = peek(0);
        break;
      }
        
      case OP_GET_GLOBAL: {
        uint8_t constant = READ_BYTE();
        ObjString* name = (ObjString*)frame->function->constants.values[constant];
        Value global;
        if (!tableGet(vm.globals, name, &global)) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return false;
        }
        push(global);
        break;
      }
        
      case OP_DEFINE_GLOBAL: {
        uint8_t constant = READ_BYTE();
        ObjString* name = (ObjString*)frame->function->constants.values[constant];
        tableSet(vm.globals, name, peek(0));
        pop();
        break;
      }
        
      case OP_SET_GLOBAL: {
        uint8_t constant = READ_BYTE();
        ObjString* name = (ObjString*)frame->function->constants.values[constant];
        if (!tableSet(vm.globals, name, peek(0))) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return false;
        }
        break;
      }
        
      case OP_EQUAL: {
        bool equal = valuesEqual(peek(0), peek(1));
        pop(); pop();
        push((Value)newBool(equal));
        break;
      }

      case OP_GREATER: {
        double a, b;
        if (!popNumbers(&a, &b)) return false;
        push((Value)newBool(a > b));
        break;
      }
        
      case OP_LESS: {
        double a, b;
        if (!popNumbers(&a, &b)) return false;
        push((Value)newBool(a < b));
        break;
      }

      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = ((ObjNumber*)pop())->value;
          double a = ((ObjNumber*)pop())->value;
          push((Value)newNumber(a + b));
        } else {
          runtimeError("Can only add two strings or two numbers.");
          return false;
        }
        break;
      }
        
      case OP_SUBTRACT: {
        double a, b;
        if (!popNumbers(&a, &b)) return false;
        push((Value)newNumber(a - b));
        break;
      }
        
      case OP_MULTIPLY: {
        double a, b;
        if (!popNumbers(&a, &b)) return false;
        push((Value)newNumber(a * b));
        break;
      }
        
      case OP_DIVIDE: {
        double a, b;
        if (!popNumbers(&a, &b)) return false;
        push((Value)newNumber(a / b));
        break;
      }
        
      case OP_NOT: {
        bool a;
        if (!popBool(&a)) return false;
        push((Value)newBool(!a));
        break;
      }

      case OP_NEGATE: {
        double a;
        if (!popNumber(&a)) return false;
        push((Value)newNumber(-a));
        break;
      }
        
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        ip += offset;
        break;
      }
        
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        Value condition = peek(0);
        if (IS_NULL(condition) ||
            (condition->type == OBJ_BOOL &&
                ((ObjBool*)condition)->value == false)) {
          ip += offset;
        }
        break;
      }
        
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        ip -= offset;
        break;
      }
        
      case OP_CALL_0:
      case OP_CALL_1:
      case OP_CALL_2:
      case OP_CALL_3:
      case OP_CALL_4:
      case OP_CALL_5:
      case OP_CALL_6:
      case OP_CALL_7:
      case OP_CALL_8: {
        int argCount = instruction - OP_CALL_0;
        Value called = peek(argCount);
        
        if (IS_NATIVE(called)) {
          NativeFn native = ((ObjNative*)called)->function;
          Value result = native(argCount,
                                &vm.stack[vm.stackSize - argCount]);
          vm.stackSize -= argCount + 1;
          push(result);
        } else if (IS_FUNCTION(called)) {
          ObjFunction* function = (ObjFunction*)called;
          frame->ip = ip;
          if (!call(function, argCount)) return false;
          frame = &vm.frames[vm.frameCount - 1];
          ip = function->code;
        } else {
          runtimeError("Can only call functions and classes.");
          return false;
        }
        break;
      }
        
      case OP_RETURN: {
        Value result = pop();
        // TODO: Close upvalues.
        if (vm.frameCount == 1) return true;
        
        // TODO: -1 here because the stack start does not include the function,
        // which we also want to discard.
        vm.stackSize = frame->stackStart - 1;
        push(result);
        
        vm.frameCount--;
        frame = &vm.frames[vm.frameCount - 1];
        ip = frame->ip;
        break;
      }
    }
  }
  
  return true;
}

InterpretResult interpret(const char* source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  call(function, 0);
  
  return run() ? INTERPRET_OK : INTERPRET_RUNTIME_ERROR;
}
