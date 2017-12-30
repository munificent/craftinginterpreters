^title Chunks of Bytecode
^part A Bytecode Virtual Machine

- have complete lox impl
- why aren't we done?
- rely on jvm to do lots of things for us
- want to learn how it does what it does
- learn this in coming chapters

- other main reason: jlox is slow
- could try to optimize each little piece of hit, but will hit wall
- fundamental arch is wrong model -- slow by design
- can't optimize way out any more than can optimize stage coach into jet fighter
- need to rethink entire model
- this chapter introduce that model -- bytecode-- start building skeleton of new
  lox impl, clox

- by end, be able to represent program that contains number literals
- not very impressive, have to start somewhere

## why bytecode?

- why bytecode
- one way to answer, why not other?

### why not tree-walk?

- already wrote it
- data very close to parser structure, so producing tree is trivial :)
- runs anywhere we can compile interp :)
- uses lot of memory -- lots of overhead, object headers, and pointers
- show memory layout of simple expr tree with vtables
- talk a bit about virtual/interface dispatch
- locality and cache misses
- really slow
- lot of dispatch and other code surrounding actual operation

### why not compile to native code?

- fast :)
- what does machine code look like?
- brief intro
- series of binary encoded instructions
- no explicit tree-like structure
- constants like numbers stored directly inline
- each instruction does one very low level thing
- operands are (simplified) found in registers or on stack
- code before instruction puts operands where need to be
- **todo: illustrate**

- code is dense, sequential, does exact operations and nothing more
- arch specific -- not portable
- lot of work, complex, lots of historical baggage
- register alloc, pipelining, instruction scheduling
- hard!
- [no jit on some platforms]

### what is bytecode?

- bytecode halfway between machine code and tree-walk interp
- not perfect, but gets much of benefits of each
- simple structure, easy to produce during parse
- portable
- dense, sequential, compact, not a lot of overhead
- fast
- (not as fast, emulation layer adds overhead)
- machine code-ish, but simpler, cleaner, little more abstract
- like machine code for ideal machine wish we had when generating code
  during parse

- only thing machine code has is perf (mem and speed)
- how get that without sacrificing benefits of interp and taking problems of
  machine?
- to get perf, need dense sequential binary rep
- sort of like machine code
- but imagine arch designed to be as easy to compile to as possible
- so simple can generate code during parse
- instructions close to primitive ops for lang
- where "addi" in x64 only adds integers, "add" in ours is lox-level add
  that handles number and strings, runtime type checking
- where data inst in x64 deal with registers, stack, raw addresses ours
  deal with local and global vars

- classically, each inst one byte long, "bytecode"

- don't have real chip for that imagined machine code
- in addition to writing compiler that parses lox and generates code
- write emulator for it
- just like emulator for nes or something
- except nes actually existed, our machine never did
- hence "virtual machine"
- program that decodes binary instructions
- for each one, performs it right then
- so emulator of "add" does runtime type checking, adds num and string

- since emulator written in c, our lang run on anything can compile c
- portability of interp
- [initial motivation for bytecode, p-code, et al]
- perf of machine code
- not all perf
- emulation layer adds quite bit of overhead
- still way faster than tree-walk

## getting started

- before can exec lox code in clox, need three pieces
  - define bytecode instruction format and representation
  - compiler to take lox source and compile to bytecode
  - vm that can interpret bytecode
- with jlox, did ast early and then went from there
- sort of go out from middle
- similar here
- start with bytecode format
- this chapter define and create data structure to represent *chunk* of bytecode
- not full instruction set
- add new inst each chapter for each lang feat
- just constants here
- won't compile it -- hardcode example one
- can't run it yet
- will write disassembler so we can see if it works
- useful later for debugging

- starting brand new program
- get out text editor and trusty c compiler

^code main-c

- from tiny seed grow whole vm

- c gives us little, so this part more utils
- start with little header

^code common-h

