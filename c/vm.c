//>> A Virtual Machine 99
//>> Types of Values 99
#include <stdarg.h>
//<< Types of Values 99
#include <stdio.h>
//>> Strings 99
#include <string.h>
//<< Strings 99
//>> Calls and Functions 99
#include <time.h>

//<< Calls and Functions 99
#include "common.h"
//>> Scanning on Demand 99
#include "compiler.h"
//<< Scanning on Demand 99
#include "debug.h"
//>> Strings 99
#include "object.h"
#include "memory.h"
//<< Strings 99
#include "vm.h"

VM vm;
//>> Calls and Functions 99

static Value clockNative(int argCount, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
//<< Calls and Functions 99

static void resetStack() {
  vm.stackTop = vm.stack;
//>> Calls and Functions 99
  vm.frameCount = 0;
//<< Calls and Functions 99
//>> Closures 99
  vm.openUpvalues = NULL;
//<< Closures 99
}
//>> Types of Values 99

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

/*>= Types of Values 99 < Calls and Functions 99
  size_t instruction = vm.ip - vm.chunk->code;
  fprintf(stderr, "[line %d] in script\n", vm.chunk->lines[instruction]);
*/
//>> Calls and Functions 99
  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
/*>= Calls and Functions 99 < Closures 99
    ObjFunction* function = frame->function;
*/
//>> Closures 99
    ObjFunction* function = frame->closure->function;
//<< Closures 99
    size_t instruction = frame->ip - function->chunk.code;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }
//<< Calls and Functions 99

  resetStack();
}
//<< Types of Values 99
//>> Calls and Functions 99

static void defineNative(const char* name, NativeFn function) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}
//<< Calls and Functions 99

void initVM() {
  resetStack();
//>> Strings 99
  vm.objects = NULL;
//<< Strings 99
//>> Garbage Collection 99
  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;

  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;
//<< Garbage Collection 99
//>> Global Variables 99

  initTable(&vm.globals);
//<< Global Variables 99
//>> Hash Tables 99
  initTable(&vm.strings);
//<< Hash Tables 99
//>> Methods and Initializers 99

  vm.initString = copyString("init", 4);
//<< Methods and Initializers 99
//>> Calls and Functions 99

  defineNative("clock", clockNative);
//<< Calls and Functions 99
}

void endVM() {
//>> Global Variables 99
  freeTable(&vm.globals);
//<< Global Variables 99
//>> Hash Tables 99
  freeTable(&vm.strings);
//<< Hash Tables 99
//>> Methods and Initializers 99
  vm.initString = NULL;
//<< Methods and Initializers 99
//>> Strings 99
  freeObjects();
//<< Strings 99
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}
//>> Types of Values 99

static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}
//<< Types of Values 99
/*>= Calls and Functions 99 < Closures 99

static bool call(ObjFunction* function, int argCount) {
  if (argCount < function->arity) {
*/
//>> Calls and Functions 99
//>> Closures 99

static bool call(ObjClosure* closure, int argCount) {
  if (argCount < closure->function->arity) {
//<< Closures 99
    runtimeError("Not enough arguments.");
    return false;
  }

  if (vm.frameCount == FRAMES_SIZE) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame* frame = &vm.frames[vm.frameCount++];
/*>= Calls and Functions 99 < Closures 99
  frame->function = function;
  frame->ip = function->chunk.code;
*/
//>> Closures 99
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
//<< Closures 99

  // +1 to include either the called function or the receiver.
  frame->slots = vm.stackTop - (argCount + 1);
  return true;
}

static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
//>> Methods and Initializers 99
      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);

        // Replace the bound method with the receiver so it's in the right slot
        // when the method is called.
        vm.stackTop[-argCount - 1] = bound->receiver;
        return call(bound->method, argCount);
      }

//<< Methods and Initializers 99
//>> Classes and Instances 99
      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(callee);

        // Create the instance.
        vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
//>> Methods and Initializers 99
        // Call the initializer, if there is one.
        Value initializer;
        if (tableGet(&klass->methods, vm.initString, &initializer)) {
          return call(AS_CLOSURE(initializer), argCount);
        }

//<< Methods and Initializers 99
        // Ignore the arguments.
        vm.stackTop -= argCount;
        return true;
      }
//<< Classes and Instances 99
//>> Closures 99

      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), argCount);

