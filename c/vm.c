//>= Types of Values
#include <stdarg.h>
//>= A Virtual Machine
#include <stdio.h>
//>= Uhh
#include <stdlib.h>
//>= Strings
#include <string.h>
//>= Uhh
#include <time.h>

//>= A Virtual Machine
#include "common.h"
//>= Scanning on Demand
#include "compiler.h"
//>= A Virtual Machine
#include "debug.h"
//>= Strings
#include "object.h"
#include "memory.h"
//>= Uhh
#include "value.h"
//>= A Virtual Machine
#include "vm.h"

VM vm;
//>= Uhh

static Value clockNative(int argCount, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

// TODO: Test.
static Value strNative(int argCount, Value* args) {
  Value arg = args[0];
  switch (arg.type) {
    case VAL_BOOL:
      if (AS_BOOL(arg)) {
        return OBJ_VAL(copyString("true", 4));
      } else {
        return OBJ_VAL(copyString("false", 5));
      }
    case VAL_NIL: return OBJ_VAL(copyString("nil", 3));
    case VAL_NUMBER: {
      // This is large enough to hold any double converted to a string using
      // "%.14g". Example:
      //
      //     -1.12345678901234e-1022
      //
      // So we have:
      //
      // + 1 char for sign
      // + 1 char for digit
      // + 1 char for "."
      // + 14 chars for decimal digits
      // + 1 char for "e"
      // + 1 char for "-" or "+"
      // + 4 chars for exponent
      // + 1 char for "\0"
      // = 24
      char buffer[24];
      int length = sprintf(buffer, "%.14g", AS_NUMBER(arg));
      return OBJ_VAL(copyString(buffer, length));
    }
    case VAL_OBJ: {
      switch (OBJ_TYPE(arg)) {
        case OBJ_CLASS:
          return OBJ_VAL(AS_CLASS(arg)->name);
          
        case OBJ_BOUND_METHOD:
        case OBJ_CLOSURE:
        case OBJ_FUNCTION:
          return OBJ_VAL(copyString("<fn>", 4));
          
        case OBJ_INSTANCE: {
          ObjInstance* instance = AS_INSTANCE(arg);
          ObjString* className = instance->klass->name;
          int length = className->length + strlen(" instance");
          char* buffer = GROW_ARRAY(NULL, char, 0, length + 1);
          strcpy(buffer, className->chars);
          strcpy(buffer + className->length, " instance");
          return OBJ_VAL(takeString(buffer, length));
        }
          
        case OBJ_NATIVE:
          return OBJ_VAL(copyString("<native>", 8));
          
        case OBJ_STRING:
          return arg;
          
        case OBJ_UPVALUE:
          return OBJ_VAL(copyString("<upvalue>", 9));
      }
    }
  }
}
//>= A Virtual Machine

static void resetStack() {
  vm.stackTop = vm.stack;
//>= Uhh
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
//>= A Virtual Machine
}
//>= Types of Values

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);
//>= Uhh
  
  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code;
    fprintf(stderr, "[line %d] in %s\n",
            function->chunk.lines[instruction],
            function->name->chars);
  }
//>= Types of Values
  
  resetStack();
}
//>= Uhh

static void defineNative(const char* name, NativeFn function) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}
//>= A Virtual Machine

void initVM() {
//>= A Virtual Machine
  resetStack();
//>= Strings
  vm.objects = NULL;
//>= Uhh
  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;
  
  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;

  initTable(&vm.globals);
//>= Hash Tables
  initTable(&vm.strings);
//>= Uhh
  
  vm.initString = copyString("init", 4);
  
  defineNative("clock", clockNative);
  defineNative("str", strNative);
//>= A Virtual Machine
}

void endVM() {
//>= Uhh
  freeTable(&vm.globals);
//>= Hash Tables
  freeTable(&vm.strings);
//>= Uhh
  vm.initString = NULL;
//>= Strings
  freeObjects();
//>= A Virtual Machine
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}
//>= Types of Values

static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}
//>= Uhh

