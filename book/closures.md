^title Closures
^part A Bytecode Virtual Machine

> As the man said, for every complex problem there's a simple solution, and it's
> wrong.
> <cite>Umberto Eco, <em>Foucault's Pendulum</em></cite>

- have fns but no closures
- fn can't access outer local

```lox
var x = "global";
fun outer() {
  var x = "outer";

  fun inner() {
    print x;
  }

  inner();
}
```

- prints global
- to fix, need to include entire lex scope when resolve
- problem harder in clox because locals on stack
- in last chapter, said locals have stack semantics
  - never need local once out of scope
- closures make not true

```lox
fun makeClosure() {
  var local = "local";
  fun closure() {
    print local;
  }
  return closure;
}

var closure = makeClosure();
closure();
```

- *todo: explain*

- could solve by heap allocating local vars
- what jlox does in envs
- but don't want to
- [not just because don't want to invalidate last chapter]
- stack is really fast
- most locals not used by closures
- don't want to make all locals slower for minority

- this chapter add support for closures
- does require heap alloc for some state
- closures mean lifetimes really not stack like, so need something dynamic
- but goal is to keep non-closed over locals on stack
- inspiration for approach comes from design of lua vm
- really beautiful
- perfect fit for clox because both single pass
- [not only way to do this. some vms heap alloc call frames. see "lambda lifting"
  or "closure conversion"]
- build in stages, intro concepts as needed

## closure obj

- currently, fn objects created at compile time
- just obj bound to name
- no instr to "create" fn at runtime, simply loaded from const table
- work like literals of other types: string num

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

- start with struct for new obj type

^code obj-closure

- right now, just wraps bare fn and adds necessary obj head
- add fields as needed later

^code new-closure-h (2 before, 1 after)

- ctor

^code new-closure

- impl
- wrap given fn

^code obj-type-closure (1 before, 1 after)

- new type num

^code free-closure (1 before, 1 after)

- doesn't free fn
- closure doesn't own fn
- may be many closures all pointing to same fn
- in doughnut bagel ex, both closures ref same fn
- gc will take care of for us
- [by "for us", mean we'll do it, since we impl gc]

- usual macros
- [maybe should have made meta macro to emit these?]

^code is-closure (2 before, 1 after)

...

^code as-closure (2 before, 1 after)

...

^code print-closure (1 before, 1 after)

- edge case
- if have debug stack printing, may print closure that wraps fn for top level
  code
- no name

^code print-script (1 before, 1 after)

- prevents crash

### compiler code to create closure obj

- have obj
- next extend comp to emit instr to create closure for fn decl

^code emit-closure (1 before, 1 after)

- instead of simple `OP_CONSTANT` to load raw fn from const table and push to
  stack, have new instr
- like `OP_CONSTANT` takes constant table index as op
- identifies where fn is in const table

^code closure-op (1 before, 1 after)

- add dis

^code disassemble-closure (3 before, 1 after)

- little more dis than usual
- going to get more complex
- closure op pretty unusual

- onto runtime

### vm code to handle closure obj

- most work over in vm
- not just exec new op
- need to actually handle closure obj
- every place used fn now use closure
- callframe, call, etc.

- start with instr

^code interpret-closure (2 before, 1 after)

- load fn from const table
- alloc new closure to wrap
- push

- once have closure, eventually call it

^code call-value-closure (1 before, 1 after)

- remove code for call obj_fn
- all fns wrapped in closures now, so rest of vm never see bare fn

- sig for call changes

^code call (1 before, 1 after)

- also fix code for arity check to unwrap closure to get to fn
- call creates new callframe
- change to work with closure

^code call-init-closure (1 before, 2 after)

- instead of fn, stores closure
- ip field doesn't change, but to init need to go through closure

- over in callframe

^code call-frame-closure (1 before, 1 after)

- replace fn field with closure

- couple of places in vm that use old fn field, need to fix
- macro for reading from fn's const tabl

^code read-constant (1 before, 1 after)

- when diag enabled to print each instr as executed

^code disassemble-instruction (1 before, 1 after)

- and runtime err

^code runtime-error-function (1 before, 1 after)

- also need to handle special code that sets up first call frame

^code interpret (2 before, 2 after)

- wraps bare fn compiler returns for compiled top level code
- little weird that still push fn onto stack then pop after creating closure
- again, keeps gc happy

- back to working interp
- no user vis diff, but not all fns wrapped in closures
- [seem like wasteful perf hit to wrap all fns even those that don't close.
  only happens when exec fn *decl*. in practice, those rarely happen inside
  hot loops, so not usually perf crit.]
- ready to start making closures do something

## upvalues

- existing local instrs limited to fn's own stack window
- need new instr
- can't just look up var farther in stack
- showed earlier, eventually will hang on to var after parent fn returns

- could treat vars that closed over very different
- not put on stack at all
- if not single-pass compiler, might be good idea
- vm design makes harder

```lox
fun outer() {
  var x = 1;
  x = 2;
  fun inner() {
    print x;
  }
  inner();
}
```

- already emitted code to treat x as local before discover used by closure
- so when closure work with var, need to handle it still on stack

- solution from lua
- solve with indirection, "upvalue"
- closure obj has array of upvalue objs
- each refers to unique var that fn accesses that's declared in surrounding fn

**todo: illustrate**

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

- when resolve ident, walk lexical scope
- first try local scopes in order
- if not there, global
- between insert

^code named-variable-upvalue (1 before, 1 after)

- new fn will look for local variable declared in surrounding fn
- if found, return "upvalue index"
- get into that later
- otherwise, return -1
- if was found, use insts for reading or writing through upvalue

^code upvalue-ops (1 before, 1 after)

- get to what do soon
- more interesting is how resolve works

^code resolve-upvalue

- call this after failing to resolve local
- so know var not in current compiler
- remember have chain of enclosing compilers, one for each surrounding fn
- use now to walk enclosing fn scopes
- look in enclosing
- if is now enclosing, at outermost fn
- if didn't find yet, must be global
- [or undefined]

- otherwise, look for local variable in *enclosing* compiler


```lox
fun outer() {
  var x = 1;
  fun inner() {
    print x; // <--
  }
  inner();
}
```

- so when compiling ident expr on marked line here, look for local variable
  in `outer()`

- if found, then we successfully resolved
- create upvalue for it so inner fn can access

^code add-upvalue

- compiler has array of upvalues to track resolved for each ident in fn
- at end of fn when emits code to create closure, will emit special code to
  close over those variables
- adds new upvalue to array
- note also store updated count in objfn itself
- not just in compiler
- important because runtime need to know how many upvalue fn has too

- track what local slot it was in in enclosing fn so know which local to close
  over
- get to `isLocal` soon
- then return index of upvalue in array
- identifies which upvalue slot is used to access in closure
- this what goes into `OP_UPVALUE` instr operand

- fn may use same var in surrounding fn multiple times
- in that case, don't want separate upvalue for each use
- so before add new upvalue, check for existing

^code existing-upvalue (1 before, 1 after)

- add new fields for data
- first in objfn track num upvalue slots

^code upvalue-count (1 before, 1 after)

- zero out when create new fn

^code init-upvalue-count (1 before, 1 after)

- might seem weird to put in fn not closure
- but number of upvalues -- num external local vars accessed -- is static
  property of code
- can determine entirely at compile time
- fn is "static" part of runtime fn obj, so naturally here

- during compilation, track each one, so compiler get array

^code upvalues-array (1 before, 1 after)

- has fixed size
- use single byte op to access upvalue, so max
- compiler needs to make sure don't overflow

^code too-many-upvalues (5 before, 1 after)

- finally struct itself

^code upvalue-struct

- index is which local slot in surrounding fn upval refers to
- other field is interesting

### upvalue flattening

- so far, assume always access local in immediately enclosing, but what about


```lox
fun outer() {
  var x = 1;
  fun middle() {
    fun inner() {
      print x;
    }
  }
}
```

- here, `x` declared in farther fn
- need to handle too
- not as simple as looking farther up stack
- consider

```lox
fun outer() {
  fun middle() {
    fun inner() {
      print x;
    }

    print "create inner closure";
    return inner;
  }

  print "return from outer";
  return middle;
}

outer("capture")()();
```

when run, prints:

```
prints:
return from outer
create inner closure
capture
```

- note that outer has already returned before inner even *captures* `x`, much
  less uses

**todo: illustrate execution flow and stack**

- have two problems
- need to find locals declared in not just immediate surround but further
- need way to close over them even when not already on stack

- already have solution for second
- looking right at it
- upvalues themselves exist to be able to refer to local no longer on stack
- soln is to flatten upvalues
- if refer to local variable in non-immediate enclosing,
  add it as upvalue to all intermediate fns
- then in inner fn, instead of closing over enclosing fn's local, close
  over one of its *upvalues*

- means resolve becomes recursive

^code resolve-upvalue-recurse (3 before, 2 after)

- [one of most difficult fns written
  - impl idea from lua, but getting it to click really hard
  - recursive in tricky way]

- tricky, so walk through slowly
- resolveupvalue first called after we know var is not local to current fn
- first try to resolve local var in immediately enclosing
- if found, we create upvalue for it
  - so always create first upvalue for local in fn right inside fn where
    local actually declared
  - in above ex, that's middle()
- othrwise, keep walking outwards by recursively calling resolve upvalue
- eventually this recursion will find fn where immediate surrounding contains
  local
- create first upvalue there
- then return to intermediate calls resolve
- each time, add corresponding upvalue in inner fn that closes over surrounding
  upvalue
- gets passed along each level of nesting

- for ex:

```lox
fun a(x) {
  fun b() {
    fun c() {
      print x;
    }
  }
}
```

todo: trace exec

- `isLocal` field is then how distinguish whether upvalue closes over local
  var in surrounding fn or upvalue

- by time compiler reaches end of fn decl
- for each var ref in fn, should now be resolved as either local, upvalue
  (which may close over local or upvalue in surrounding) or global
- have enough data to emit code to create closure that captures correct vars

^code capture-upvalues (1 before, 1 after)

- opclosure very special instr
- has one byte operand for fn index in const table
- can look up fn to find how many upvalues it needs to capture at runtime
- then, for each one, have two byte operands
- first byte is 1 if closing over local variable in enclosing fn, 0 if upvalue
- then next byte is slot index
- will be index of local var on stack in enclosing fn's stack window if closing
  over local
- if upvalue, will be in enclosing fn's closure's list of upvalues

- need special code to dis

^code disassemble-upvalues (2 before, 1 after)

**todo: show example output**

- two other new instrs

^code disassemble-upvalue-ops (2 before, 1 after)

- compiler now doing its job
- resolves vars defined in surrounding fns
- emits closure code to capture upvalues
- stores upvalue count in created fn

## open upvalues in vm

- move to vm
- ready for real runtime rep of upvalues

^code obj-upvalue

- upval obj has pointer to val on stack but itself lives on heap
- easiest way to manage is make obj
- let gc handle later

- usual obj machinery
- ctor

^code new-upvalue-h (1 before, 1 after)

- takes pointer to stack slot for local var where val lives
- impl

^code new-upvalue

- just inits obj header and stores pointer
- new obj type

^code obj-type-upvalue (1 before, 1 after)

- free it

^code free-upvalue (3 before, 1 after)

- just points to val on stack, but doesn't own
- and print

^code print-upvalue (3 before, 1 after)

- never actually reached by any user code, but keeps c compiler happy so switch
  covers all cases

### upvalues in closures

- earlier said closure obj has array of upvalues
- this how actually accesses vars from enclosing scopes
- finally time for that

^code upvalue-fields (1 before, 1 after)

- has pointer to dynamic array of upval pointers
- also track count
- [redundant from fn, but covers weird edge case where might have already freed
  fn before closure]

- when create closure, alloc array of proper size

^code allocate-upvalue-array (1 before, 1 after)

- create array before creating closure itself and init all fields to null
- as usual, weird stuff around mem is to satisfy gc gods
- ensures gc never sees uninitialized mem

- then initialize fields in closure itself

^code init-upvalue-fields (1 before, 1 after)

- handle freeing

^code free-upvalues (1 before, 1 after)

- upvalue does not own var, but closure does own array
- free array
- don't have to free upvalues
- gc will

- upvalue array initialized to real upvalues when closure is created
- this when interp handle operands compiler emited after opclosure

^code interpret-capture-upvalues (1 before, 1 after)

- expect pair of byte operands for each upvalue

- if first byte is 1, indicating we are closing over var that
  is not declared in immediate encl
- in that case, already in upvalue in current fn
- so just grab right from fn's upvalue array

- otherwise if first byte 0, capturing local in enclosing scope
- second byte is slot index in enclosing fn's stack frame
- since exec closure decl right now, not inside its body, current call frame
  is enclosing fn
- so offset from `frame->slots` which is pointer to first slot owned by fn
- then call `captureUpvalue()` to get upvalue for that local

^code capture-upvalue

- seems silly to define fn instead of call newupvalue directly
- revisit soon

- back to main loop
- when completes, have new closure with full init upvalue array

- now that closures have upvalues, can implement instrs that read and write

^code interpret-get-upvalue (1 before, 2 after)

- operand is index in current closure's upvalue array
- so simply look up upvalue in array, then deref value pointer to get to actual
  value on stack

- write is similar

^code interpret-set-upvalue (1 before, 2 after)

- as usual instructions that execute frequently, these ones, very simple and
  fast
- try to do as much work at compile time and closure creation time as possible

- now have working closures

```lox
fun outer() {
  var x = "outside";
  fun inner() {
    print x;
  }
  inner();
}
```

- prints "outside"

## closed upvalues

- above example works because `x` still on stack when calling `inner()`
- closures retain variable even when function where declared has returned:

```lox
fun outer() {
  var x = "outside";
  fun inner() {
    print x;
  }

  return inner;
}

var closure = outer();
closure();
```

- this does not work
- right now, just accessed trashed stack slot and does who knows what

- key problem is closed over vars do not have stack semantics
- means variable needs to not be on stack
- instead, live somewhere on heap

- last section of chapter is about getting this to work

### values and variables

- before start writing code, need to dig into really important semantic point
- "does a closure close over a value or a variable?"
- not an academic question, not just splitting hairs
- consider

```lox
var globalSet;
var globalGet;

fun main() {
  var a = "initial";

  fun set() { a = "updated"; }
  fun get() { print a; }

  globalSet = set;
  globalGet = get;
}

main();
globalSet();
globalGet();
```

- `main()` creates two closures and stores in global vars
- both closures close over same variable
- first closure assigns new value to variable
- second closure reads variable

- [globals not very significant here. just way of "returning" two fns from
  `main()`. if had lists, could return list of two fns]

- what does call to `globalGet()` print?
- if closures capture values, then each closure gets own copy of `a` with value
  it had at time when closure created
- call to `globalSet()` changes its copy of `a`, but `globalGet()` unaffected
- prints "initial"

- if closes over variables, both have reference to same mutable variable
- two closures connected
- when either changes var, other sees it
- prints "updated"

- answer for lox and other langs is the latter
- closure closes over variable

- [note only matters because variables can be assigned. if all locals final,
  then no difference]

- important to keep in mind as we deal with closed over vars
- when var no longer on stack, needs to be somewhere that all closures that
  reference same closed over var share
- that way, they all see assignments to var

### capturing upvalues

- know closed over var needs to be on heap
- questions are where on heap and when it gets there

- where easy: already have heap object just perfect for this objupvalue itself
- closed over var will live right inside new field in upvalue

- when little harder
- might be nice if var always lived in upvalue on heap
- but remember single pass compiler already emitted code that expects var to
  be on stack
- that code correct -- var will still be on stack when that code execs
- but means we can't have var on heap *before* that code
- in other words, have to *move* it from stack to upvalue

- logical place to do that is when var goes out of scope
- once out of scope, no code can be accessing it from stack any more

- compiler already emits pop when var goes out of scope to free stack slot
- if var gets closed over, instead emit new instruction to capture var and
  move into upvalue
- to do that, compiler needs to know which locals are closed over
- already tracks which surrounding vars a fn accesses for emit pseudo-ops after
  opclosure
- compiler upvalue list good for answering "which vars does this closure use?"
- not structured well to answer "is this var closed over by any fn?"

- [general obs here is that even when technically storing data, may not be in
  form can efficiently query. for ex: much faster to tell if given key present
  in map than if given value is. important to choose ds that aligns with
  queries need to perform. if need multiple queries, may need multiple ds even
  if technically all store "same" info]

- so add more tracking to existing local struct for that
- add new field

^code is-upvalue-field (1 before, 1 after)

- true if local used as upvalue by some later fn
- initially false

^code init-is-upvalue (1 before, 1 after)

- also false for hidden local slot zero

^code init-zero-local-is-upvalue (1 before, 1 after)

- whenever resolve reference to local in enclosing fn, set it

^code mark-local-upvalue (1 before, 1 after)

- now at end of block scope when discarding locals, can use that to emit
  special instr
- no operand because always closing local right at top of stack

^code end-scope (3 before, 2 after)

- new instr

^code close-upvalue-op (1 before, 1 after)

- dis

^code disassemble-close-upvalue (1 before, 1 after)

### tracking open upvalues

- ready for runtime side
- before add new support, have issue to resolve
- right now, every time create closure, each closure gets new upvalue for every
  var on stack it references
- breaks sharing described before
- instead, if already created an upvalue for some local slot, need to ensure
  that any later closures reuse same upvalue instead of creating dup
- do that now

- problem is hard to find them since locked up in closures
- closures could be anywhere

- to fix, vm keep own list of all upvalues that point to local slots
- can search that to find upvalue

- having to search list for every var closure refs can be slow
- however, number of closed over vars currently on stack tends to be small
- fun declarations not executed that frequently

- even better, can store list in order sorted by stack slot index
- common case is not already upvalue for some local
- rare that multiple closures close over same var
- also closures tend to close over nearby locals in immediate fn
- means tend to be near top of stack
- so as traverse upvalue list from top to bottom, if go past slot where local
  closing over lives, know it won't be found
- can early out
- in practice quite fast

- because list is sorted, need to insert easily
- linked list instead of dynamic array
- since we own upvalue struct, easiest way is intrusive list
- put next pointer right in upvalue

^code next-field (1 before, 1 after)

- when create upvalue, not part of list, so null link

^code init-next (1 before, 1 after)

- add field to vm for root of list

^code open-upvalues-field (1 before, 3 after)

- call upvalues that point to local slots still on stack "open"
- closed upvalues are upvalues for vars that have moved to heap
- list initially empty

^code init-open-upvalues (1 before, 1 after)

- now when vm captures local slot into upvalue, first look for existing one

^code look-for-existing-upvalue (1 before, 1 after)

- start at head, upvalue nearest top of stack
- work down from there
- three exit conditions:

    - first upvalue for exact stack slot looking for
      - in that case, `upvalue->local` will equal `local`
      - reuse that exact upvalue, so return it and done
    - ran out of upvalues to search
      - means definitely no upvalue for stack slot, so exit loop and create new
        one
    - last more interesting
      - since list is sorted, if found upvalue whose stack slot is below one
        looking for, know won't find upvalue for our slot
      - can early exit
      - also need to create new upvalue

- if found upvalue for slot, done
- otherwise, need to create new upvalue like before
- but also need to insert into list in appropriate place for later search

^code insert-upvalue-in-list (1 before, 1 after)

**todo: illustrate**

- exited loop either because ran out of upvalues, or stopped on upvalue that is
  below this one's local
- so new upvalue goes before it on list
- set next pointer to that upvalue (which may be null)
- then update prev pointer to point to new upvalue
- if in middle of list, that's `prevUpvalue`
- otherwise, inserting new head node, so update vm's head pointer

- [shorter impl that uses pointer to pointer so don't have to special case
  inserting at head of list
  confuses everyone]

- now vm correctly ensures only ever one upvalue for given variable
- ready to actually close them

### closing upvalues

- over in vm

^code interpret-close-upvalue (2 before, 1 after)

- when vm exec, local variable being moved to upvalue on top of stack
- call helper to move value into upvalue
- then pop to remove from stack

^code close-upvalues

- helper takes pointer to local stack slot
- closes every upvalue in that slot or higher
- right now, fn just closes single variable on top of slot
- but later use to close entire range of upvalues
- walks open upvalue list from top of stack down
- for every upvalue whose stack slot within range, closes it

- take current value of variable from local slot, copy to new field in upvalue
- next part is clever

- closed over var can live in two places, stack or heap
- how do get_upvalue and set_upvalue instrs handle?
- could add some conditional logic there to check upvalue state and then
  access appropriately
- but already accessing val through pointer
- pointer don't care where it points
- have indirection we need
- so when close upvalue and move value into objupvalue itself
  update pointer to point to that field right inside upvalue

**todo: illustrate**

- no change needed to upvalue instrs
- do need to add new field to upvalue struct

^code closed-field (2 before, 1 after)

- go ahead and zero out too so open upvalues don't have garbage data

^code init-closed (1 before, 1 after)

- reason close takes range is because also need to handle local variables in
  root scope of fn body
- [including params]
- compiler does not end that scope
- doesn't emit code to discard those slots because runtime implicitly pops them
  when discards callframe
- also means doesn't emit closeup instrs
- do need to close those, though
- so when exec return, automatically close upvalues for every local var fn
  still has

^code return-close-upvalues (1 before, 2 after)

- that's it
- quite a lot more complex than jlox
- having all envs heap alloc makes many things simpler
- lot more regular
- but simplicity comes at price
- always heap alloc memory for local variables even when majority do have
  stack semantics quite slow
- approach in clox more sophisticated
- basically two different implementations, each tuned for semantics that some
  locals have
- in return for complexity, much better perf
- [and mem usage, clox only heap allocs vars actually needed
  jlox keeps entire environment with every single var, even those closure
  never accesses]

- we have all of the features we need now to full implement lexical scoping
- also have a lot more interesting memory usage going on
- many heap alloc objects with lots of pointers between them
- closures can build big graphs of objects
- soon will be time for us to start managing that memory

<div class="challenges">

## Challenges

1.  allocating an objclosure and then going through that to reach fn for const
    table and chunk has overhead
    - not needed for fns that don't close over variables
    - change to not wrap fn in closure unless has upvalues
    - how does affect perf?

2.  read design note. what do you think lox should do?
    - change clox to create new var each time

**todo: more**

</div>

<div class="design-note">

## Design Note: Closing Over the Loop Variable

- closures close over var
- if two closures capture same var like get/set ex earlier, both share ref to it
- if closures capture different vars, obviously no sharing

```lox
var globalOne;
var globalTwo;

fun main() {
  {
    var a = "one";
    fun one() {
      print a;
    }
    globalOne = one;
  }

  {
    var a = "two";
    fun two() {
      print a;
    }
    globalTwo = two;
  }
}

main();
globalOne();
globalTwo();
```

- prints one then two
- usually clear when two vars are same or different
- but not always

```lox
var globalOne;
var globalTwo;

fun main() {
  for (var a = 1; a  2; a = a + 1) {
    fun closure() {
      print a;
    }
    if (globalOne == nil) {
      globalOne = closure;
    } else {
      globalTwo = closure;
    }
  }
}

main();
globalOne();
globalTwo();
```

- code really convoluted here because no collections in lox
- does two iter of loop
- creates closure for each iter and stores in two globals
- each closure captures loop variable

- q: do they capture *same* var, or does each iter get own separate var?
- is there one `a` or two?

- seems contrived, but consider in language where code is simpler
- here js:

```js
var closures = [];
for (var i = 1; i <= 2; i++) {
  closures.push(function () { console.log(i); });
}

closures[0]();
closures[1]();
```

- does print 1, 2, 3, or 4, 4, 4?
- answer is 4, 4, 4
- all close over same i
- i gets incremented to 4 on last iteration which is what causes loop to exit
- what you expected?
- if know js, know `var` hoisted to function or top level, so above code same
  as

```js
var closures = [];
var i;
for (i = 1; i <= 2; i++) {
  closures.push(function () { console.log(i); });
}

closures[0]();
closures[1]();
```

- hopefully little more obvious that all close over same var
- if use newer `let`:

```js
var closures = [];
for (let i = 1; i <= 2; i++) {
  closures.push(function () { console.log(i); });
}

closures[0]();
closures[1]();
```

- still same?
- no, now prints 1, 2
- strange when think about it
- increment clause is `i++`
- looks very much like assigning new value to *same* var

- try other langs

```python
closures = []
for i in range(1, 3):
  closures.append(lambda: print(i))

closures[0]()
closures[1]()
# 9
```

- python doesn't really have local scopes
- instead, implicit decl [link]
- so i basically hoisted
- 2 2

- how about ruby
- two ways of looping
- low level

```ruby
closures = []
for i in 1..2 do
  closures << lambda { puts i }
end

closures[0].call
closures[1].call
```

- what do?
- 2 2
- guess implicitly declares at function/script level too?
- idiomatic way is using higher order method on range:

```ruby
closures = []
(1..2).each do |i|
  closures << lambda { puts i }
end

closures[0].call
closures[1].call
```

- different now
- now i is param to closure (block) passed to `each()`
- each call to block binds new params in new callframe, so clearly separate vars
- prints 1 2

- if language has higher level looping structure like `foreach` in c#, `for-of`
  in js, or `for-in` in dart
- very natural to treat each iteration as separate var
- clear evidence that's what users expect
- dig around stackoverflow find lots of questions where users confused when
  langs don't do that
- in particular, c# originally treated all iter of foreach as same var
- was common enough source of bugs that took rare step of breaking change to
  fix

- c-style loop harder
- increment clause does look like mutation, which implies one single var for
  all iters
- but not really *useful* to do that
- better answer probably do what js does with `let` and treat as new var each
  time

- already impl in jlox and clox
- does create same var for entire loop
- create one scope for entire loop instead of pushing and popping one for body
- makes easier to impl increment clause
- but means same var each time

</div>
