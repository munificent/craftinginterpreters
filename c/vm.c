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
//>= A Virtual Machine

static void resetStack() {
  vm.stackTop = vm.stack;
//>= Functions
  vm.frameCount = 0;
//>= Closures
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

/*>= Types of Values <= Jumping Forward and Back
  size_t instruction = vm.ip - vm.chunk->code;
  fprintf(stderr, "[line %d] in script\n", vm.chunk->lines[instruction]);
*/
//>= Functions
  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
/*== Functions
    ObjFunction* function = frame->function;
*/
//>= Closures
    ObjFunction* function = frame->closure->function;
//>= Functions
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
//>= Garbage Collection
  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;
  
  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;
//>= Global Variables
  
  initTable(&vm.globals);
//>= Hash Tables
  initTable(&vm.strings);
//>= Methods and Initializers
  
  vm.initString = copyString("init", 4);
//>= Uhh
  
  defineNative("clock", clockNative);
//>= A Virtual Machine
}

void endVM() {
//>= Global Variables
  freeTable(&vm.globals);
//>= Hash Tables
  freeTable(&vm.strings);
//>= Methods and Initializers
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
/*== Functions
 
static bool call(ObjFunction* function, int argCount) {
  if (argCount < function->arity) {
*/
//>= Closures

static bool call(ObjClosure* closure, int argCount) {
  if (argCount < closure->function->arity) {
//>= Functions
    runtimeError("Not enough arguments.");
    return false;
  }
  
  if (vm.frameCount == FRAMES_SIZE) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame* frame = &vm.frames[vm.frameCount++];
/*== Functions
  frame->function = function;
  frame->ip = function->chunk.code;
*/
//>= Closures
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
//>= Functions
  
  // +1 to include either the called function or the receiver.
  frame->slots = vm.stackTop - (argCount + 1);
  return true;
}

static bool callValue(Value callee, int argCount) {
//>= Closures
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
//>= Methods and Initializers
      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
        
        // Replace the bound method with the receiver so it's in the right slot
        // when the method is called.
        vm.stackTop[-argCount - 1] = bound->receiver;
        return call(bound->method, argCount);
      }
        
//>= Classes and Instances
      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(callee);
        
        // Create the instance.
        vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
//>= Methods and Initializers
        // Call the initializer, if there is one.
        Value initializer;
        if (tableGet(&klass->methods, vm.initString, &initializer)) {
          return call(AS_CLOSURE(initializer), argCount);
        }
        
//>= Classes and Instances
        // Discard the arguments.
        // TODO: Make this an error?
        vm.stackTop -= argCount;
        return true;
      }
//>= Closures
        
      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), argCount);
//>= Uhh
        
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        Value result = native(argCount, vm.stackTop - argCount);
        vm.stackTop -= argCount + 1;
        push(result);
        return true;
      }
//>= Closures
        
      default:
        // Do nothing.
        break;
    }
  }
/*== Functions
  if (IS_FUNCTION(callee)) {
    return call(AS_FUNCTION(callee), argCount);
  }
*/
//>= Functions
  
  runtimeError("Can only call functions and classes.");
  return false;
}
//>= Methods and Initializers

static bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount) {
  // Look for the method.
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }
  
  return call(AS_CLOSURE(method), argCount);
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
    return callValue(value, argCount);
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
//>= Closures

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
//>= Methods and Initializers

static void defineMethod(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
}
/*>= Classes and Instances <= Methods and Initializers

static void createClass(ObjString* name) {
  ObjClass* klass = newClass(name);
*/
//>= Inheritance

static void createClass(ObjString* name, ObjClass* superclass) {
  ObjClass* klass = newClass(name, superclass);
//>= Classes and Instances
  push(OBJ_VAL(klass));
//>= Inheritance
  
  // Inherit methods.
  if (superclass != NULL) {
    tableAddAll(&superclass->methods, &klass->methods);
  }
//>= Classes and Instances
}
//>= Types of Values

static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
//>= Strings

static void concatenate() {
/*>= Strings <= Closures
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());
*/
//>= Garbage Collection
  ObjString* b = AS_STRING(peek(0));
  ObjString* a = AS_STRING(peek(1));
//>= Strings
  
  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';
  
  ObjString* result = takeString(chars, length);
//>= Garbage Collection
  pop();
  pop();
//>= Strings
  push(OBJ_VAL(result));
}
//>= A Virtual Machine

static bool run() {
//>= Functions
  CallFrame* frame = &vm.frames[vm.frameCount - 1];
  
/*>= A Virtual Machine <= Jumping Forward and Back
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
*/
/*== Jumping Forward and Back
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
*/
//>= Functions
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
/*== Functions
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
*/
//>= Closures
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
//>= Global Variables
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
/*>= A Virtual Machine <= Jumping Forward and Back
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
*/
/*== Functions
    disassembleInstruction(&frame->function->chunk,
        (int)(frame->ip - frame->function->chunk.code));
*/
//>= Closures
    disassembleInstruction(&frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));
