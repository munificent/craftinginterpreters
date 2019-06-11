^title Calls and Functions
^part A Bytecode Virtual Machine

> Any problem in computer science can be solved with another level of
> indirection. Except for the problem of too many layers of indirection.
>
> <cite>David Wheeler</cite>

- big chapter, lot to get through
- add functions, but not useful if can't call
- add calls, but not useful if nothing to call
- both

- supporting fn calls large structural change in vm
- vm has stack already, partway there
- but no notion of *call* stack
- big three pieces in this chapter
  - compiler support for compiling function declarations
    - runtime representation for function object
  - function calls
  - vm support for call stack

- as much as possible, try to build up program in little pieces
- try to keep you in runnable state as much as possible
- but all three of these pieces depend on each other
- so need patience in this chapter
- will have to get through lot of code before get back to something that
  compiles and runs
- but once it does, much more powerful!

## Function Objects

- most interesting change around call stacks, but first write some code
- can't do much without having fns, start there
- from vm pers, what is fn?

- contains executable code, which means bytecode
- could compile entire program to one monolithic chunk
- each fn would have pointer to where its bytecode begins
- would mean that have single constant pool for entire program
- end up needing larger operand for constants since more of them
- instead, think cleaner model is give each fn own chunk

- will need some other metadata too, so put in now
- struct

^code obj-function (2 before, 2 after)

- fns first class, so struct is also lox obj
- arity is number of parameters fn expects
- used to check that right number of args passed
- store name so we can show it in runtime errors
- then chunk of code for body of fn and constants in it
- need incl

^code object-include-chunk (1 before, 1 after)

- as with other obj types, have some stuff to work with them
- ctor

^code new-function-h (3 before, 1 after)

- impl

^code new-function

- creates new empty fn
- instead of passing in things like name and arity, leave blank and caller
  fill in later

- runtime obj rep needs to track what kind of obj
- so add new enum case

^code obj-type-function (1 before, 2 after)

- when done with fn, can free it

^code free-function (1 before, 1 after)

- responsible for freeing memory fn obj owns and then fn itself
- in this case, mainly chunk

- [don't need to worry about freeing name. since name is lox obj, gc will handle
  it once add gc]

- in fn to print obj, need to handle fn type

^code print-function (1 before, 1 after)

- show fns name to make things little easier for user
- so if do:

```lox
fun someFunction() {}

print someFunction;
```

- prints "<fn someFunction>"

- couple of macros for doing type tests and conversions

^code is-function (2 before, 1 after)

- returns true if given value is fn obj
- then, once know have fn, can cast value to that type

^code as-function (2 before, 1 after)

- ok have working fn
- got warmed up, time for something harder

## Compiling to Function Objects

- compiler now assumes compiling to only one chunk
- each fn has own chunk
- when fn declaration begins compiler needs to handle switching to new chunk
  for fn
- then at end of fn, go back to previous chunk
- since fns just declarations, can nest arbitrarily deeply too
- so compiler needs to handle tracking which chunk

- alsonot all code is inside fn, though
- unlike c, top level of lox script contains imperative code
- so still need to handle top level code
- before get fn decls working, reorganize compiler to handle this

- to simplify, will wrap top level code in own implicit fn
- interpreter runs program by automatically calling that fn
- that way, compiler always inside some fn
- tracks it:

^code function-fields (1 before, 1 after)

- need type to distinguish when compiling top level code from code inside fn
  decl

^code function-type-enum

- two are mostly the same, but some differences
- ex: can't have return statement at top level
- this lets compiler know context

- most compiler code writes to chunk being compiled
- gets at chunk through `currentChunk()` helper
- put that there as indirection so that we could change "current" chunk without
  having to rewrite a bunch
- now:

^code current-chunk (2 before, 2 after)

- current chunk is always chunk owned by fn currently compiling

- next, need to create that fn
- right now, don't have fn decls, so just need one for top level code
- in entrypoint fn

^code call-init-compiler (1 before, 2 after)

- bunch of changes in how compiler gets init
- first clear out new fields

^code init-compiler (1 after)

- then create fn obj

^code init-function (1 before, 1 after)

- [weird to null fn field only to initialize a bit later. need for gc.]

