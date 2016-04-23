#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "vm.h"

VM vm;

static Value clockNative(int argCount, Value* args) {
  return (Value)newNumber((double)clock() / CLOCKS_PER_SEC);
}

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
    size_t instruction = frame->ip - frame->closure->function->code;
    int line = frame->closure->function->codeLines[instruction];
    // TODO: Include function name.
    fprintf(stderr, "[line %d]\n", line);
  }
}

static void defineNative(const char* name, NativeFn function) {
  push((Value)newString((uint8_t*)name, (int)strlen(name)));
  push((Value)newNative(function));
  tableSet(&vm.globals, (ObjString*)vm.stack[0], vm.stack[1]);
  pop();
  pop();
}

void initVM() {
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.objects = NULL;
  vm.openUpvalues = NULL;
  
  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;

  initTable(&vm.globals);
  
  defineNative("clock", clockNative);
  defineNative("print", printNative);
}

void endVM() {
  freeTable(&vm.globals);
  freeObjects();
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}

static bool callClosure(ObjClosure* closure, int argCount) {
  if (argCount < closure->function->arity) {
    runtimeError("Not enough arguments.");
    return false;
  }
  
  // TODO: Check for overflow.
  CallFrame* frame = &vm.frames[vm.frameCount++];
  frame->closure = closure;
  frame->ip = closure->function->code;

  // +1 to include either the called function or the receiver.
  frame->slots = vm.stackTop - (argCount + 1);
  return true;
}

static bool call(Value callee, int argCount) {
  if (IS_CLASS(callee)) {
    ObjClass* klass = (ObjClass*)callee;

    ObjInstance* instance = newInstance(klass);
    
    // Swap out the class for the instance.
    vm.stackTop[-argCount - 1] = (Value)instance;
    
    // TODO: Call constructor if there is one.
    vm.stackTop -= argCount;
    return true;
  }
  
  if (IS_CLOSURE(callee)) {
    return callClosure((ObjClosure*)callee, argCount);
  }
  
  if (IS_NATIVE(callee)) {
    NativeFn native = ((ObjNative*)callee)->function;
    Value result = native(argCount,
                          vm.stackTop - argCount);
    vm.stackTop -= argCount + 1;
    push(result);
    return true;
  }
  
  runtimeError("Can only call functions and classes.");
  return false;
}

static bool invoke(Value receiver, ObjString* name, int argCount) {
  if (!IS_INSTANCE(receiver)) {
    runtimeError("Only instances have methods.");
    return false;
  }
  
  ObjInstance* instance = (ObjInstance*)receiver;

  // First look for a field which may shadow a method.
  Value value;
  if (tableGet(&instance->fields, name, &value)) {
    vm.stackTop[-argCount] = value;
    return call(value, argCount);
  }

  // Look for a method.
  ObjClass* klass = instance->klass;
  Value method;
  if (tableGet(&klass->methods, name, &method)) {
    return callClosure((ObjClosure*)method, argCount);
  }

  runtimeError("%s does not implement '%s'.",
               instance->klass->name->chars, name->chars);
  return false;
}

// Captures the local variable [local] into an [Upvalue]. If that local is
// already in an upvalue, the existing one is used. (This is important to
// ensure that multiple closures closing over the same variable actually see
// the same variable.) Otherwise, it creates a new open upvalue and adds it to
// the VM's list of upvalues.
static ObjUpvalue* captureUpvalue(Value* local) {
  // If there are no open upvalues at all, we must need a new one.
  if (vm.openUpvalues == NULL) {
    vm.openUpvalues = newUpvalue(local);
    return vm.openUpvalues;
  }
  
  ObjUpvalue* prevUpvalue = NULL;
  ObjUpvalue* upvalue = vm.openUpvalues;
  
  // Walk towards the bottom of the stack until we find a previously existing
  // upvalue or reach where it should be.
  while (upvalue != NULL && upvalue->value > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }
  
  // If we found it, reuse it.
  if (upvalue != NULL && upvalue->value == local) return upvalue;

  // We walked past the local on the stack, so there must not be an upvalue for
  // it already. Make a new one and link it in in the right place to keep the
  // list sorted.
  ObjUpvalue* createdUpvalue = newUpvalue(local);
  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL) {
    // The new one is the first one in the list.
    vm.openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }
  
  return createdUpvalue;
}

static void closeUpvalues(Value* last) {
  while (vm.openUpvalues != NULL &&
         vm.openUpvalues->value >= last) {
    ObjUpvalue* upvalue = vm.openUpvalues;
    
    // Move the value into the upvalue itself and point the upvalue to it.
    upvalue->closed = *upvalue->value;
    upvalue->value = &upvalue->closed;
    
    // Pop it off the open upvalue list.
    vm.openUpvalues = upvalue->next;
  }
}

static void bindMethod(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = (ObjClass*)peek(1);
  tableSet(&klass->methods, name, method);
  pop();
}

static void createClass(ObjString* name, ObjClass* superclass) {
  ObjClass* klass = newClass(name, superclass);
  push((Value)klass);
  
  // Inherit methods.
  if (superclass != NULL) {
    for (int i = 0; i < superclass->methods.count; i++) {
      TableEntry* method = &superclass->methods.entries[i];
      tableSet(&klass->methods, method->key, method->value);
    }
  }
}

static bool popNumbers(double* a, double* b) {
  if (!IS_NUMBER(vm.stackTop[-1]) || !IS_NUMBER(vm.stackTop[-2])) {
    runtimeError("Operands must be numbers.");
    return false;
  }

  *b = ((ObjNumber*)pop())->value;
  *a = ((ObjNumber*)pop())->value;
  return true;
}