- some types and constants use throughout interp
- also, want to use nice c99 bool and sized int types, so include here

## chunk

---

- bytecode series of instructions
- each instruct is one byte op code, followed by operands
- op code is "type", what kind of inst
- operands vary
- some ops have no operands, some multiple etc.

**todo: illustrate instruction**

- next chapter learn lot more about how inst exec
- for now, just one instr

^code chunk-h

- will be instr to exit from fn
- in:

```lox
return;
```

- not useful
- start somewhere
- very simple instr

## dynamic array of code

- chunk of bytecode is sequence of instructions

^code chunk-struct (1 before, 2 after)

- struct is wrapper around array of bytes

- problem: compiler doesn't know how big array should be ahead of time
- need sequence can dynamically grow
- cs 101 would say linked list
- remember: key goal of bytecode is dense, linear encoding
- don't want to hop pointers

- instead, one of fave data structures
- dynamic array
- [same data structure as java arraylist]

- like saying vanilla favorite ice cream, but really great
- simple to impl correctly
- cache friendly
- constant perf for index and append
- [aside on amortized analysis]

- basic idea is alloc array on heap of certain size
- if fill that up and need more room, alloc new larger array
- copy over, remove old one, add

- don't want to have to realloc and copy on every single append
- so when alloc, allocate some extra space
- distinguish between "count" -- number of used elements in array, and
  "capacity", number of allocated

^code count-and-capacity (1 before, 1 after)

- initialize new chunk using fn

^code init-chunk-h (1 before, 2 after)

- starts off empty

^code chunk-c

- to add instr to chunk, use:

^code write-chunk-h (1 before, 2 after)

- impl

^code write-chunk

- easy case is if cap > count
- literally just store byte in array

- before can add char to array, needs to make sure have enough capacity

- if not, need to figure out how big array should be, then grow it to that
  size

- rely on couple of low level utilities for mem
- [in c now, so have to manage ourselves]

^code chunk-c-include-memory (1 before, 2 after)

- new file looks like

^code memory-h

- growcapacity answers how big new array should be
- given current capacity, says what new capacity is
- (assumes only need enough room for one more element)
- dynamic array needs to grow by multiple of current capacity
- 2x or 1.5x typical
- [aside on amortized complexity]
- we do two
- also need to handle edge case where capacity is zero
- to avoid extra churn on first couple of writes, jump straight to eight instead
  of 1, 2, 4
- little micro-opt
- trading off little wasted space in tiny cases for fewer reallocations

- to allocate or realloc, use grow_array
- macro is sugar around fn call to handle passing in type of array element and
  getting its size
- real fn is reallocate

^code memory-c

- not much yet
- just forwards to c realloc
- realloc() will update allocated size of existing block if it can
- if not, it will allocate new block, copy over existing stuff, and free old
  one
- just what we need

- seems pointless to wrap in our own fn
- when we add gc, going to get a lot more code here

- can create chunk, write to it, what's left?
- free

^code free-chunk-h (1 before, 1 after)

- impl

^code free-chunk

- deallocate memory, then call init() again to zero out fields so know it's
  deallocated
- use new macro

^code free-array (1 before, 2 after)

- again, just helper for call to realloc to handle element size
- to "free" means to resize array to zero elements
- realloc() smart enough to handle that
- [hardcore mode for part iii would have us implement own realloc() from
  scratch]

- now have api to create chunks and write instrs to them
- can only have series of return instructions...
- would be nice if could have chunk to return something
- need value

## disassembly

- let's try it out
- temp code in main

^code main-include-chunk (1 before, 1 after)

- hand create chunk

^code main-chunk (1 before, 1 after)

- how know if work?
- can't run it

- write disassembler
- takes binary bytecode rep prints to textual assembly-like form
- invaluable for debugging internals of compiler and vm
- after hand-create chunk, disassemble it:

^code main-disassemble-chunk (2 before, 1 after)