- seems strange
- fn is runtime representation of fn object, but creating at compile time
- sort of bridge compile time and runtime world
- similar to how we handle other literals like strings
- fn basically *is* literal
- fn decl is syntax that creates fn object
- so compiler creates fn objects during comp
- at runtime, just invoke them
- [can do this because fn obj just contains code, name, and arity
  - all of that known at compile time and constant
  - when have closures (link) get more complex]

- another strange thing

^code init-function-slot (1 before, 1 after)

- recall locals array used by compiler to track which stack slot associated
  with which local variable or temporary
- from now on, compiler pre-emptively claims slot zero for special use
- trust me, explain why further in

- that's init, then over in other end

^code end-compiler (1 after)

- right now, compiler is called by `interpret()`
- passes in chunk, which compiler fills in with code
- now going to change it
- instead of passing in chunk, compiler will create own fn obj for top level
  code and return it
- grabs fn from global compiler struct

^code end-function (1 before, 1 after)

- then returns it

^code return-function (1 before, 1 after)

- while in here
- have some diagnostic code in compiler to print generated bytecode so we can
  debug
- need to fix up to handle chunk being in fn

^code disassemble-end (2 before, 2 after)

- need to adjust main `compile()` fn that is entrypoint to compiler to handle
- adjust signature in head

^code compile-h (2 before, 2 after)

- instead of taking chunk, returns fn
- likewise in defn

^code compile-signature (1 after)

- then in impl

^code call-end-compiler (4 before, 1 after)

- get fn from compiler
- if compiled without error, return it
- otherwise, signal compilation error by returning null
- eventually, we'll update `interpret()` to call this, but have to do other
  stuff first

## Call Frames

- ready for big conceptual leap
- before can impl fn decls and calls, need to get vm ready to handle them
- two main problems need to worry about

### allocating locals

- compiler allocates stack slots to locals
- how does that work when locals dstribu across multiple fns

- one option would be to keep totally separate
- each fn gets own dedicated set of slots
- slots never used for any other fn ever
- sort of like declaring every local var `static` in c
- each local in entire program has own bit of memory it owns forever

- believe it or not, early pl did this
- first fortran compilers worked this way
- compiler basically statically alloc memory for all locals

- obv problem really wasteful
- have memory set aside for local even when fn not being called
- more fundamental problem, though, recursion
- can be "in" two or more calls to same fn at same time
- when that happens, each fn needs own copy of its locals
- [early fortran didn't support recursion specifically for this reason.
  recursion was considered advanced feature at the time]

- could solve this by dynamically allocating locals as needed
- but don't want perf hit of doing full heap alloc for each local variable when
  comes into scope
- instead, do mixture of static and dynamic alloc

- even with fns in mix, locals still conveniently have stack semantics
- consider

```lox
fun first() {
  var a = "a";
  second();
  var b = "b";
}

fun second() {
  var c = "c";
  var d = "d";
}

first();
```

- as exec flows through fn calls, locals still behave in lifo
- so can still alloc locals on stack

**todo: illustrate flow**

- ideally, still want to know *where* on stack at compile time
- for perf, want to do as little computation as possible
- in above example, c ends up in slot 1 and d in slot 2, but not always case
- consider:

```lox
fun first() {
  var a = "a";
  second();
  var b = "b";
  second();
}

fun second() {
  var c = "c";
  var d = "d";
}

first();
```

- in first call to `second()`, go in slots 1 and 2
- in second call, need to make room for b on stack, so go in 2 and 3
- compiler can't pin down exact slot for each local in fn
- but note that *relative* slots for c and d always same
- key insight

- when call fn, don't know where top of stack is because can be called in many
  different contexts
- but wherever that top happens to be when fn call begins, know exactly where
  locals are relative to that
- so, like many prob, solve with level of indirection
- for each fn call, vm stores first slot on stack where fn's own slots begin
- locals and temporaries inside fn accessed relative to that
- compiler calcs offset
- vm at runtime adds location of first slot to get real slot
- first slot called "frame pointer" or "base pointer" in cpu

- so first piece of data
- for each fn in process of being executed by vm, keep track of address or
  offset of first stack slot used by fn

### return addresses

- right now vm executes instrs one at a time by incrementing ip
- only interesting logic control flow where ip offset directly
- fn calls pretty easy: set ip to point to first instr in fn's chunk
- what about when fn is done?
- need to return back to chunk called fn from and pick up at instr right after
  call

