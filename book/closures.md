^title Closures
^part A Bytecode Virtual Machine

## closure obj

- currently, fn objects created at compile time
- just obj bound to name
- no instr to "create" fn at runtime, simply loaded from const table
- for closure, some runtime obj needs to be created

  ```lox
  fun makeClosure(value) {
    fun closure() {
      print value;
    }
    return closure;
  }

  var doughnut = makeClosure("doughnut");
  var bagel = makeClosure("bagel");
  doughnut();
  bagel();
  ```

- two fns must be diff since do different things
- first step is create runtime rep of closure
- wrap fn which contains static part of fn -- code and const
- eventually contain runtime state needed to close over vars
- every fn in clox wrapped in closure, even if doesn't close over anything
- simplifies vm because doesn't need to handle calling both closure and bare
  fn

### obj

- *todo: walk though code...*

### compiler code to create closure obj

- *todo: walk though code...*

### vm code to handle closure obj

- *todo: walk though code...*

## upvalues

- existing local instrs limited to fn's own stack window
- need new instr
- can't just look up var farther in stack
- eventually will hang on to var after parent fn returns
- todo: ex of function that returns closure

- could treat vars that closed over very different
- not put on stack at all
- if not single-pass compiler, might be good idea
- todo: example program that works with local var before closure refers to it
- already emitted code to treat var as local before discover used by closure
- so when closure work with var, need to handle it still on stack

- solution from lua
- solve with indirection, "upvalue"
- closure obj has array of upvalue objs
- each refers to unique var that fn accesses that's declared in surrounding fn
- only have upvalues for vars actually refed
- have one upvalue for each var, even if refed multiple times
- upvalue points back into stack where var lives
- can read and write through that

- when closure obj created, create upvalues and wire up pointers into stack
- instr for closed over vars has operand to index into upvalue array

- will extend later to see how works when fn containing closed-over var returns
- enough to get going

### compiling upvalues

- as usual want to do as much as possible at compile time
- since vars all lexical, can figure out exactly which vars each fn closes over
  and where defined
- thus know how many upvalues closure needs and which variables they refer to

- *talk about flattening and indirect upvalues...*

### upvalue obj

### upvalue in closure

## closed upvalues

---

### outline

- intro
  - have fns but no closures
    - ex of closure
  - harder because local vars on stack
    - assumes lifetime of var has stack semantics
    - ex of local living past return of fn
  - want add support for closures while keeping perf of locals on stack
  - take approach used by lua
    - [good fit for clox since also single-pass bytecode vm]
  - build in stages, intro concepts as needed

- closure obj

- open upvalues

### todo

- quote
- challenges
  - instead of always wrapping fn in closure, only do so if it has upvalues
    how does that affect perf? complexity?
- maybe a design note on closing over loop variables?
- explain ObjClosure has copy of upvalueCount because gc will need

- single pass compiler can't eagerly place closed-over variables into upvalues
  - because local can be used as local before closure uses it, have already
    generated code to access it directly on stack

- stuff
  - vm linked list of open upvalues
  - wrapping fn in closure
    - capturing upvalues
  - closing upvalues at end of scope
  - upvalue is obj, but never first-class visible in lox prog
    - use obj so gc can manage mem
  - different closures over same fn may capture different vars
  - implies need some runtime obj to rep closure since not all data known at
    compile time
  - wrap ObjFunction in ObjClosure as first step
  - define new obj type
  - OP_CLOSURE
  - update vm to go through that to get to fn

```lox
fun main() {
  var x = 1;

  fun middle() {
    var y = 0;

    fun closure() {
      print x;
    }

    closure();
    return closure;
  }

  var captured = middle();
  captured();
}
```

in this example, when closure for c is first created, x, which is closes over
is already off the stack. this is why closures are flattened:

```lox
fun a(x) {
  fun b() {
    print "create c";
    fun c() {
      print x;
    }

    print "end of b";
    return c;
  }

  print "end of a";
  return b;
}

a("capture")()();
// prints:
// end of a
// create c
// end of b
// capture
```

here, the function that declares the variable assigns to it after the closure
has captured it. this is why the upvalue needs to point back to the stack.
also, it assigns to the variable before the closure is compiled. since we do
single pass compilation, we have to compile that assignment to a normal local
assign.

