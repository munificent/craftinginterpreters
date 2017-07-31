//> A Virtual Machine not-yet
//> Types of Values not-yet
#include <stdarg.h>
//< Types of Values not-yet
#include <stdio.h>
//> Strings not-yet
#include <string.h>
//< Strings not-yet
//> Calls and Functions not-yet
#include <time.h>

//< Calls and Functions not-yet
#include "common.h"
//> Scanning on Demand not-yet
#include "compiler.h"
//< Scanning on Demand not-yet
#include "debug.h"
//> Strings not-yet
#include "object.h"
#include "memory.h"
//< Strings not-yet
#include "vm.h"

VM vm;
//> Calls and Functions not-yet

static Value clockNative(int argCount, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
//< Calls and Functions not-yet

static void resetStack() {
  vm.stackTop = vm.stack;
//> Calls and Functions not-yet
  vm.frameCount = 0;
//< Calls and Functions not-yet
//> Closures not-yet
  vm.openUpvalues = NULL;
//< Closures not-yet
}
//> Types of Values not-yet

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

/* Types of Values not-yet < Calls and Functions not-yet
  size_t instruction = vm.ip - vm.chunk->code;
  fprintf(stderr, "[line %d] in script\n", vm.chunk->lines[instruction]);
*/
//> Calls and Functions not-yet
  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
/* Calls and Functions not-yet < Closures not-yet
    ObjFunction* function = frame->function;
*/
//> Closures not-yet
    ObjFunction* function = frame->closure->function;
//< Closures not-yet
    size_t instruction = frame->ip - function->chunk.code;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }
//< Calls and Functions not-yet

  resetStack();
}
//< Types of Values not-yet
//> Calls and Functions not-yet

static void defineNative(const char* name, NativeFn function) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}
//< Calls and Functions not-yet

void initVM() {
  resetStack();
//> Strings not-yet
  vm.objects = NULL;
//< Strings not-yet
//> Garbage Collection not-yet
  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;

  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;
//< Garbage Collection not-yet
//> Global Variables not-yet

  initTable(&vm.globals);
//< Global Variables not-yet
//> Hash Tables not-yet
  initTable(&vm.strings);
//< Hash Tables not-yet
//> Methods and Initializers not-yet

  vm.initString = copyString("init", 4);
//< Methods and Initializers not-yet
//> Calls and Functions not-yet

  defineNative("clock", clockNative);
//< Calls and Functions not-yet
}

void endVM() {
//> Global Variables not-yet
  freeTable(&vm.globals);
//< Global Variables not-yet
//> Hash Tables not-yet
  freeTable(&vm.strings);
//< Hash Tables not-yet
//> Methods and Initializers not-yet
  vm.initString = NULL;
//< Methods and Initializers not-yet
//> Strings not-yet
  freeObjects();
//< Strings not-yet
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}
//> Types of Values not-yet

static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}
//< Types of Values not-yet
/* Calls and Functions not-yet < Closures not-yet

static bool call(ObjFunction* function, int argCount) {
  if (argCount != function->arity) {
    runtimeError("Expected %d arguments but got %d.",
        function->arity, argCount);
*/
//> Calls and Functions not-yet
//> Closures not-yet

static bool call(ObjClosure* closure, int argCount) {
  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.",
        closure->function->arity, argCount);
//< Closures not-yet
    return false;
  }

  if (vm.frameCount == FRAMES_SIZE) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame* frame = &vm.frames[vm.frameCount++];
/* Calls and Functions not-yet < Closures not-yet
  frame->function = function;
  frame->ip = function->chunk.code;
*/
//> Closures not-yet
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
//< Closures not-yet

  // +1 to include either the called function or the receiver.
  frame->slots = vm.stackTop - (argCount + 1);
  return true;
}

static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
//> Methods and Initializers not-yet
      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);

        // Replace the bound method with the receiver so it's in the right slot
        // when the method is called.
        vm.stackTop[-argCount - 1] = bound->receiver;
        return call(bound->method, argCount);
      }

//< Methods and Initializers not-yet
//> Classes and Instances not-yet
      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(callee);

        // Create the instance.
        vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
//> Methods and Initializers not-yet
        // Call the initializer, if there is one.
        Value initializer;
        if (tableGet(&klass->methods, vm.initString, &initializer)) {
          return call(AS_CLOSURE(initializer), argCount);
        } else if (argCount != 0) {
          runtimeError("Expected 0 arguments but got %d.", argCount);
          return false;
        }

//< Methods and Initializers not-yet
        return true;
      }
//< Classes and Instances not-yet
//> Closures not-yet

      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), argCount);