- for each fn call, need to store where called from
- called "return address" because address of instr that will return to
- note not property of fn itself, property of fn call
- because of recursion, same fn may be in middle of multiple calls simul
- need to track for each fn invocation

- [self-modifying code]

### call stack

- so for each fn call vm is exec need struct to track data about call
- looks like

^code call-frame (1 before, 2 after)

- [most native archs do not use separate stack to track call frames
  instead return address and frame pointer of caller stored directly on value
  stack]

- each call frame reps one ongoing fn call
- little different from how described above, but accomplishes same goal
- instead of storing return address in callframe for callee, callers stores own
  ip
- when return, jump to ip in call frame of fn returning to
- also store fn itself, so we can look up things like constants
- then store pointer to first slot on stack fn can use for locals and temps
- refer to as "slots" because will treat it like array
- [c treating pointers and arrays as similar weird but handy here]

- each time fn called, need to allocate one of these
- could dynamically allocate those
- [many lisp impls do because of call/cc]

- as usual, try to avoid heap alloc for perf
- fn calls happen all the time, need to be fast
- fortunately, same obs as locals: fn calls have stack semantics
- if `first()` calls `second()`, `second()` must complete before `first()` does
- [not true if lang has coroutines]

- so vm can allocate array of these up front and treat like stack

^code frame-array (1 before, 1 after)

- to keep clox simple, just have fixed max size
- `frameCount` is current size of stack

^code frame-max (2 before, 2 after)

- means lox program has max call depth
- [many langs work this way. most lisps do "tail call optimization" to avoid
  using too many for deeply recursive fns. some langs can grow stack
  dynamically.]

- when create vm, starts empty

^code reset-frame-count (1 before, 1 after)

- grungy work going through bytecode interp and using call frames
- every time read or write value from stack, need to change to relative to
  current call frame's stack pointer
- likewise, vm no long has single ip
- instead, need to use ip from current call frame
- start at top and grind way through

^code run (1 before, 1 after)

- first store current call frame in local variable
- [could read vm array each time, but encourages c compiler to store current
  frame in register, which likely helps perf]
- replace macros for reading operands with ones that read from call frame's
  ip

- into instrs

^code push-local (2 before, 1 after)

- to read local, treat call frame's stack pointer as array
- operand has slot relative to that
- same for set

^code set-local (2 before, 1 after)

- jump instrs that modify ip do it for current frmae

^code jump (2 before, 1 after)

- conditional jump

^code jump-if-false (2 before, 1 after)

- backwards jump

^code loop (2 before, 1 after)

- also need to fix up diagnostic code for debugging the vm

^code trace-execution (1 before, 1 after)

- instead of hardcoded chunk and single ip, reads ip and chunk from call frame

- not too bad actually
- last bit is code that calls into main `run()` fn

^code interpret-stub (1 before, 2 after)

- wire up new compiler
- call compiler to get top level code compiled to a fn
- if returns null, there was a compile error, so return that and don't try to
  run anything

- otherwise, create initial call frame to invoke synthetic fn for running
  top level code
- contains fn for code
- initialize ip to point to first instr in chunk
- then point call frame's slot pointer to bottom of stack

- then at end, don't need to worry about freeing hardcoded chunk any more
  since owned by fn

^code end-interpret (2 before, 1 after)

- after that, ready to run!
- if did everything write, clox now does... exactly what it did before
- but go infra in place for fns and calls

## Function Declarations

- before can call, need some fns to call so decl first
- start with keyword

^code match-fun (1 before, 1 after)

- calls

^code fun-declaration

- fns just first-class values stored in named vars
- so parse name like any other var decl
- at top level, creates global
- inside block or other fn, creates local var
- last chapter, explained two stage init for vars
- ensured can't access var inside own initializer
- fns don't have problem
- safe to access fn inside own body because by time fn invoked, fully defined
- in fact want to enable this to allow recursion on local fns
- so mark name initialized immediately before compiling body

- to parse and compile function itself -- param list and body -- use helper
- that generates code that will leave fn object on stack
- then call definevar to emit code to store that fn obj in named variable for
  fn

- use separate helper because will use it later for compiling method defns
- start simple

^code compile-function

- for now, no parameters
- just parses empty parens followed by body
- body starts with brace
- then call block() which knows how to compile rest of block including closing
  curly