static bool callClosure(ObjClosure* closure, int argCount) {
  if (argCount < closure->function->arity) {
    runtimeError("Not enough arguments.");
    return false;
  }
  
  if (vm.frameCount == FRAMES_SIZE) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame* frame = &vm.frames[vm.frameCount++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;

  // +1 to include either the called function or the receiver.
  frame->slots = vm.stackTop - (argCount + 1);
  return true;
}

static bool call(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
        
        // Replace the bound method with the receiver so it's in the right slot
        // when the method is called.
        vm.stackTop[-argCount - 1] = bound->receiver;
        return callClosure(bound->method, argCount);
      }
        
      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(callee);
        
        // Create the instance.
        vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
        
        // Call the initializer, if there is one.
        Value initializer;
        if (tableGet(&klass->methods, vm.initString, &initializer)) {
          return callClosure(AS_CLOSURE(initializer), argCount);
        }
        
        // No initializer, so just discard the arguments.
        vm.stackTop -= argCount;
        return true;
      }
        
      case OBJ_CLOSURE:
        return callClosure(AS_CLOSURE(callee), argCount);
        
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        Value result = native(argCount, vm.stackTop - argCount);
        vm.stackTop -= argCount + 1;
        push(result);
        return true;
      }
        
      default:
        // Do nothing.
        break;
    }
  }
  
  runtimeError("Can only call functions and classes.");
  return false;
}

static bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount) {
  // Look for the method.
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }
  
  return callClosure(AS_CLOSURE(method), argCount);
}

static bool invoke(ObjString* name, int argCount) {
  Value receiver = peek(argCount);
  
  if (!IS_INSTANCE(receiver)) {
    runtimeError("Only instances have methods.");
    return false;
  }
  
  ObjInstance* instance = AS_INSTANCE(receiver);

  // First look for a field which may shadow a method.
  Value value;
  if (tableGet(&instance->fields, name, &value)) {
    vm.stackTop[-argCount] = value;
    return call(value, argCount);
  }
  
  return invokeFromClass(instance->klass, name, argCount);
}

static bool bindMethod(ObjClass* klass, ObjString* name) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }
  
  ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
  pop(); // Instance.
  push(OBJ_VAL(bound));
  return true;
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

static void defineMethod(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
}

static void createClass(ObjString* name, ObjClass* superclass) {
  ObjClass* klass = newClass(name, superclass);
  push(OBJ_VAL(klass));
  
  // Inherit methods.
  if (superclass != NULL) {
    tableAddAll(&superclass->methods, &klass->methods);
  }
}
//>= Types of Values

static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
//>= Strings

static void concatenate() {
  ObjString* b = AS_STRING(peek(0));
  ObjString* a = AS_STRING(peek(1));
  
  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';
  
  ObjString* result = takeString(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}
//>= A Virtual Machine

static bool run() {
//>= Uhh
  CallFrame* frame = &vm.frames[vm.frameCount - 1];
  
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
/*>= A Virtual Machine <= Hash Tables
  uint8_t* ip = vm.chunk->code;
 
#define READ_BYTE() (*ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

*/
/*>= A Virtual Machine <= Compiling Expressions
#define BINARY_OP(op) \
    do { \
      double b = pop(); \
      double a = pop(); \
      push(a op b); \
    } while (false)
*/
//>= Types of Values
#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers."); \
        return false; \
      } \
      \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } while (false)
//>= A Virtual Machine
 
  for (;;) {
//>= Uhh
#ifdef DEBUG_TRACE_EXECUTION
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("| ");
      printValue(*slot);
      printf(" ");
    }
    printf("\n");
    disassembleInstruction(&frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));
#endif
    
//>= A Virtual Machine
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: push(READ_CONSTANT()); break;
//>= Types of Values
      case OP_NIL: push(NIL_VAL); break;
      case OP_TRUE: push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
//>= Uhh
      case OP_POP: pop(); break;
        
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
        ObjString* name = READ_STRING();
        Value value;
        if (!tableGet(&vm.globals, name, &value)) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return false;
        }
        push(value);
        break;
      }
        
      case OP_DEFINE_GLOBAL: {
        ObjString* name = READ_STRING();
        tableSet(&vm.globals, name, peek(0));
        pop();
        break;
      }
        
      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (tableSet(&vm.globals, name, peek(0))) {
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
          runtimeError("Only instances have properties.");
          return false;
        }
        
        ObjInstance* instance = AS_INSTANCE(peek(0));
        ObjString* name = READ_STRING();
        Value value;
        if (tableGet(&instance->fields, name, &value)) {
          pop(); // Instance.
          push(value);
          break;
        }
        
        if (!bindMethod(instance->klass, name)) return false;
        break;
      }
        
      case OP_SET_FIELD: {
        if (!IS_INSTANCE(peek(1))) {
          runtimeError("Only instances have fields.");
          return false;
        }
        
        ObjInstance* instance = AS_INSTANCE(peek(1));
        tableSet(&instance->fields, READ_STRING(), peek(0));
        Value value = pop();
        pop();
        push(value);
        break;
      }
        
      case OP_GET_SUPER: {
        ObjString* name = READ_STRING();
        ObjClass* superclass = AS_CLASS(pop());
        if (!bindMethod(superclass, name)) return false;
        break;
      }