//<< Closures 99
/*>= Calls and Functions 99 < Closures 99
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
//<< Calls and Functions 99
//>> Methods and Initializers 99

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
//<< Methods and Initializers 99
//>> Closures 99

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
//<< Closures 99
//>> Methods and Initializers 99

static void defineMethod(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
}
//<< Methods and Initializers 99
/*>= Classes and Instances 99 < Superclasses 99

static void createClass(ObjString* name) {
  ObjClass* klass = newClass(name);
*/
//>> Classes and Instances 99
//>> Superclasses 99

static void createClass(ObjString* name, ObjClass* superclass) {
  ObjClass* klass = newClass(name, superclass);
//<< Superclasses 99
  push(OBJ_VAL(klass));
//>> Superclasses 99

  // Inherit methods.
  if (superclass != NULL) {
    tableAddAll(&superclass->methods, &klass->methods);
  }
//<< Superclasses 99
}
//<< Classes and Instances 99
//>> Types of Values 99

static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
//<< Types of Values 99
//>> Strings 99

static void concatenate() {
/*>= Strings 99 < Garbage Collection 99
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());
*/
//>> Garbage Collection 99
  ObjString* b = AS_STRING(peek(0));
  ObjString* a = AS_STRING(peek(1));
//<< Garbage Collection 99

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
//>> Garbage Collection 99
  pop();
  pop();
//<< Garbage Collection 99
  push(OBJ_VAL(result));
}
//<< Strings 99

static bool run() {
//>> Calls and Functions 99
  CallFrame* frame = &vm.frames[vm.frameCount - 1];

/*>= A Virtual Machine 99 < Calls and Functions 99
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
*/
/*>= Jumping Forward and Back 99 < Calls and Functions 99
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
*/
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
//<< Calls and Functions 99
/*>= Calls and Functions 99 < Closures 99
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
*/
//>> Closures 99
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
//<< Closures 99
//>> Global Variables 99
#define READ_STRING() AS_STRING(READ_CONSTANT())
//<< Global Variables 99
/*>= A Virtual Machine 99 < Types of Values 99

#define BINARY_OP(op) \
    do { \
      double b = pop(); \
      double a = pop(); \
      push(a op b); \
    } while (false)
*/
//>> Types of Values 99

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
//<< Types of Values 99

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
/*>= A Virtual Machine 99 < Calls and Functions 99
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
*/
/*>= Calls and Functions 99 < Closures 99
    disassembleInstruction(&frame->function->chunk,
        (int)(frame->ip - frame->function->chunk.code));
*/
//>> Closures 99
    disassembleInstruction(&frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));
//<< Closures 99
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: push(READ_CONSTANT()); break;
//>> Types of Values 99
      case OP_NIL: push(NIL_VAL); break;
      case OP_TRUE: push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
//<< Types of Values 99
//>> Global Variables 99
      case OP_POP: pop(); break;
//<< Global Variables 99
//>> Local Variables 99

      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
/*>= Local Variables 99 < Calls and Functions 99
        push(vm.stack[slot]);
*/
//>> Calls and Functions 99
        push(frame->slots[slot]);
//<< Calls and Functions 99
        break;
      }

      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
/*>= Local Variables 99 < Calls and Functions 99
        vm.stack[slot] = peek(0);
*/
//>> Calls and Functions 99
        frame->slots[slot] = peek(0);
//<< Calls and Functions 99
        break;
      }
//<< Local Variables 99
//>> Global Variables 99

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
//<< Global Variables 99
//>> Closures 99

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
//<< Closures 99
//>> Classes and Instances 99

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

/*>= Classes and Instances 99 < Methods and Initializers 99
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
*/
//>> Methods and Initializers 99
        if (!bindMethod(instance->klass, name)) return false;
        break;
//<< Methods and Initializers 99
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
//<< Classes and Instances 99
//>> Superclasses 99

      case OP_GET_SUPER: {
        ObjString* name = READ_STRING();
        ObjClass* superclass = AS_CLASS(pop());
        if (!bindMethod(superclass, name)) return false;
        break;
      }
//<< Superclasses 99
//>> Types of Values 99

      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }

      case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
//<< Types of Values 99
/*>= A Virtual Machine 99 < Types of Values 99
      case OP_ADD:      BINARY_OP(+); break;
      case OP_SUBTRACT: BINARY_OP(-); break;
      case OP_MULTIPLY: BINARY_OP(*); break;
      case OP_DIVIDE:   BINARY_OP(/); break;
      case OP_NEGATE:   push(-pop()); break;
*/
/*>= Types of Values 99 < Strings 99
      case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break;
*/
//>> Strings 99

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