//< Closures not-yet
/* Calls and Functions not-yet < Closures not-yet
      case OBJ_FUNCTION:
        return call(AS_FUNCTION(callee), argCount);

*/
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
//< Calls and Functions not-yet
//> Methods and Initializers not-yet

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
//< Methods and Initializers not-yet
//> Closures not-yet

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
//< Closures not-yet
//> Methods and Initializers not-yet

static void defineMethod(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
}
//< Methods and Initializers not-yet
/* Classes and Instances not-yet < Superclasses not-yet

static void createClass(ObjString* name) {
  ObjClass* klass = newClass(name);
*/
//> Classes and Instances not-yet
//> Superclasses not-yet

static void createClass(ObjString* name, ObjClass* superclass) {
  ObjClass* klass = newClass(name, superclass);
//< Superclasses not-yet
  push(OBJ_VAL(klass));
//> Superclasses not-yet

  // Inherit methods.
  if (superclass != NULL) {
    tableAddAll(&superclass->methods, &klass->methods);
  }
//< Superclasses not-yet
}
//< Classes and Instances not-yet
//> Types of Values not-yet

static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
//< Types of Values not-yet
//> Strings not-yet

static void concatenate() {
/* Strings not-yet < Garbage Collection not-yet
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());
*/
//> Garbage Collection not-yet
  ObjString* b = AS_STRING(peek(0));
  ObjString* a = AS_STRING(peek(1));
//< Garbage Collection not-yet

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
//> Garbage Collection not-yet
  pop();
  pop();
//< Garbage Collection not-yet
  push(OBJ_VAL(result));
}
//< Strings not-yet

static bool run() {
//> Calls and Functions not-yet
  CallFrame* frame = &vm.frames[vm.frameCount - 1];

/* A Virtual Machine not-yet < Calls and Functions not-yet
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
*/
/* Jumping Forward and Back not-yet < Calls and Functions not-yet
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
*/
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
//< Calls and Functions not-yet
/* Calls and Functions not-yet < Closures not-yet
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
*/
//> Closures not-yet
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
//< Closures not-yet
//> Global Variables not-yet
#define READ_STRING() AS_STRING(READ_CONSTANT())
//< Global Variables not-yet
/* A Virtual Machine not-yet < Types of Values not-yet

#define BINARY_OP(op) \
    do { \
      double b = pop(); \
      double a = pop(); \
      push(a op b); \
    } while (false)
*/
//> Types of Values not-yet

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
//< Types of Values not-yet

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
/* A Virtual Machine not-yet < Calls and Functions not-yet
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
*/
/* Calls and Functions not-yet < Closures not-yet
    disassembleInstruction(&frame->function->chunk,
        (int)(frame->ip - frame->function->chunk.code));
*/
//> Closures not-yet
    disassembleInstruction(&frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));
//< Closures not-yet
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: push(READ_CONSTANT()); break;
//> Types of Values not-yet
      case OP_NIL: push(NIL_VAL); break;
      case OP_TRUE: push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
//< Types of Values not-yet
//> Global Variables not-yet
      case OP_POP: pop(); break;
//< Global Variables not-yet
//> Local Variables not-yet

      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
/* Local Variables not-yet < Calls and Functions not-yet
        push(vm.stack[slot]);
*/
//> Calls and Functions not-yet
        push(frame->slots[slot]);
//< Calls and Functions not-yet
        break;
      }

      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
/* Local Variables not-yet < Calls and Functions not-yet
        vm.stack[slot] = peek(0);
*/
//> Calls and Functions not-yet
        frame->slots[slot] = peek(0);
//< Calls and Functions not-yet
        break;
      }
//< Local Variables not-yet
//> Global Variables not-yet

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
//< Global Variables not-yet
//> Closures not-yet

      case OP_GET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        push(*frame->closure->upvalues[slot]->value);
        break;
      }

      case OP_SET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        *frame->closure->upvalues[slot]->value = peek(0);
        break;
      }
//< Closures not-yet
//> Classes and Instances not-yet

      case OP_GET_PROPERTY: {
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

/* Classes and Instances not-yet < Methods and Initializers not-yet
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
*/
//> Methods and Initializers not-yet
        if (!bindMethod(instance->klass, name)) return false;
        break;
//< Methods and Initializers not-yet
      }

      case OP_SET_PROPERTY: {
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
//< Classes and Instances not-yet
//> Superclasses not-yet

      case OP_GET_SUPER: {
        ObjString* name = READ_STRING();
        ObjClass* superclass = AS_CLASS(pop());
        if (!bindMethod(superclass, name)) return false;
        break;
      }
//< Superclasses not-yet
//> Types of Values not-yet

      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }

      case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
//< Types of Values not-yet
/* A Virtual Machine not-yet < Types of Values not-yet
      case OP_ADD:      BINARY_OP(+); break;
      case OP_SUBTRACT: BINARY_OP(-); break;
      case OP_MULTIPLY: BINARY_OP(*); break;
      case OP_DIVIDE:   BINARY_OP(/); break;
      case OP_NEGATE:   push(-pop()); break;
*/
/* Types of Values not-yet < Strings not-yet
      case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break;
*/
//> Strings not-yet

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

