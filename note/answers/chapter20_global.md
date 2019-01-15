## 1

The optimization is pretty straightforward. When adding a string constant, we
look in the constant table to see if that string is already in there. The
interesting question is how. The simplest implementation is a linear scan over
the existing constants.

But that means compilation time is quadratic in the number of unique identifiers
in the chunk. While that's fine for relatively small programs, users have a
habit of writing larger programs than we ever anticipated. Virtually every
algorithm in the compiler that isn't linear is potentially a performance
problem.

Fortunately, we have a way of looking up strings in constant time -- a hash
table. So, in the compiler, we add a hash table that keeps track of the
identifier constants that have already been added. Each key is an identifier,
and its value is the index of the identifier in the constant table.

In compiler.c, add a module variable:

```c
Table stringConstants;
```

In `compile()`, we initialize and tear it down:

```c
bool compile(const char* source, Chunk* chunk) {
  initScanner(source);

  compilingChunk = chunk;
  parser.hadError = false;
  parser.panicMode = false;
  initTable(&stringConstants); // <--

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  endCompiler();
  freeTable(&stringConstants); // <--
  return !parser.hadError;
}
```

When adding an identifier constant, we look for it in the hash table first:

```c
static uint8_t identifierConstant(Token* name) {
  // See if we already have it.
  ObjString* string = copyString(name->start, name->length);
  Value indexValue;
  if (tableGet(&stringConstants, string, &indexValue)) {
    // We do.
    return (uint8_t)AS_NUMBER(indexValue);
  }

  uint8_t index = makeConstant(OBJ_VAL(string));
  tableSet(&stringConstants, string, NUMBER_VAL((double)index));
  return index;
}
```

That's pretty simple. Compiling an identifier is still (amortized) constant
time, though with slightly worse constant factors. In return, we use up fewer
constant table slots. We don't actually save memory from redundant strings
because clox already interns all strings. But the smaller table is nice.

*Note that we leak memory for the identifier string in `identifierConstant()`
if the name is already found. That's because we don't have a GC yet.*

## 2

There are a few ways to solve this. I'll do one that introduces another layer
of indirection, and a little information sharing between the compiler and VM.

In the VM, we remove the `global` hash table and replace it with:

```c
  Table globalNames;
  ValueArray globalValues;
```

The value array is where the global variable values live. The hash table maps
the name of a global variable to its index in the value array. So, if the
program is:

```lox
var a = "value";
```

Then `globalNames` will contain a single entry, `"a" -> 0` and `globalValues`
will contain a single element, `"value"`. This association is all wired up at
compile time:

```c
static uint8_t identifierConstant(Token* name) {
  Value index;
  ObjString* identifier = copyString(name->start, name->length);
  if (tableGet(&vm.globalNames, identifier, &index)) {
    return (uint8_t)AS_NUMBER(index);
  }

  writeValueArray(&vm.globalValues, UNDEFINED_VAL);
  uint8_t newIndex = (uint8_t)vm.globalValues.count;

  tableSet(&vm.globalNames, identifier, NUMBER_VAL((double)newIndex));
  return newIndex;
}
```

When compiling a reference to a global variable, we see if we've ever
encountered its name before. If so, we know what index the value will be in in
the `globalValues` array. Otherwise, we add a new empty undefined value in the
array and then store a new hash table entry binding the name to that index.

Even though these two fields live in the VM, the compiler creates them at
compile time. You can think of it sort of like statically allocating memory for
the globals. We actually store the values in the VM so that they persist across
multiple REPL entries. We need to store the name association there too so that
we can find existing global variables.

`UNDEFINED_VAL` is a new, separate singleton value like `nil`. It's used to
mark a global variable slot as not having been defined yet. We can't use `nil`
because `nil` is a valid value to store in a variable.

At runtime, the instructions work like so:

```c
      case OP_GET_GLOBAL: {
        Value value = vm.globalValues.values[READ_BYTE()];
        if (IS_UNDEFINED(value)) {
          runtimeError("Undefined variable.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }

      case OP_DEFINE_GLOBAL: {
        vm.globalValues.values[READ_BYTE()] = pop();
        break;
      }

      case OP_SET_GLOBAL: {
        uint8_t index = READ_BYTE();
        if (IS_UNDEFINED(vm.globalValues.values[index])) {
          runtimeError("Undefined variable.");
          return INTERPRET_RUNTIME_ERROR;
        }
        vm.globalValues.values[index] = peek(0);
        break;
      }
```

The operand for the instructions is now the direct index of the global variable
in the `globalValues` array. We've looked up the slot at compile time and
bound the result, so at runtime we don't need to worry about the name at all.
This is much faster. The only perf hit we take now is the necessary check at
runtime to ensure the variable has been initialized.

## 3

TODO