//>= A Virtual Machine
#endif
    
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: push(READ_CONSTANT()); break;
//>= Types of Values
      case OP_NIL: push(NIL_VAL); break;
      case OP_TRUE: push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
//>= Statements
      case OP_POP: pop(); break;
//>= Local Variables
        
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
/*>= Local Variables <= Jumping Forward and Back
        push(vm.stack[slot]);
*/
//>= Functions
        push(frame->slots[slot]);
//>= Local Variables
        break;
      }
        
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
/*>= Local Variables <= Jumping Forward and Back
        vm.stack[slot] = peek(0);
*/
//>= Functions
        frame->slots[slot] = peek(0);
//>= Local Variables
        break;
      }
//>= Global Variables
        
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
//>= Closures
        
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
//>= Classes and Instances
        
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
        
/*== Classes and Instances
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
*/
//>= Methods and Initializers
        if (!bindMethod(instance->klass, name)) return false;
        break;
//>= Classes and Instances
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
//>= Inheritance
        
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
//>= Statements
        
      case OP_PRINT: {
        printValue(pop());
        printf("\n");
        break;
      }
//>= Jumping Forward and Back
        
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
/*== Jumping Forward and Back
        vm.ip += offset;
*/
//>= Functions
        frame->ip += offset;
//>= Jumping Forward and Back
        break;
      }
        
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
/*== Jumping Forward and Back
        if (isFalsey(peek(0))) vm.ip += offset;
*/
//>= Functions
        if (isFalsey(peek(0))) frame->ip += offset;
//>= Jumping Forward and Back
        break;
      }
        
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
/*== Jumping Forward and Back
        vm.ip -= offset;
*/
//>= Functions
        frame->ip -= offset;
//>= Jumping Forward and Back
        break;
      }
//>= Functions
        
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
        if (!callValue(peek(argCount), argCount)) return false;
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
//>= Methods and Initializers
        
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
//>= Inheritance
        
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
//>= Closures
        
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
/*>= A Virtual Machine <= Hash Tables
        printValue(pop());
        printf("\n");
*/
/*>= A Virtual Machine <= Jumping Forward and Back
        return true;
*/
//>= Functions
        Value result = pop();
//>= Closures
        
        // Close any upvalues still in scope.
        closeUpvalues(frame->slots);
//>= Functions
        
        vm.frameCount--;
        if (vm.frameCount == 0) return true;
        
        vm.stackTop = frame->slots;
        push(result);
        
        frame = &vm.frames[vm.frameCount - 1];
//>= A Virtual Machine
        break;
      }
//>= Classes and Instances
        
      case OP_CLASS:
/*>= Classes and Instances <= Methods and Initializers
        createClass(READ_STRING());
*/
//>= Inheritance
        createClass(READ_STRING(), NULL);
//>= Classes and Instances
        break;
//>= Inheritance
        
      case OP_SUBCLASS: {
        Value superclass = peek(0);
        if (!IS_CLASS(superclass)) {
          runtimeError("Superclass must be a class.");
          return false;
        }
        
        createClass(READ_STRING(), AS_CLASS(superclass));
        break;
      }
//>= Methods and Initializers
        
      case OP_METHOD:
        defineMethod(READ_STRING());
        break;
//>= A Virtual Machine
    }
  }
  
  return true;
  
#undef READ_BYTE
//>= Jumping Forward and Back
#undef READ_SHORT
//>= A Virtual Machine
#undef READ_CONSTANT
//>= Global Variables
#undef READ_STRING
//>= A Virtual Machine
#undef BINARY_OP
}

/*== A Virtual Machine
InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
*/
//>= Scanning on Demand
InterpretResult interpret(const char* source) {
/*== Scanning on Demand
  compile(source);
  return INTERPRET_OK;
*/
/*>= Compiling Expressions <= Jumping Forward and Back
  Chunk chunk;
  initChunk(&chunk);
  if (!compile(source, &chunk)) return INTERPRET_COMPILE_ERROR;
 
  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;
*/
//>= Functions
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;
  
/*== Functions
  callValue(OBJ_VAL(function), 0);
*/
//>= Garbage Collection
  push(OBJ_VAL(function));
//>= Closures
  ObjClosure* closure = newClosure(function);
//>= Garbage Collection
  pop();
//>= Closures
  callValue(OBJ_VAL(closure), 0);
  
//>= A Virtual Machine
  InterpretResult result = INTERPRET_RUNTIME_ERROR;
  if (run()) result = INTERPRET_OK;
/*>= Compiling Expressions <= Jumping Forward and Back
 
  freeChunk(&chunk);
*/
//>= A Virtual Machine
  return result;
}