//< Strings not-yet
//> Types of Values not-yet
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;

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
//< Types of Values not-yet
//> Global Variables not-yet

      case OP_PRINT: {
        printValue(pop());
        printf("\n");
        break;
      }
//< Global Variables not-yet
//> Jumping Forward and Back not-yet

      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
/* Jumping Forward and Back not-yet < Calls and Functions not-yet
        vm.ip += offset;
*/
//> Calls and Functions not-yet
        frame->ip += offset;
//< Calls and Functions not-yet
        break;
      }

      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
/* Jumping Forward and Back not-yet < Calls and Functions not-yet
        if (isFalsey(peek(0))) vm.ip += offset;
*/
//> Calls and Functions not-yet
        if (isFalsey(peek(0))) frame->ip += offset;
//< Calls and Functions not-yet
        break;
      }

      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
/* Jumping Forward and Back not-yet < Calls and Functions not-yet
        vm.ip -= offset;
*/
//> Calls and Functions not-yet
        frame->ip -= offset;
//< Calls and Functions not-yet
        break;
      }
//< Jumping Forward and Back not-yet
//> Calls and Functions not-yet

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
//< Calls and Functions not-yet
//> Methods and Initializers not-yet

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
//< Methods and Initializers not-yet
//> Superclasses not-yet

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
//< Superclasses not-yet
//> Closures not-yet

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

//< Closures not-yet
      case OP_RETURN: {
/* A Virtual Machine not-yet < Global Variables not-yet
        printValue(pop());
        printf("\n");
*/
/* A Virtual Machine not-yet < Calls and Functions not-yet
        return true;
*/
//> Calls and Functions not-yet
        Value result = pop();
//> Closures not-yet

        // Close any upvalues still in scope.
        closeUpvalues(frame->slots);
//< Closures not-yet

        vm.frameCount--;
        if (vm.frameCount == 0) return true;

        vm.stackTop = frame->slots;
        push(result);

        frame = &vm.frames[vm.frameCount - 1];
//< Calls and Functions not-yet
        break;
      }
//> Classes and Instances not-yet

      case OP_CLASS:
/* Classes and Instances not-yet < Superclasses not-yet
        createClass(READ_STRING());
*/
//> Superclasses not-yet
        createClass(READ_STRING(), NULL);
//< Superclasses not-yet
        break;
//< Classes and Instances not-yet
//> Superclasses not-yet

      case OP_SUBCLASS: {
        Value superclass = peek(0);
        if (!IS_CLASS(superclass)) {
          runtimeError("Superclass must be a class.");
          return false;
        }

        createClass(READ_STRING(), AS_CLASS(superclass));
        break;
      }
//< Superclasses not-yet
//> Methods and Initializers not-yet

      case OP_METHOD:
        defineMethod(READ_STRING());
        break;
//< Methods and Initializers not-yet
    }
  }

  return true;

#undef READ_BYTE
//> Jumping Forward and Back not-yet
#undef READ_SHORT
//< Jumping Forward and Back not-yet
#undef READ_CONSTANT
//> Global Variables not-yet
#undef READ_STRING
//< Global Variables not-yet
#undef BINARY_OP
}

/* A Virtual Machine not-yet < Scanning on Demand not-yet
InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
*/
//> Scanning on Demand not-yet
InterpretResult interpret(const char* source) {
/* Scanning on Demand not-yet < Compiling Expressions not-yet
  compile(source);
  return INTERPRET_OK;
*/
/* Compiling Expressions not-yet < Calls and Functions not-yet
  Chunk chunk;
  initChunk(&chunk);
  if (!compile(source, &chunk)) return INTERPRET_COMPILE_ERROR;

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;
*/
//> Calls and Functions not-yet
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

//< Calls and Functions not-yet
/* Calls and Functions not-yet < Closures not-yet
  callValue(OBJ_VAL(function), 0);
*/
//> Garbage Collection not-yet
  push(OBJ_VAL(function));
//< Garbage Collection not-yet
//> Closures not-yet
  ObjClosure* closure = newClosure(function);
//< Closures not-yet
//> Garbage Collection not-yet
  pop();
//< Garbage Collection not-yet
//> Closures not-yet
  callValue(OBJ_VAL(closure), 0);

//< Closures not-yet
//< Scanning on Demand not-yet
  InterpretResult result = INTERPRET_RUNTIME_ERROR;
  if (run()) result = INTERPRET_OK;
/* Compiling Expressions not-yet < Calls and Functions not-yet

  freeChunk(&chunk);
*/
  return result;
}