- again new module

^code main-include-debug (1 before, 2 after)

- header is

^code debug-h (1 before, 1 after)

- fn call above to disassemble entire chunk
- also have fn to dis single instr
- use that later in vm

^code debug-c

- main fn just prints diagnostic name passed in, then iterates and dis inst
- way we iter little funny
- disinst takes byte offset into chunk of which inst to show
- returns offset of next instr
- as see later, instr can have different sizes

^code disassemble-instruction

- first print byte offset of instr
- [useful especially later when add flow control and start jumping to offsets]
- get code from chunk
- get byte at offset, that's op code
- see which op code it is
- only have one now
- will revisit this switch each time add new instr
- return 0; never get reached unless bytecode malformed
- needed to make c compiler happy

- OP_RETURN is simple instr
- use utility fn to dis

^code simple-instruction

- just prints opcode name
- more complex inst do more interesting stuff

- now run

```
== test chunk ==
0000 OP_RETURN
```

- not exciting, but getting there
- sort of "vm hello world"
- have api to create binary bytecode data
- then circle back and turn back into text we can comprehend
- shows that we did encode and decode instr correctly in byte array

## constants

- have bare minimum
- can store code in chunks, but what about data?
- many vals produced at runtime by operations
- in 1 + 2, 3 does not exist in code
- but 1 and 2 do
- constants or literals
- not running code yet, but start think about how values rep

### values

- for now, simple
- only support numbers

^code value-h

- put in own file because going to grow in later chapters
- wrap in typedef because "Value" get more interesting later

- if compiling code like:

```lox
return 123;
```

- where "123" stored?
- for simple like ints or float, many instrs put right in code after instr
- "immediate"
- things like strings or compound need to live elsewhere
- in machine code, have const data section of executable
- separate blob of read-only where compile time data stored
- then instruction for producing literal can say, "get from const section here"
- [jvm "constant pool"]

### constant array