//<< Strings 99
//>> Types of Values 99
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
//<< Types of Values 99
//>> Global Variables 99

      case OP_PRINT: {
        printValue(pop());
        printf("\n");
        break;
      }
//<< Global Variables 99
//>> Jumping Forward and Back 99

      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
/*>= Jumping Forward and Back 99 < Calls and Functions 99
        vm.ip += offset;
*/
//>> Calls and Functions 99
        frame->ip += offset;
//<< Calls and Functions 99
        break;
      }

      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
/*>= Jumping Forward and Back 99 < Calls and Functions 99
        if (isFalsey(peek(0))) vm.ip += offset;
*/
//>> Calls and Functions 99
        if (isFalsey(peek(0))) frame->ip += offset;
//<< Calls and Functions 99
        break;
      }

      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
/*>= Jumping Forward and Back 99 < Calls and Functions 99
        vm.ip -= offset;
*/
//>> Calls and Functions 99
        frame->ip -= offset;
//<< Calls and Functions 99
        break;
      }
//<< Jumping Forward and Back 99
//>> Calls and Functions 99

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
//<< Calls and Functions 99
//>> Methods and Initializers 99

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
//<< Methods and Initializers 99
//>> Superclasses 99

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
//<< Superclasses 99
//>> Closures 99

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

//<< Closures 99
      case OP_RETURN: {
/*>= A Virtual Machine 99 < Global Variables 99
        printValue(pop());
        printf("\n");
*/
/*>= A Virtual Machine 99 < Calls and Functions 99
        return true;
*/
//>> Calls and Functions 99
        Value result = pop();
//>> Closures 99

        // Close any upvalues still in scope.
        closeUpvalues(frame->slots);
//<< Closures 99

        vm.frameCount--;
        if (vm.frameCount == 0) return true;

        vm.stackTop = frame->slots;
        push(result);

        frame = &vm.frames[vm.frameCount - 1];
//<< Calls and Functions 99
        break;
      }
//>> Classes and Instances 99

      case OP_CLASS:
/*>= Classes and Instances 99 < Superclasses 99
        createClass(READ_STRING());
*/
//>> Superclasses 99
        createClass(READ_STRING(), NULL);
//<< Superclasses 99
        break;
//<< Classes and Instances 99
//>> Superclasses 99

      case OP_SUBCLASS: {
        Value superclass = peek(0);
        if (!IS_CLASS(superclass)) {
          runtimeError("Superclass must be a class.");
          return false;
        }

        createClass(READ_STRING(), AS_CLASS(superclass));
        break;
      }
//<< Superclasses 99
//>> Methods and Initializers 99

      case OP_METHOD:
        defineMethod(READ_STRING());
        break;
//<< Methods and Initializers 99
    }
  }

  return true;

#undef READ_BYTE
//>> Jumping Forward and Back 99
#undef READ_SHORT
//<< Jumping Forward and Back 99
#undef READ_CONSTANT
//>> Global Variables 99
#undef READ_STRING
//<< Global Variables 99
#undef BINARY_OP
}

/*>= A Virtual Machine 99 < Scanning on Demand 99
InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
*/
//>> Scanning on Demand 99
InterpretResult interpret(const char* source) {
/*>= Scanning on Demand 99 < Compiling Expressions 99
  compile(source);
  return INTERPRET_OK;
*/
/*>= Compiling Expressions 99 < Calls and Functions 99
  Chunk chunk;
  initChunk(&chunk);
  if (!compile(source, &chunk)) return INTERPRET_COMPILE_ERROR;

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;
*/
//>> Calls and Functions 99
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

//<< Calls and Functions 99
/*>= Calls and Functions 99 < Closures 99
  callValue(OBJ_VAL(function), 0);
*/
//>> Garbage Collection 99
  push(OBJ_VAL(function));
//<< Garbage Collection 99
//>> Closures 99
  ObjClosure* closure = newClosure(function);
//<< Closures 99
//>> Garbage Collection 99
  pop();
//<< Garbage Collection 99
//>> Closures 99
  callValue(OBJ_VAL(closure), 0);

//<< Closures 99
//<< Scanning on Demand 99
  InterpretResult result = INTERPRET_RUNTIME_ERROR;
  if (run()) result = INTERPRET_OK;
/*>= Compiling Expressions 99 < Calls and Functions 99

  freeChunk(&chunk);
*/
  return result;
}
