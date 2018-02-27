^title A Virtual Machine
^part A Bytecode Virtual Machine

- have code rep, but opaque and lifeless
- know bytecode is sequence of binary instr, not much about what instr is
- hard to understand without knowing how instr is processed
- so before write compiler, write vm
- brings instr to life -- understand what instr are because know what they do

## an instruction execution machine

- hand vm chunk of code and it runs it
- in clox, vm is separate struct, separate module

^code vm-h

- start simple
- stores chunk of code it's executing
- like most objects we'll define, has fns to create and destroy one

^code vm-c

- not much yet
- no interesting state to init/free
- one interesting bit is module level vm variable
- c doesn't have methods with implicit "this" like java
- instead of passing vm pointer to every single fn in vm, make single global one
- [means can't have multiple independent vms, fine for book]

- go ahead and wire into main module

^code main-init-vm (1 before, 1 after)

- then when done

^code main-free-vm (1 before, 1 after)

- don't forget

^code main-include-vm (1 before, 2 after)

- clox now creates vm before building chunk
- empty shell

### executing instructions

- vm's job is execute code
- after create chunk, tell vm to run it
- [soon chunk created by compiler]

^code main-interpret (1 before, 1 after)

- main entrypoint into vm
- declare

^code interpret-h (1 before, 2 after)

- tells vm to exec given chunk
- returns enum indicating how it went

^code interpret-result

- not using yet, but when have compiler and runtime errors, tell us if
  successful or not
- impl

^code interpret

- sets up vm to run chunk, then hands off to run()
- vm zips through instrs one at time
- need to keep track of which instr currently interp
- pointer to current instr -> ip
- [every vm and real arch has register or variable for this
- also called program counter]
- start at pointer to first byte in bytecode
- declare in vm

^code ip (1 before, 1 after)

- fun happens in run

^code run

- most important fn in all of clox, by far
- will spend > 90% of exec inside this fn
- critical for perf

- conceptually simple
- outer loop runs until done executing code

- inside loop, read and execute single bytecode instr

- to exec instr, first read it out of bytecode
- use READ_BYTE macro
- looks up byte at ip, then increments ip
- [at any point in time, ip points to *next* byte to execute
- most hardware ips do same]

- since opcode is single byte, also works to read opcode of current instr
- each instr has own semantics, so write chunk of c code to interp or emulate
  each
- process of taking numeric opcode and getting to right piece of c code that
  implements that opcode's semantics is "dispatch" or "decoding"
- have to do that for every single instruction, every single time executed, so
  one of most performance critical bits of code
- lots of research on it, lots of clever techniques
- often hand-written assembly
- [aside on direct-threaded, indirect threaded, etc.]
- most approaches either arch-specific or use non-standard c extension
- for clox, take simple, platform-indenpendent
- giant switch with case for each

- start with just OP_RETURN
- used to signal done with code, which exits run()
- [later will be used to return from lox fn, don't have fns yet]

- other instr defined so far is constant

^code op-constant (1 before, 1 after)

- can't do anything useful with it
- so for now just print constant value
- call printf(), so

^code vm-include-stdio (1 after)

- look up constant from chunk's constant table
- other instrs will also access constant table, so make macro

^code read-constant (1 before, 2 after)

- reads next byte from bytecode, which is const table index
- returns value at index in const array
- like READ_BYTE, macro only meaninful inside body of run()
- little uncommon to see macros scoped to single fn
- but run() will get so big almost little world unto itself
- good c citizen, keep macro scoped to fn

^code undef-read-constant (1 before, 1 after)

- if run now, should see it print out 1.2 constant defined in last chapter
- can see what's going on now, but only because have temp code to print constant
- real op_const won't print, will pass value to other code
- vm inner workings can be very opaque

- just like disassembler lets us debug compiler by seeing contents of chunk,
  would be useful to see instrs that vm exec
- add optional flag in common.g to enable inst tracing
- end user won't use, but helpful for us

^code define-debug-trace (1 before, 2 after)

- when on, vm prints every instr before it execs it

^code trace-execution (1 before, 1 after)

- convert ip back to relative offset by sub from start of bytecode
- use previous dis fn for logging single instruction
- need include

^code vm-include-debug (1 before, 1 after)

- have imperative side of vm, can do things
- can act, but not think -- no data

## a place to store values

- in source lang, exprs consume and produce values
- instrs need to consume and produce data

- `3 - 2`, need instrs for constants `3` and `2` and instr to sub them
- how does sub know what to sub?
- how do values flow through machine?

- consider:

```lox
fun echo(n) {
  print n;
  return n;
}

print echo(echo(1) + echo(2)) + echo(echo(4) + echo(5));
```

- wrap each subexpr in call to echo() to make order of eval visible and
  significant
- ast looks like

**todo: illustrate**

- think just about lox semantics
- arith operands must be evaluated before can perform operation, obv
- also define that lhs eval before right
- *must* output

```
1 // echo(1)
2 // echo(2)
3 // echo(1 + 2)
4 // echo(4)
5 // echo(5)
9 // echo(4 + 5)
12 // print 3 + 9
```

- in jlox, got that by traversing ast
- post-order: traverse left branch then right then perform node itself
- to compute binary expr, used local java vars to store lhs result while
  calculating rhs
- had unique java call frame for each node eval
- don't have that now

- weird exercise, step through execution of program
- keep track of lifetime of each value produced and consumed
- number is when value first produced
- line means "still need it later"
- star is when consumed by operation and no longer needed

```
const 1  -> 1
echo(1)  -> |
const 2  -> | 2
echo(2)  -> | |
add 1 2  -> * * 3
const 4  ->     | 4
echo(4)  ->     | |
const 5  ->     | | 5
echo(5)  ->     | | |
add 4 5  ->     | * * 9
echo(9)  ->     |     |
add 3 9  ->     *     * 12
print 12 ->             *
```

- some values short-lived, like 2
- others stay around longer like 3, result if outermost lhs
- has to live until entire `echo(echo(4) + echo(5))` is done
- show again but reuse columns: if value discarded free up column for other vals

```
const 1  -> 1
echo(1)  -> |
const 2  -> |  2
echo(2)  -> |  |
add 1 2  -> 3
const 4  -> |  4
echo(4)  -> |  |
const 5  -> |  | 5
echo(5)  -> |  | |
add 4 5  -> |  9
echo(9)  -> |  |
add 3 9  -> 12
print 12 ->
```

- what have here?!
- columns form a stack
- values pushed in from right when first produced
- popped when consumed by add or print

- [not revelation: jlox was recursive and java uses stack for locals]

- to take advantage of this, our vm stack-based
- temporary values live on stack
- when instr needs data, pops
- when instr produces data, pushes

### stack-based vm

- really excited
- magic trick fills you with wonder
- want to know how it works
- usually some mundane trick,
- once you know, magic gone
- but couple ideas in cs that seemed like magic and still feel that way after
  learning how they work
- stack-based bytecode vms one of them
- so simple to execute
- todo: loc for this chapter?
- as see later, dead simple to compile to
- yet still fast enough for real-world use
- feels like cheating at pl impl
- [not silver bullet: can spend lot of time dispatching instrs and manip stack
  register-based vms seem faster]

- general idea, make concrete
- in addition to chunk, vm stores stack of values

^code vm-stack (1 before, 1 after)

- implement our own stack using array
- stack grows down through array
- elem zero is bottom of stack
- top is highest
- since stack grows and shrinks, need to keep track of top
- use direct pointer instead of index -- little faster
- stackTop going to change *all the time*
- points to value just *past* top-most element
- seems weird, but all do this
- means can represent empty stack by pointing to elem zero
- pointing to array[-1] undefined in c
- [pointing one past end of array *is* defined, to allow things like this]
- think of stackTop as "address of slot where next value gets pushed"

**todo: illustrate stackTop pointer into various stacks**

- max stack size is:

^code stack-max (1 before, 2 after)

- fixed size
- [fact that it's fixed-size means we can run out -- classic stack overflow]
- could grow dynamically
- go does
- for now, keep simple
- need an include so can use Value

^code vm-include-value (1 before, 2 after)

- need to initialize stack

^code call-reset-stack (1 before, 1 after)

- calls

^code reset-stack

- since array statically-alloc, no mem operation needed
- don't need to clear cells
- if reset stackTop, implicitly discards all values past it
- point to elem zero empties stack
- super fast -- just pointer assign!

- stack protocol two operations

^code push-pop (1 before, 2 after)

- impl is simple

^code push

- to push, store into array at current top pos
- then increment to track that slot is used now

^code pop

- pop is reverse
- decrement pointer, then return element

**todo: illustrate operations and stackTop pointer**

- stack pretty opaque
- when start doing interesting instr, lot of useful state hiding in it
- make our lives easier to be able to see it while interp running
- if tracing instrs, also print constants of stack before each instr

^code trace-stack (1 before, 1 after)

- really verbose, but useful when debugging something in bowels of interp

- revisit insts to use stack now

^code push-constant (1 before, 1 after)

- last chapter was vague about "load" constant
- finally see what means to load -> push to stack

^code print-return (1 before, 1 after)

- conversely, for now, return instr will pop top of stack and print it
- do something different later, but let us get vm evaluating simple exprs and
  displaying result

## arithmetic instructions

- above is heart and soul of vm
- instr loop dispatches and execs instrs
- stack keeps track of temporary data and flows values between instrs
- stack is there, but utility not visible
- make more clear by turning vm into calculator
- add instrs for arithmetic

### unary

- start with simplest arith instr, unary negation

```lox
var a = 123;
print -a; // -123.
```

- has one operand, result is negation
- syntax is prefix "-", but not relevant here
- add new op code

^code negate-op (1 before, 1 after)

- then in interp loop, impl just:

^code op-negate (1 before, 1 after)

- needs one operand, which pops from stack
- negates that
- produces result by pushing back onto stack

- complete job by dis support

^code disassemble-negate (2 before, 1 after)

- try out

^code main-negate (1 before, 2 after)

- insert insrt in hand-auth between code to load constant and return, which
  prints result
- since negate pops one and pushes one, stack ends up still with single value
  on it
- run

```text
-1.2
```

- not super interesting
- always works with single value, so could replace stack with one slot
- to see how stack works, do binary

### binary

- have four binary arith instrs
- lay all at once
- [other languages have modulo, shift, bitwise, etc.]

^code binary-ops (1 before, 1 after)

- in bytecode loop, impl

^code op-binary (1 before, 1 after)

- only differnce between ops is which c operator they use, `+`, `-`, etc.
- lot of identical boilerplate to pull values off stack, etc.
- later when add dynamic typing, have more ceremony
- to avoid copy/paste, define macro

^code binary-op (1 before, 1 after)

- preprocessor easy to abuse -- often hard to tell difference between good and
  bad use
- be careful
- mainly using here to avoid redundant code in later chapters of book when
  need to amend each operator

- couple of macro tricks
- to have macro expand to multiple statements in safe way, wrap in do while
  false block
- execs body exactly once
- [safer than {} consider:
    #define macro() { one(); two(); }

    if (sometime) macro();
    else nope();

  see what ";" does]

- did know can pass operator to macro?
- preprocessor doesn't know diff between operator and identifier
- all just tokens

- actual behavior straightforward
- need two operands, so pop both
- calc result
- push it

- note order that popped matters, at least for "-" and "/"
- operands evalled left to right by previous code
- so will be pushed left first then right
- so left below right on stack
- thus when pop, pop right first then left
- pushing and popping sequence reverses it
- undo reversing, by popping and storing b then a

**todo: illustrate**

- again clean up mess

^code undef-binary-op (1 before, 1 after)

- and add to dis

^code disassemble-binary (2 before, 1 after)

- instrs have no operands (arith expr does, instr itself does not)
- try out by adding little more code to chunk

^code main-chunk (1 before, 2 after)

- push two constants
- then pop, add, push result
- which then negated
- note how arith operations compose naturally
- negate doesn't care that thing negating is now result of add
- just gets it from stack
- stack decouples instrs that produce values from those that consume

- in example here, stack only gets two deep
- could hand-build chunk that uses much more stack
- push dozens of constants, then merge them all together with series of arith
- play around with different instr orders to get feel for it
- last we'll have to hand-author chunk
- when next work with chunk, have compiler

**todo: challenges**

**todo: note on bytecode format**

- art to designing a bytecode
  - fewer lower level instructions keeps interp loop small and fast
  - too low level can waste too much time on dispatch
  - stack based simpler, does more stack churn, more instructions, smaller inst
  - register based more complex, bigger denser inst
  - designing two languages now
  - nice thing is bytecode is impl detail, so free to redesign without breaking
    users