//>= Types of Values
        
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }

      case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
/*>= A Virtual Machine <= Compiling Expressions
      case OP_ADD:      BINARY_OP(+); break;
      case OP_SUBTRACT: BINARY_OP(-); break;
      case OP_MULTIPLY: BINARY_OP(*); break;
      case OP_DIVIDE:   BINARY_OP(/); break;
      case OP_NEGATE:   push(-pop()); break;
*/
/*== Types of Values
      case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break;
*/
//>= Strings
        
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtimeError("Operands must be two numbers or two strings.");
          return false;
        }
        break;
      }
        
//>= Types of Values
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
//>= Types of Values
        
      case OP_NOT:
        push(BOOL_VAL(isFalsey(pop())));
        break;
        
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be a number.");
          return false;
        }
        
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
//>= Uhh
        
      case OP_PRINT: {
        Value string = strNative(1, &vm.stackTop[-1]);
        printf("%s\n", AS_STRING(string)->chars);
        pop();
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
        ObjString* method = READ_STRING();
        int argCount = instruction - OP_INVOKE_0;
        if (!invoke(method, argCount)) return false;
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
        
      case OP_SUPER_0:
      case OP_SUPER_1:
      case OP_SUPER_2:
      case OP_SUPER_3:
      case OP_SUPER_4:
      case OP_SUPER_5:
      case OP_SUPER_6:
      case OP_SUPER_7:
      case OP_SUPER_8: {
        ObjString* method = READ_STRING();
        int argCount = instruction - OP_SUPER_0;
        ObjClass* superclass = AS_CLASS(pop());
        if (!invokeFromClass(superclass, method, argCount)) {
          return false;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }

      case OP_CLOSURE: {
        ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
        
        // Create the closure and push it on the stack before creating upvalues
        // so that it doesn't get collected.
        ObjClosure* closure = newClosure(function);
        push(OBJ_VAL(closure));
        
        // Capture upvalues.
        for (int i = 0; i < closure->upvalueCount; i++) {
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
        
//>= A Virtual Machine
      case OP_RETURN: {
        Value result = pop();
/*>= A Virtual Machine <= Hash Tables
        printValue(result);
        printf("\n");
        return true;
*/
//>= Uhh
        
        // Close any upvalues still in scope.
        closeUpvalues(frame->slots);

        vm.frameCount--;
        if (vm.frameCount == 0) return true;
        
        vm.stackTop = frame->slots;
        push(result);
        
        frame = &vm.frames[vm.frameCount - 1];
//>= A Virtual Machine
        break;
      }
//>= Uhh
      case OP_CLASS:
        createClass(READ_STRING(), NULL);
        break;
        
      case OP_SUBCLASS: {
        Value superclass = peek(0);
        if (!IS_CLASS(superclass)) {
          runtimeError("Superclass must be a class.");
          return false;
        }
        
        createClass(READ_STRING(), AS_CLASS(superclass));
        break;
      }
        
      case OP_METHOD:
        defineMethod(READ_STRING());
        break;
//>= A Virtual Machine
    }
  }
  
  return true;
  
#undef READ_BYTE
//>= Uhh
#undef READ_SHORT
//>= A Virtual Machine
#undef READ_CONSTANT
//>= Uhh
#undef READ_STRING
//>= A Virtual Machine
#undef BINARY_OP
}

/*== A Virtual Machine
InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
*/
//>= Scanning on Demand
InterpretResult interpret(const char* source) {
/*== Scanning on Demand
  compile(source);
  return INTERPRET_OK;
*/
/*>= Compiling Expressions <= Hash Tables
  Chunk chunk;
  initChunk(&chunk);
  if (!compile(source, &chunk)) return INTERPRET_COMPILE_ERROR;
 
  vm.chunk = &chunk;
*/
//>= Uhh
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  ObjClosure* closure = newClosure(function);
  pop();
  call(OBJ_VAL(closure), 0);
  
//>= A Virtual Machine
  InterpretResult result = INTERPRET_RUNTIME_ERROR;
  if (run()) result = INTERPRET_OK;
/*>= Compiling Expressions <= Hash Tables
 
  freeChunk(&chunk);
*/
//>= A Virtual Machine
  return result;
}