- interesting part is compiler stuff
- compiler struct stores things like what slots owned by what local vars
- how many blocks of nesting currently in
- all of that specific to single fn
- how to handle compiling multiple fns simultaneously?
- [remember even top level code compiled in implicit fn, so always inside at
  least one fn]

- trick is create separate compiler struct for each one
- when begin compiling fn, create new compiler on c stack
- initcompiler sets that to current one
- so all other compiler fns then compile into new compiler
- after fn body, call end compiler to get resulting fn obj
- then store that in constant table of surrounding fn's chunk

- but how do we get back to surrounding fn?
- when set new fn as current, lost it
- fix that by treating compiler structs as stack
- each compiler points to surrounding compiler in linked list all the way back
  to top

^code enclosing-field (1 before, 1 after)

- when create new compiler, remembers pointer to current one

^code store-enclosing (1 before, 1 after)

- then when innermost compiler ends, restores previous current

^code restore-enclosing (4 before, 1 after)

- because compiler uses recursive descent and compiler structs stored right on
  c stack, don't even need to dynamically alloc
- [does mean amount of function nesting practically limited by depth of c stack
  deeply nested fns could crash compiler. maybe good to set
max depth and check on recursive calls.

- couple loose ends
- fn not very useful without params

^code parameters (1 before, 1 after)

- semantically, param just local variable in outermost scope of fn
- use existing compiler code to declare named local
- but no code to initialize value, get to that later with fn calls
- also keep track of fn's arity by noting how many params parsed

- last piece of data in fn obj is fn name
- call initcompiler right after parsing name, so it can grab it from previous
  token

^code init-function-name (1 before, 1 after)

- creates copy of it because remember lexeme just points into source string
- fn obj exists at runtime after compilation done, so need new heap alloc
  string with longer lifetime

- ok, can now compile fn decls

```lox
fun areWeHavingItYet() {
  print "Yes we are!";
}
```

- just can't call them

## Function Calls

- making good progress
- don't think of it this way, but fn call expression really infix op
- have expression that evals to fn on left
- [high precedence, so usually lhs is just identifier or result of other fn]
- then `(` in middle, followed by args and `)`
- so hook into pratt parser as infix op

^code infix-left-paren (1 before, 1 after)

- calls

^code compile-call

- already consumed `(`, so then compile arguments
- returns number of args in call
- after that, emit new call instruction using count as operand
- use helper because will also use for compiling arg lists in method and super
  calls

^code argument-list

- seen similar code in jlox
- keeps chewing through args as long as we find commas
- then consumes final closing paren

- also put limit on how many args can pass to fn

^code arg-limit (1 before, 1 after)

- put limit in in jlox, now see why
- two real limits in vm
- 1-byte arg count operand in call instr, so only up to 255
- only 256 local slots, and slot 0 already claimed so really 255
- assuming not too many args, emits new instr

^code op-call (1 before, 1 after)

- head over to vm
- before look at code, think about state of stack and what need to do
- to call fn, need to get fn obj being called
- create call frame
- also need to collect args and bind to fn's parameters
- want to do that as fast as possible

- when compile call, compile fn first
- usually just identifier expr when eval leaves obj on stack
- so fn obj sitting on stack
- then compile args in order, so args on top

- finally emit call instr which begins call

- for ex:

```lox
fun addThree(a, b, c) {
  print a + b + c;
}

addThree(1, 2, 3);
```

- at point that exec call instr, stack looks like:

**todo: illustrate**

- when compiled `addThree`, compiler alloc local slot zero
- then after that, one slot for each param
- so when start exec body, want stack slots for fn to look like:

**todo: illustrate**

- need call frame with pointer to first slot
- then need arg values in slots 1 - num params
- can think of call frame's stack pointer as taking window of entire stack
  and given fn that window into stack
- stack pointer points to first slot in window
- other end of window grows and shrinks as fn allocs locals and temps

- code that calls fn allocs temp slots to store each arg in
- after call, no longer needs them
- because temps always at top of stack above locals, know that caller won't
  be using any slots above arg temps
- so can do clever trick: overlap the windows

**todo: illustrate**

- when call fn, set up new call frame
- stack pointer point to just before first arg already on stack
- that way param slots exactly line up with where arg value already stored
- don't need to do any copying to bind arg to param!
- means callee's window overlaps callers
- ok, because caller won't do any work until callee returns
- and know that caller doesn't have any useful data in stack above args since
  those were most recently pushed

- impl

^code interpret-call (3 before, 1 after)

- need to know how many args passed to fn to know how much stack windows
  overlap
- that's why put in bytecode operand which read here
- actual set up happen in helper fn
- some calls can cause runtime error, so returns false if call not successful
- in that case, interp exits with error

- if successful, will have new frame on callframe stack
- interp caches current frame in local, so need to update

^code update-frame-after-call (2 before, 1 after)

- since interp pulls ip from this frame, when bytecode dispatch loop circles to
  next instr, jumps into body of called fn and begins exec

- to set up new call frame

^code call-value

- lox dyn type, so nothing to prevent weird code like

```lox
var notAFunction = 123;
notAFunction();
```

- compiler can't prevent so have to check type at runtime
- if value being called not fn, report runtime error and abort
- otherwise, actual call happens in

^code call

- just inits next callframe on stack
- store ref to fn being called
- set up frame's ip to point to first instr in chunk
- then set up stack pointer to give callee window into stack

- start at top of stack, which is right after last arg pushed
- subtract number of args to overlap window for params
- then subtrack one more
- this special pre-alloc slot 0 compiler set up
- contains ref to fn being called

- not super useful now
- when add methods, slot will contain object method is called on
- this

### runtime errors

- overlapping windows works perfectly because num args matches param
- user could call with too many or too few
- runtime error

^code check-arity (1 before, 1 after)

- this why store arity in fn obj
- other error more internal limitation

- call frame array has fixed size
- don't want to wander into random memory

^code check-overflow (2 before, 1 after)

- if too many call frames, runtime error
- in practice, almost always because of runaway recursion

- while on subject of runtime errors
- make more helpful for dev
- when runtime error occurs, programmers wants to know how got into bad state
- right now, just print line that error occurred on and abort
- not super helpful

- now that have call stack and fns know names, can show entire call stack
  on runtime error
- tells user not just where error occurred, but how program to go be executing
  that fn
- critical when multiple ways to reach given line
- looks like

^code runtime-error-stack (2 before, 2 after)

- after printing error message, walk call stack from top to bottom
- for each frame, find line number corresponding to current ip
- then print that and each fn name

- for ex, if run this broken prog:

```lox
fun a() { b(); }
fun b() { c(); }
fun c() {
  c("too", "many");
}

a();
```

- prints out

```text
Expected 0 arguments but got 2.
[line 10] in c()
[line 6] in b()
[line 2] in a()
[line 13] in script
```

- not bad

### calling the top level code

- now that have nice fn for initiating call, can use to set up first stack
  frame for exec top level code

^code interpret (2 before, 2 after)

- getting close
- can call fns
- but can't return from them

- have temp code in return op to exit interp
- can now have real impl

^code interpret-return (1 before, 1 after)

- when fn returns value, will be on top of stack
- grab that
- then discard call frame for fn returning
- if that last frame, means top level code done, exit interp
- otherwise, returning to caller
- update vm top of stack to first slot of fn just returned
- that right below where args were on stack
- those were temps pushed right before call, so done now
- thus stack back to what it was before call
- push return value in that new location

**todo: illustrate**

- then update cached frame to point to frame of fn returning to
- that call frame still has own ip, pointing to instr right after call
- bytecode loop will read that ip and proceed from there

- popping return value assumes fn actually did return value, but fn can
  implicitly complete by reaching end of body:

```lox
fun noReturn() {
  print "Do stuff";
  // No return here.
}

print noReturn();
```

- in that case, lang specified to implicitly return nil
- need to make that happen

^code return-nil (1 before, 2 after)

- when emit return op at end of fn body, push implicit nil return value

- almost done, just need to dis new instr

^code disassemble-call (1 before, 1 after)

- have working fn calls!

## Return Statements

- implicitly returning nil fine for some, but useful if fns can actually return
  values
- some langs implicitly return result of last expr
- lox require explicit return stmt
- do now

- new statement, start from keyword

^code match-return (1 before, 1 after)

- hand off to

^code return-statement

- return value is optional
- so check for semicolon
- if no value, implicitly return nil by emiting nil instr
- otherwise compile return value
- either way, return value on top of stack
- then emit return instr, just like implicit one at end of body

- compile error to worry about too
- lox top level code is imperative code can have arbitrary statements
- shouldn't be able to have return statement though

```lox
return "What?!";
```

- compile time error to have return stmt outside of n

^code return-from-script (1 before, 1 after)

- fns full featured now
- args, params, returns, recursion
- all working

- but user defined fns can't *do* anything
- ultimately all user code defined in terms of primitive op lang provides
- right now, only visible thing is print statement

- most langs expose functionality by providing "built in", "native", or
  "primitive" fns
- these look like normal fns to caller, but not impl in lang itself
- instead written in lower level impl lang

## Native Functions

- lox is toy lang mainly because doesn't have many
- clox doesn't have any right now
- adding more can turn into actual useful lang
- but not very educational
- seen one, seen em all
- just do one

- even for that, need lot of machinery to support
- native fns very different from existing fn obj
- no bytecode chunk to point to
- when called, don't push vm callframe because no chunk

- handle by making them entirely different obj type

^code obj-native (1 before, 2 after)

- native fn is different obj
- has typical lox obj header
- then pointer to c fn that impls native functionality
- fn gets passed the arg count and pointer to first arg on stack
- returns value that is result

- new obj type means few pieces
- declare ctor-like fn to create one

^code new-native-h (1 before, 1 after)

- impl in module

^code new-native

- takes pointer to c fn that it wraps
- allocs struct and sets type so runtime knows what kind of obj
- new enum case

^code obj-type-native (2 before, 2 after)

- when done with, need to free

^code free-native (4 before, 1 after)

- not much here since struct doesn't own any extra memory
- also need to handle printing obj

^code print-native (2 before, 1 after)

- and couple of macros use for type tests and cast from value

^code is-native (1 before, 1 after)

- after tested type, safe to cast

^code as-native (1 before, 1 after)

- now native fn are obj like any other
- can store in var, etc.
- aren't called like other fns
- instead, in vm when handle call instr, look at type and handle natives
  specially

^code call-native (2 before, 2 after)

- don't need to mess with vm call frame stack at all
- just immediately call c fn and rely on c stack
- get result value back
- then like lox fn call, discard arg slots and store result back on stack
- after that, fn done

- user has no way to create own native fns
- need to be wired directly into c code in vm
- provide to user by having them pre-defined in global vars

- add utility fn in vm to add one

^code define-native

- creates native fn obj for given c fn
- then stores in global var hash table under given name
- first, have to wrap raw c string in lox string obj

- probably wondering why tossing name and fn on stack only to pop
- weird, right?

- this is kind of stuff have to do when have gc
- we don't yet, but will soon
- creating string, native fn, and storing entry in hash all dynamically alloc
  mem
- means any of them could trigger gc
- if gc happens in middle of fn, need to make sure vm doesn't get confused and
  accidentally free string for name or native obj
- storing on stack helps vm find them
- [don't worry if confusing, make sense when add gc]

- for all that, only going to add one native
- returns elapsed exec time in seconds
- [use for profiling later]

^code clock-native (1 before, 2 after)

- bind it to global "clock"

^code define-native-clock (1 before, 1 after)

- need couple of includes for this to work

^code vm-include-time (1 before, 2 after)

- also

^code vm-include-object (2 before, 1 after)

- give try

```lox
fun fib(n) {
  if (n < 2) return n;
  return fib(n - 2) + fib(n - 1);
}

var start = clock();
print fib(35);
print clock() - start;
```

- can now not just write inefficient fib, but can measure just how ineffic
- not smartest way to calc fib, but good test of fn call perf
- on my machine, clox ~5x faster jlox
- not bad

<div class="challenges">

## Challenges

1. reading writing ip address one of most frequent oper in bytecode loop
   - right now, access through pointer to current call frame, requires indirection
   - cache ip of current fn in local var in run()
   - make sure to update correctly when call frame pushed or popped
   - how does affect perf?

2. - native fns in clox have no way of reporting runtime error
   - cannot define native that fails
   - add support for reporting runtime error from native fn

3. - add more natives to clox to do useful things
   - what did you add?
   - write some code using them?
   - how does it affect how the language feels and how useful it is?

4. - instead of separate callframe array, store data for callers directly on
     value stack

</div>