static bool popNumber(double* a) {
  if (!IS_NUMBER(vm.stackTop[-1])) {
    runtimeError("Operand must be a number.");
    return false;
  }
  
  *a = ((ObjNumber*)pop())->value;
  return true;
}

static bool isFalsey(Value value) {
  return IS_NULL(value) ||
        (IS_BOOL(value) && ((ObjBool*)value)->value == false);
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
  
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->constants.values[READ_BYTE()])
#define READ_SYMBOL() ((ObjString*)READ_CONSTANT())
  
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("| ");
      printValue(*slot);
      printf(" ");
    }
    printf("\n");
    printInstruction(frame->closure->function,
                     (int)(frame->ip - frame->closure->function->code));
#endif
    
    uint8_t instruction;
    switch (instruction = *frame->ip++) {
      case OP_CONSTANT: {
        push(READ_CONSTANT());
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
        push(frame->slots[slot]);
        break;
      }
        
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = peek(0);
        break;
      }
        
      case OP_GET_GLOBAL: {
        ObjString* name = READ_SYMBOL();
        Value value;
        if (!tableGet(&vm.globals, name, &value)) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return false;
        }
        push(value);
        break;
      }
        
      case OP_DEFINE_GLOBAL: {
        ObjString* name = READ_SYMBOL();
        tableSet(&vm.globals, name, peek(0));
        pop();
        break;
      }
        
      case OP_SET_GLOBAL: {
        ObjString* name = READ_SYMBOL();
        if (!tableSet(&vm.globals, name, peek(0))) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return false;
        }
        break;
      }
        
      case OP_GET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        push(*frame->closure->upvalues[slot]->value);
        break;
      }
        
      case OP_SET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        *frame->closure->upvalues[slot]->value = pop();
        break;
      }
        
      case OP_GET_FIELD: {
        if (!IS_INSTANCE(peek(0))) {
          runtimeError("Only instances have fields.");
          return false;
        }
        
        ObjInstance* instance = (ObjInstance*)pop();
        ObjString* name = READ_SYMBOL();
        Value value;
        if (tableGet(&instance->fields, name, &value)) {
          push(value);
          break;
        }
        
        runtimeError("Undefined field '%s'.", name->chars);
        return false;
        break;
      }
        
      case OP_SET_FIELD: {
        if (!IS_INSTANCE(peek(1))) {
          runtimeError("Only instances have fields.");
          return false;
        }
        
        ObjInstance* instance = (ObjInstance*)peek(1);
        tableSet(&instance->fields, READ_SYMBOL(), peek(0));
        Value value = pop();
        pop();
        push(value);
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
          runtimeError("Operands must be two numbers or two strings.");
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
        
      case OP_NOT:
        push((Value)newBool(isFalsey(pop())));
        break;

      case OP_NEGATE: {
        double a;
        if (!popNumber(&a)) return false;
        push((Value)newNumber(-a));
        break;
      }
        
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        frame->ip += offset;
        break;
      }
        
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek(0))) frame->ip += offset;
        break;
      }
        
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        frame->ip -= offset;
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
        if (!call(peek(argCount), argCount)) return false;
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
        
      case OP_INVOKE_0:
      case OP_INVOKE_1:
      case OP_INVOKE_2:
      case OP_INVOKE_3:
      case OP_INVOKE_4:
      case OP_INVOKE_5:
      case OP_INVOKE_6:
      case OP_INVOKE_7:
      case OP_INVOKE_8: {
        ObjString* method = READ_SYMBOL();
        int argCount = instruction - OP_INVOKE_0;
        if (!invoke(peek(argCount), method, argCount)) return false;
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
        
      case OP_CLOSURE: {
        ObjFunction* function = (ObjFunction*)READ_CONSTANT();
        
        // Create the closure and push it on the stack before creating upvalues
        // so that it doesn't get collected.
        ObjClosure* closure = newClosure(function);
        push((Value)closure);
        
        // Capture upvalues.
        for (int i = 0; i < function->upvalueCount; i++) {
          uint8_t isLocal = READ_BYTE();
          uint8_t index = READ_BYTE();
          if (isLocal) {
            // Make an new upvalue to close over the parent's local variable.
            closure->upvalues[i] = captureUpvalue(frame->slots + index);
          } else {
            // Use the same upvalue as the current call frame.
            closure->upvalues[i] = frame->closure->upvalues[index];
          }
        }

        break;
      }
        
      case OP_CLOSE_UPVALUE:
        closeUpvalues(vm.stackTop - 1);
        pop();
        break;
        
      case OP_RETURN: {
        Value result = pop();
        
        // Close any upvalues still in scope.
        closeUpvalues(frame->slots);

        if (vm.frameCount == 1) return true;
        
        vm.stackTop = frame->slots;
        push(result);
        
        vm.frameCount--;
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
        
      case OP_CLASS:
        createClass(READ_SYMBOL(), NULL);
        break;
        
      case OP_SUBCLASS: {
        Value superclass = pop();
        if (!IS_CLASS(superclass)) {
          runtimeError("Superclass must be a class.");
          return false;
        }
        
        createClass(READ_SYMBOL(), (ObjClass*)superclass);
        break;
      }
        
      case OP_METHOD:
        bindMethod(READ_SYMBOL());
        break;
    }
  }
  
  return true;
}

InterpretResult interpret(const char* source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push((Value)function);
  ObjClosure* closure = newClosure(function);
  pop();
  push((Value)closure);
  call((Value)closure, 0);
  
  return run() ? INTERPRET_OK : INTERPRET_RUNTIME_ERROR;
}