```lox
fun a(x) {
  var x = 1;
  x = 2;

  fun b() {
    print x;
  }

  b();
  x = 3;
  b();
}
```



## closure obj

...

^code obj-closure

...

^code new-closure-h (2 before, 1 after)

...

^code new-closure

...

^code obj-type-closure (1 before, 1 after)

...

^code free-closure (1 before, 1 after)

...

^code is-closure (2 before, 1 after)

...

^code as-closure (2 before, 1 after)

...

^code print-closure (1 before, 1 after)

### compiling

**todo: currently, no special instr for fns, just load const, so runtime
does not know when to create closure.**

^code emit-closure (1 before, 1 after)

...

^code closure-op (1 before, 1 after)

...

^code disassemble-closure (3 before, 1 after)

...

### closure in vm

**todo: remove comment**

^code interpret-closure (1 before, 1 after)

...

^code call-value-closure (1 before, 1 after)

...

^code call (1 before, 1 after)

...

^code call-init-closure (1 before, 2 after)

...

^code call-frame-closure (1 before, 1 after)

...

^code read-constant (1 before, 1 after)

...

^code disassemble-instruction (1 before, 1 after)

...

^code runtime-error-function (1 before, 1 after)

...

^code interpret (2 before, 2 after)

## compiling open upvalues

- compiler upvalue struct
- resolve outer locals

...

^code named-variable-upvalue (1 before, 1 after)

...

**todo: merge snippets?**

^code get-upvalue-op (1 before, 1 after)

...

^code set-upvalue-op (1 before, 1 after)

...

^code resolve-upvalue

...

^code add-upvalue

...

^code upvalue-count (1 before, 1 after)

...

^code init-upvalue-count (1 before, 1 after)

**todo: separate snippet to look for existing?**

...

^code upvalue-struct

...

^code upvalues-array (1 before, 1 after)

...

^code too-many-upvalues (1 before, 1 after)

...

^code resolve-upvalue-recurse (1 before, 2 after)

...

^code capture-upvalues (1 before, 1 after)

...

^code disassemble-upvalue-ops (2 before, 1 after)

...

**todo: say that in OP_CLOSURE case since context doesn't show**

^code disassemble-upvalues (2 before, 1 after)

**todo: show example output**

## open upvalues in vm

- fn accesses outer local var still on stack
- upvalues
- capturing upvalues

...

^code interpret-get-upvalue (1 before, 2 after)

...

^code interpret-set-upvalue (1 before, 2 after)

...

### upvalue objects

...

^code obj-upvalue

...

^code new-upvalue-h (1 before, 1 after)

...

^code new-upvalue

...

^code obj-type-upvalue (1 before, 1 after)

...

^code free-upvalue (3 before, 1 after)

...

^code print-upvalue (3 before, 1 after)

...

### upvalues in closures

...

^code upvalue-fields (1 before, 1 after)

...

^code allocate-upvalue-array (1 before, 1 after)

**todo: null slots in case of gc**

...

^code init-upvalue-fields (1 before, 1 after)

...

^code free-upvalues (1 before, 1 after)

...

^code interpret-capture-upvalues (1 before, 1 after)

...

^code capture-upvalue

**todo: split into snippets?**

...

^code open-upvalues-field (1 before, 3 after)

...

^code init-open-upvalues (1 before, 1 after)

## closed upvalues

- move into upvalue
- close on scope end

^code end-scope (3 before, 2 after)

...

^code is-upvalue-field (1 before, 1 after)

...

^code init-is-upvalue (1 before, 1 after)

...

^code init-zero-local-is-upvalue (1 before, 1 after)

...

^code mark-local-upvalue (1 before, 1 after)

...

^code close-upvalue-op (1 before, 1 after)

...

^code disassemble-close-upvalue (1 before, 1 after)

...

### runtime support

...

^code interpret-close-upvalue (2 before, 1 after)

...

^code close-upvalues

...

^code closed-field (2 before, 1 after)

...

^code init-closed (1 before, 1 after)

...

^code return-close-upvalues (1 before, 2 after)

...