- do something similar
- for lox, put all constants there, even number literals
- [don't have to worry about alignment and padding]

- chunk have code and also array of constants that that code refers to
- when compile, every literal become constant in that table
- then instr for literal loads constant from table
- table simple array of values

- like code, don't know how big array should be while compiling
- dynamic array
- no generics in c, so write dedicated ValueArray type
- [won't need too many more of these]

^code value-array (1 before, 2 after)

- like code, struct wraps pointer to array along with allocated size and number
  used
- same three fns to manipulate
- [could do some kind of preprocessor macros to reuse dyn array stuff]

^code array-fns-h (1 before, 2 after)

- init makes empty array

^code value-c

- write adds new value to end of array
- [don't need operations like insert, remove, thankfully]

^code write-value-array

- like above, grows first
- now see why macros
- then free releases mem and zeroes out

^code free-value-array

- now can give chunk array of values to store constants

^code chunk-h-include-value (1 before, 2 after)

- add field

^code chunk-constants (1 before, 1 after)

- make sure to init when init chunk

^code chunk-c-include-value (1 before, 2 after)

- ...

^code chunk-init-constant-array (1 before, 1 after)

- likewise free

^code chunk-free-constants (1 before, 1 after)

- add helper fn to add constant to given chunk

^code add-constant-h (1 before, 2 after)

- compiler will call when compiling const

^code add-constant

- adds const to array
- returns index

### constant instruction

- with that, can define instruction to load constant from array

^code op-constant (1 before, 1 after)

- instr not enough, need to know *which* constant
- many instrs have one or more **operands**
- data following op code
- each op code has "instruction format" determines operands and sizes
- OP_RETURN has none
- OP_CONSTANT has one
- single byte identifies index in constant array of constant to load

- try out

^code main-constant (1 before, 1 after)

- add constant to chunk
- then right const op followed by operand
- fail if run, need to add to dis

^code disassemble-constant (1 before, 1 after)

- new instruction format, new helper fn

^code constant-instruction (1 before, 1 after)

- more complex because have to deal with operand
- read constant index operand from code
- note also return `i + 1` now to tell dischunk to advance past operand

- in addition to operand index, useful to show value
- need way to print lox value
- don't get for free since we define val rep
- add ourselves
- will be in value module

^code debug-include-value

- in value header, declare

^code print-value-h (1 before, 2 after)

- define

^code print-value

- only numbers now, so easy
- get more complex later
- ["%g" is short fmt string, shows integer-value floats without decimal]

## line info

- chunk holds almost all info we need now
- need lots more instr, but basic there
- one missing piece
- when runtime error occurs, show user where in program -- line number
- in jlox, accessed by storing tokens in ast
- tokens stored line number

- in clox, at runtime, working with chunks
- need line number in there

- runtime error can occur in most instr, so for any instr in chunk, need to
  get line number of source code instr was compiled from
- lots of potential ways to store
- do absolute simplest thing here
- parallel array of ints
- for each byte in code array, have corresponding int in line info array
- if runtime error occurs, look up line at same index as current instr in line
  array
- [yes, means debug info 4x larger than actual code, see challenges]

- could weave into code array
- have "line" instr that says "ok, current line is now x"
- insert those whenever compiler advances to next line
- means interp would have to execute those, or at least skip past them
- slows it down

- line info very rarely needed -- only when failure occurs
- want to keep out of main execution path
- hence separate array

- add another array to chunk

^code chunk-lines (1 before, 1 after)

- don't need separate count and capacity
- parallel with code, so uses same

- then every thing we do with code, do with lines
- init

^code chunk-null-lines (1 before, 1 after)

- free

^code chunk-free-lines (1 before, 1 after)

- when write code byte, also need to pass in line to write

^code write-chunk-with-line-h (1 before, 1 after)

- then update both buffers

^code write-chunk-with-line (1 after)

- when code array is resized, resize line info too

^code write-chunk-line (1 before, 1 after)

- finally, write element to line array

^code chunk-write-line (1 before, 1 after)

### testing it

- since added param to write, need to fix up test code to pass in made up
  line nums

^code main-chunk-line (2 before, 2 after)

- when dis, really helpful if can see line number for each instr so we can
  mentall map back to source
- show that

^code show-location (1 before, 1 after)

- often, single line of code compile to many instrs, so use "|" to visually
  join
- now get

```
== test chunk ==
0000  123 OP_CONSTANT         0 '1.2'
0002    | OP_RETURN
```

- so have three byte chunk
- first two bytes are const instr that loads `1.2` from constant pool
- third byte (at offset 0x2) is return instr
- both instrs on line 123

- in later chapters, define new opcodes
- but now have all we need to completely represent executable code
- remember big pile of separate ast classes each with own fields?
- in clox, reduce down to struct with three arrays: bytes of code, constants,
  and debug line info
- reduction is key piece of performance story
- next start bringing this data structure to life

---

When we first talk about compiling Lox, explain that "compiling" doesn't always
mean "statically typed". See:

http://akaptur.com/blog/2013/12/03/introduction-to-the-python-interpreter-4/

--

One way to look at bytecode is a really dense serialization of the AST.

--

One way to look at compilation is that it separates out the behavior of a
program into two halves. The static stuff is the same no matter how the code
executes. The dynamic stuff is only known at runtime. A compiler then does all
the static work once, before the code is run.

See chapter 6 of Lisp in Small Pieces.

---

- challenge to do span-based line info?
- resizable dynamic array
  - amortized constant complexity
- constant table
- challenge to implement realloc()?

- art to designing a bytecode
  - fewer lower level instructions keeps interp loop small and fast
  - too low level can waste too much time on dispatch
  - stack based simpler, does more stack churn, more instructions, smaller inst
  - register based more complex, bigger denser inst
  - designing two languages now
  - nice thing is bytecode is impl detail, so free to redesign without breaking
    users

