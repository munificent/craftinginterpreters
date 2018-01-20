^title Chunks of Bytecode
^part A Bytecode Virtual Machine

We already have ourselves a complete implementation of Lox in jlox, so why isn't
the book over yet? Part of this is because jlox relies on the JVM to do lots of
things for us. We can't claim to understand how an interpreter works all the way
down to the <span name="metal">metal</span> if our interpreter sits on top of
the fairly lofty Java platform.

<aside name="metal">

Of course, even our new C-based interpreter relies on the C standard library for
fundamental operations like memory allocation. And we use the C compiler to free
us from details of the underlying machine code we're running it on. Heck, even
that machine code is probably implemented in terms of microcode on the chip. And
the C runtime relies on the operating system to hand out pages of memory. But I
guess we have to stop *somewhere* if this book is going to be able to fit on
your bookshelf.

</aside>

We want to learn how the JVM does what it does and implement those basic
services and data structures ourselves. We'll be doing that a little here and
even more in the coming chapters.

An even more fundamental reason that jlox isn't enough is that it's too damn
slow. A tree-walk interpreter can be sufficient for some specific high-level,
declarative little languages. But for a general-purpose, imperative language --
even a "scripting" language like Lox -- it's brutally slow.

Take this little <span name="fib">script</span>:

```lox
fun fib(n) {
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}

var before = clock();
print fib(40);
var after = clock();
print after - before;
```

<aside name="fib">

This is obviously an inane way to actually calculate Fibonacci numbers. Our goal
is to see how fast the *interpreter* runs, not to see how fast of a program we
can write. A slow program that does a lot of work -- pointless or not -- is a
good test case for that.

</aside>

On my laptop, that takes jlox about 72 seconds to execute. An equivalent C
program finishes in half a second. (I will grant that the jlox run is better at
keeping my legs toasty warm.) Our dynamically-typed scripting language is never
going to be as fast as a statically-typed language with manual memory
management, but that doesn't mean we need to settle for being more than *two
orders of magnitude* slower.

We could take jlox and run it in a profiler and start tuning and tweaking
hotspots. But that's only going to get us so far. The execution model -- walking
the AST -- is fundamentally the wrong architecture. We can't micro-optimize that
to the performance we want any more than you can polish an AMC Gremlin into an
SR-71 Blackbird.

We need to rethink the core model. This chapter introduces that model: bytecode.
Then we'll start growing the skeleton of our new Lox implementation, clox.

## Bytecode?

I think the most compelling way to learn *what* bytecode is is by comparing it
to the two other approaches we might consider if we *weren't* doing a bytecode
virtual machine. In engineering, few choices are without trade-offs, and we'll
most clearly see the benefits and limitations of bytecode in the context of its
alternatives.

### Why not walk the AST?

Our existing interpreter has a couple of things going for it:

*   Well, first, we already wrote it. It's done. The *reason* we already have it
    is because this style of interpreter is **really simple to implement.** The
    runtime model of the code directly maps to the syntax. That means there's
    almost no effort to go from the parser to the data structures we need at
    runtime.

*   It's **portable.** Our current interpreter is written in Java and runs on
    any platform Java supports. If we wrote an AST walker in C, we could compile
    and run it on basically every platform under the sun.

Those are real advantages. But, on the other hand, **it's not
memory-efficient.** Each piece of syntax becomes an AST node. On top of the
overhead the JVM adds to each object to track its identity and other stuff, we
have a ton of tiny objects with lots of pointers between them. A simple Lox
expression like `1 + 2` turns into an object tree something like this:

**todo: illustrate**

Each of those pointers adds an extra 32- or 64- bits of overhead to the
object. Much worse, this sprinkling our data across the heap in a
loosely-connected slew of little objects does very bad things for <span
name="locality">*spatial locality*</span>.

<aside name="locality">

I wrote [an entire chapter][gpp locality] about this exact problem in my first
book, "Game Programming Patterns", if you want to really dig in.

[gpp locality]: http://gameprogrammingpatterns.com/data-locality.html

</aside>

Over the past several decades, CPUs have gotten orders of magnitude faster.
Alas, RAM has not. If every instruction needed data from RAM, the chip would be
slowed to a crawl, sitting waiting for bytes to trickle in from the bug.

To compensate for that, a CPU has multiple layers of caching. If a piece of
memory it needs is already in the cache, it can be loaded *much* faster. We're
talking upwards of 100 *times* faster than pulling from main memory.

How does data get into that cache? The machine speculatively stuffs things in
there for you automatically. It's heuristic is pretty simple. Chip designers
noticed that programs often work through memory sequentially. If your C program
has an array of data that it's iterating over and processing, it will be reading
consecutive bytes of memory.

They called this observation *data locality* -- when a program uses some data at
one location, it will likely need other nearby data soon after. To take
advantage of that, whenever the CPU reads a bit of data from RAM, it pulls in a
whole little bundle of nearby memory and stuffs it in the cache.

If your program next requests some data close enough to be inside that
already-cached bundle, your CPU runs like a well-oiled conveyor belt in a
factory. You *really* want to take advantage of this. Doing that means the way
we represent code in the interpreter should be dense and sequential in memory.

Now look up at that tree. Those subobjects could be <span
name="anywhere">*anywhere*</span>. Every step the tree-walker takes where it
follows a references to a subnode is likely to step outside the bounds of the
cache and force the CPU to stall until a new lump of data can be slurped in from
RAM. Just the *size* of those tree nodes with all of their pointer fields and
object headers tends to push objects away from each other and out of the cache.

<aside name="anywhere">

Even if they happened to be allocated in sequential memory when the parser first
produced them, after a couple of rounds of garbage collection -- which may move
objects around in memory -- there's no telling where they'll be.

</aside>

There is other overhead around the interface dispatch that we use for the
visitor pattern, but the above is plenty enough to keep us up at night.

### Why not compile to native code?

If you want to go *real* fast, you want to get all of those layers of
indirection out of the way. Right down to the metal. Machine code. It even
*sounds* fast. *Machine code.*

Compiling directly to the native instruction set the chip supports is the way
all the fastest languages run, and has been since the early days when engineers
actually hand-wrote programs in machine code. On punched cards, which,
presumably, they punched *with their fists*.

If you've never written any machine code, or it's slightly more human-palatable
cousin assembly code before, I'll give you the gentlest of introductions. Native
code is a densely-packed linear series of instructions, encoded directly in
binary. On most architectures, each instruction is between one and a few bytes
long, depending on what operation it represents and what operands it needs to
modify the operation.

Instructions are almost mind-numbingly low level. "Move a word from this address
to this register." "Add the integers in these two registers." Stuff like that.
Unlike our AST, there is no tree structure to the code. Control flow is handled
by jumping directly from one point in the code to another.

To execute it, the CPU tears through the instruction sequence, executing each
one in turn. If it hits a jump, it moves execution to a different point in the
code, but otherwise it just cranks through one instruction after the other. No
indirection, no overhead, no unnecessary skipping around or chasing pointers.

This performance comes at a cost. First of all, compiling to native code ain't
easy. Most chips in wide use today have instruction set architectures whose
history dates back to the eighties or longer. Each generation crams new
instructions in the nooks and crannies left between the former, like a
ramshackle series of additions built onto a cottage.

These new instructions offer speed but only to compilers that use them well. At
a bare minumum, you need to be smart about your register allocation, pipelining,
and instruction selection. It's a *lot* to learn. Fun for some, but a high
barrier to entry.

Even if you do cross that hurdle and master some instruction set, that's only
*one* of the several dominant architectures out there. In order to support your
language on all of those architectures, you'll have to learn all of their
instruction sets and write a separate <span name="back">back end</span> for each
one.

<aside name="back">

It's not quite as dire as that sounds. A well-engineered compiler will let you
share the front and and most of the middle layer optimization passes across the
different architectures you support. It's mainly the code generation and some of
the details around instruction selection that you'll need to write afresh each
time.

</aside>

### What is bytecode?

Now we can see where bytecode comes in. Our tree-walk interpreter is simple and
portable, but slow. Native code gives us *all* the performance but sacrifices
that simplicity and portability. Bytecode sits in the middle. It retains all of
the portability of our first interpreter. It gives up some simplicity to get
some performance in return, though not as fast as going fully native.

Structurally, bytecode resembles machine code. It's a dense, linear sequence of
binary instructions. That keeps overhead low and plays nice with the cache.
However, it's a *much* simpler instruction set than any real chip out there. (In
many language, each instruction is only a single byte long, hence "bytecode".)
It's cleaner, and closer to the semantics of the language we're compiling from.

I think of it as sort of the ideal fantasy architecture that someone writing a
compiler for the language wishes they could target. It's designed to be easy to
compile to.

The problem with a fantasy architecture, of course, is that it doesn't exist. We
solve that by writing an *emulator* -- a simulated chip written in software that
can execute the bytecode an instruction at a time. A **virtual machine** if you
will.

That emulation layer adds <span name="p-code">overhead</span>, which is the main
reason bytecode is slower than real native code. But in return, it gives us
portability. If we write our VM in a language like C that already has support
for all of the machines we care about, we can run our emulator on top of any
hardware we like.

This is the path we'll take with our new interpreter, clox. The same path taken
by the main implementations of Python, Ruby, Lua, OCaml, Erlang, and others.

<aside name="p-code">

One of the first bytecode formats was [p-code][], developed for Niklaus Wirth's
Pascal language. You would think the overhead of interpreting bytecode would be
intolerable on PDP-11 running at 15MHz, which is the system it was originally
developed for.

But back then, computers were in their Cambrian explosion period and new
architectures seemed to appear every day. Keeping up with the latest chips was a
really challenge. That's why the "p" in "p-code" doesn't stand for "Pascal", but
"portable".

[p-code]: https://en.wikipedia.org/wiki/P-code_machine

</aside>

In many ways, it parallels the structure of our previous interpreter:

**todo: illustrate jlox vs clox. jlox has columns for parser -> ast ->
interpreter. clox is compiler -> bytecode -> vm.**

Of course, we won't do it all in that order. Like our previous interpreter,
we'll bounce between all three of those, building up the interpreter one
language feature at a time. In this chapter, we'll get the skeleton of the
application in place and the data structures needed to store and represent a
chunk of bytecode.

## Getting Started

The two major components of our new interpreter are the compiler to produce
bytecode and the VM to run it. Between those is the bytecode itself. Since both
the front- and back-ends depend on it, we'll start there, in the middle. In this
chapter, we'll begin defining our virtual instruction set, and write the C code
needed to create chunks of those instructions and the accoutrements that go with
them.

But let's not get ahead of ourselves. <span name="ready">Fire</span> up your
trusty text editor and C compiler. It all begins with `main()`:

<aside name="ready">

Now is a good time to stretch, maybe crack your knuckles. A little montage music
wouldn't hurt either.

</aside>

^code main-c

From this tiny seed, we will grow our entire VM. Since C gives us so little on
its own, we'll need to spend some time amending the soil with some utilities and
helpers first. We'll start with a header:

^code common-h

There are some types and constants we'll use throughout the interpreter, and
this is a handy place to put them. For now, it's just the nice C99 Boolean and
sized integer types.

## Chunks of Instructions

Bytecode is made up of instructions. Each instruction has a one-byte **operation
code** (universally shorted to "opcode"). That's a number that defines what kind
of instruction we're talking about -- add, subtract, look up variable, etc.

We need a nice term to refer to a sequence of bytecode instructions. I've been
using "chunk" informally for that, so lets make it the official term too. We
actually do need a structure that wraps both an array of instruction and some
other ancillary data we want to carry along with it.

"Chunk" works fine for that, so let's use it. We'll put our opcodes and chunk
structure in a new module:

^code chunk-h

We'll start with a single instruction, `OP_RETURN`. When we have a VM, this
instruction will mean "return from the current function". In other words, it's
what this compiles to:

```lox
return;
```

I'll admit this isn't exactly useful yet, but we have to start somewhere, and
this is a particularly simple instructions (for reasons we'll learn more about
later).

### A dynamic array of instructions

A chunk is a sequence of instructions:

^code chunk-struct (1 before, 2 after)

This struct, at the moment, is simply a wrapper around an array of bytes. That raise the obvious question, "How big should this array be?" The problem is that the compiler doesn't know ahead of time how many instructions a given Lox program will compile to. So we need some kind of growable structure.

If this were a <span name="lisp">CS 101 class</span>, we'd proably do a linked list. But it turns out in practice that those are rarely a good idea on today's hardware. Remember, we want a *dense* *linear* series of instructions to maximize our locality. A linked list doesn't do that. A *dynamic array* does.

<aside name="lisp">

Or possibly a book on implementing a Lisp, since linked lists are so fundamental
to that language family that it's in the *name* -- "**Lis**t **P**rocessing".

</aside>

Dynamic arrays are one of my <span name="arraylist">favorite</span> data
structures. I realize that sounds a bit like saying vanilla is my favorite ice
cream flavor, but look at the perks:

1. The cache-friendliness of a fixed-size array.
2. Fast, constant-time, indexed lookup of an element.
3. Constant-time appending to the end of the array.

<aside name="arraylist">

A more direct justification for dynamic arrays is that that's how the Java
ArrayList class we used everywhere in jlox is implemented.

</aside>

If you're rusty on dynamic arrays, the idea is pretty simple. You allocate an
array of some size on the heap and then incrementally fill it up as you add
elements to it. If it's full when you try to add an element, you do this little
dance first:

1.  Allocate a new, bigger array.
2.  <span name="amortized">Copy</span> the elements from the old too-small array
    over to the new array.
3.  Delete the old array.

**todo: illustrate**

<aside name="amortized">

You'd think that copying the array when it grows would mean that appending would
be O(n), not O(1). It's weird, though, because if you allocate a bigger array
with some extra slots, you only need to grow on *some* appends. So appending is
O(1) sometimes, and O(n) other times.

[*Amortized analysis*](https://en.wikipedia.org/wiki/Amortized_analysis) gives a
way to unify those costs. Using that, you can show that as long as you grow the
array by a *multiple* of its current size, that the cost of a *series* of
appends is O(n), which means each individual append has *average* cost O(1).

</aside>

When we grow the array, it will have some extra slots that have been *allocated*
but that are not yet *used*. That means we need to keep track of two sizes for
the array -- the number of used elements ("count"), and the number of allocated
ones ("capacity").

^code count-and-capacity (1 before, 1 after)

C doesn't have constructors, so we'll add a function to initialize a new chunk:

^code init-chunk-h (1 before, 2 after)

And implement it thusly:

^code chunk-c

It starts off completely empty. We don't even allocate an array yet. To add
instructions to the chunk, we'll declare a function to write to it:

^code write-chunk-h (1 before, 2 after)

This is where the interesting work happens:

^code write-chunk

The first thing we need to do is see if the current array already has capacity
for the new instruction. That will be true if capacity is greater than count. If
that's *not* true, then we first need to grow the array to make room (or
allocate it at all if this is the very first instruction).

That's a two step process. First, we figure out the new capacity based on the
current capacity. Then we grow the array to that size. Both of those lower-level
memory operations are defined in a new module:

^code chunk-c-include-memory (1 before, 2 after)

It looks like this:

^code memory-h

The first macro, `GROW_CAPACITY()`, calculates the new capacity based on the
current capacity. In order to get the performance we want, the important part is
that it *scales* based on the old size. We double it here, which is pretty
typical. 1.5x is another common factor.

We also have a little edge case to handle the first grow where the old capacity
is zero. In that case, we jump straight to eight elements. That <span
name="profile">avoids</span> a little extra memory churn when the array is very
small, at the expense of wasting a few extra bytes on small chunks.

<aside name="profile">

I picked the number eight somewhat arbitrarily for the book. Most dynamic array
implementations have some minimum threshold like this. The right way to pick a
value for this is to profile against real-world usage and see which constant
seems to make the best performance trade-off between extra grows versus wasted
space.

</aside>

Once we know the desired capacity, we create or grow the array to that size
using `GROW_ARRAY()`:

^code grow-array (2 before, 2 after)

This macro wraps a little syntactic sugar around a function call to
`reallocate()` where the real work happens. The macro itself takes care of
getting the size of the array's element type and casting the resulting `void*`
back to a pointer of the right type.

The magic lives here:

^code memory-c

OK, "magic" might be overselling it. This function is again the thinnest of
wrappers around the C <span name="realloc">standard</span> library's `realloc()`
function. That function, if you're rusty on your C stdlib, is sort of a hybrid
of `malloc()`, `free()`, and some extra special sauce.

<aside name="realloc">

If we wanted to be hardcore, we would write our own memory allocation function
from scratch, using only `mmap()`. I'll leave that as an exercise for
particularly adventurous readers. (Daunting as it sounds, it is actually kind of
fun.)

</aside>

You give it a pointer to some existing memory (`previous`) and a desired size
(`newSize`). If the pointer is `NULL`, it allocates and returns a brand new
chunk of memory with the desired size, just like `malloc()`. If the desired size
is zero, and the pointer is not `NULL`, it deallocates the pointer, like
`free()` and returns `NULL`.

**todo: illustrate with table of existing pointer null-ness, desired size and
resulting behavior.**

The interesting case is when the pointer is not null and the size is not zero.
In that case, it will try to resize the allocated block. If the new size is
*smaller* than the existing block of memory, it simply <span
name="shrink">shrinks</span> the allocated size of the block and returns the
same pointer you gave it. If the new size is *larger*, it will attempt to grow
the existing block of memory.

It can only do that if the memory after that block isn't already in use. If it's
unavailable, `realloc()` instead allocates a *new* block of memory of the
desired size, copies over the old bytes, frees the old one, and then returns a
pointer to the new block. Remember, that's exactly the behavior we want for our
dynamic array.

<aside name="shrink">

Given that all you passed in was a bare pointer to the first byte of memory,
what does it mean to "shrink" the allocated block? Under the hood, the C
standard memory allocator keeps track of some additional bookkeeping information
for each block of heap-allocated memory. It knows the size of each block. Given
a pointer to some previously-allocated memory, it can find this bookkeeping
information, which is necessary to be able to cleanly free it. (Many
implementations of `malloc()` store the allocated size in memory right *before*
the returned address.) It's this size metadata that `realloc()` updates.

</aside>

Right now, all of the interesting work happens in `realloc()`. When we later
implement our own garbage collector to automatically manage memory,
`reallocate()` is going to get a *lot* more fun. For now, this is enough.

OK, we can create new chunks and write instructions to them? Are we done? Nope!
We're in C now, remember, we have to manage all our memory ourselves, like Ye
Olden Times. We also need a function to release all of the memory used by a
chunk:

^code free-chunk-h (1 before, 1 after)

The implementation is:

^code free-chunk

It deallocates all of the memory and then calls `initChunk()` again to zero out
all of the fields leaving the chunk in a well-defined zero-memory state. To free
the memory, we add one more macro:

^code free-array (1 before, 2 after)

Like `GROW_ARRAY()`, it's a wrapper around a call to `reallocate()`. We just
"resize" the memory down to zero bytes, which the function already knows how to
handle.

## Disassembling Chunks

Great, now we have a little module for creating chunks of bytecode. Let's try
it out by hardcoding a sample chunk:

^code main-chunk (1 before, 1 after)

Don't forget the include:

^code main-include-chunk (1 before, 2 after)

Give that a try. Did it work? Uh... who knows? All it does is push some bytes
around in memory. We have no human-friendly way to see what's actually inside
that chunk we made.

To fix that, we're going to create a *disassembler*. An *assembler* is an
old-school program that takes a file containing human-friendly mnemonic names
for machine code instructions like "PUSH" and "RETURN" and translates them to
their binary machine code equivalent. A *dis*-assembler goes in the other
direction -- given a blob of binary machine code, it spits out a human-readable
textual listing of the instructions.

We don't have a real assembler per se. Our OpCode enum more or less approximates
that instead. But we will write a <span name="printer">disassembler</span>.
Given a chunk, it will print all of the instructions in it to stdout.
Technically, a Lox *user* would never use this. But we Lox *maintainers* will
certainly benefit from it since it gives us a window into the interpreter's
internal representation of code.

<aside name="printer">

In jlox, we built analogous tool -- the [AstPrinter class][].

[astprinter class]: representing-code.html#a-(not-very)-pretty-printer

</aside>

In `main()`, after we hand-create the chunk, we'll pass it to the disassembler
to dump it to the screen:

^code main-disassemble-chunk (2 before, 1 after)

Again, we're going to whip up a <span name="module">new</span> module:

<aside name="module">

I promise you we won't be creating this many new files in the later chapters.
This first one is laying a lot of the groundwork.

</aside>

^code main-include-debug (1 before, 2 after)

Here's that header:

^code debug-h

In `main()`, we call `disassembleChunk()` to disassemble all of the instructions
in the entire chunk. That's implemented in terms of `disassembleInstruction()`,
which just disassembles a single instruction. It shows up here in the header
because we'll call it from the VM in later chapters.

Now that that ceremony is out of the way, let's write some actual code. Here's
a start at the implementation file:

^code debug-c

To disassemble a chunk, we simply print a little header (so we can tell *which*
chunk we're looking at) and then scan through the bytecode, disassembling each
instruction. The way it iterates through the chunk is a little funny. Instead of
incrementing `i` ourselves, we let `disassembleInstruction()` do it for us. When
you call it, after disassembling the instruction at the given offset, it returns
the offset of the *next* instruction. This is because, as we'll see later,
different instructions can have different sizes.

The core of the debug module is this function:

^code disassemble-instruction

First, it prints the byte offset of the given instruction -- it tells us where
in the chunk the instruction is. This will be a helpful signpost when we start
doing control flow and jumping around in the chunk.

Next it reads a single byte from the bytecode at the given offset. That's our
opcode. We <span name="switch">switch</span> on that. For each kind of
instruction, we dispatch to a little utlity function for displaying it. On the
off chance that the given byte doesn't look like an instruction at all --
presumably from some kind of bug in our compiler, we print that too. For the one
instruction we do have, `OP_RETURN`, the display function is:

<aside name="switch">

We only have one instruction right now, but this switch will grow throughout the
rest of the book.

</aside>

^code simple-instruction

There's nothing to a return instruction, so all it does is print the name of the
opcode, then return the next byte offset past this instruction. Other later
instructions will be have more action going on.

Now if we run our nascent interpreter, it actually prints something:

```
== test chunk ==
0000 OP_RETURN
```

It worked! This is sort of the "Hello, world!" of our compilation pipeline.
We've shown we can create a chunk, write an instruction to it, and then extract
that instruction back out of it. It means our binary encoding and decoding of
the bytecode is actually working.

## Constants

Now that we have a rudimentary chunk structure working, let's start making it
more useful. We can store *code* in chunks, but what about *data*? That might
seem like a weird question. Most values the interpreter works with are created
at runtime as the result of operations. In:

```lox
1 + 2;
```

The value 3 appears nowhere in the code, but the literals 1 and 2 do. To
compiled that to bytecode, we need some sort of instruction that means "produce
a constant" and those actual values need to get stored in the chunk somewhere.
In jlox, the AST node for a number literal held the value. We need a different
solution now that we're using bytecode.

### Representing values

We won't be *running* any code in this chapter, but since constants sort of span
the static and dynamic worlds of our interpreter, they force us to start
thinking at least a little bit about how our VM should represent values.

For now, we're going to start a simple as possible -- we'll only support
double-precision floating point number values. This will obviously expand over
time, so we'll sketch out a new module now to give ourselves room to grow:

^code value-h

For now, we'll create a typedef that abstracts how values are concretely
represented in C. That way, we can change that representation without needing to
go back and fix existing code that passes around values.

Consider this tiny script:

```lox
return 123;
```

We can use our `OP_RETURN` instruction to represent the `return` part, but where
do we store the `123` in the chunk? For small fixed-size values, many bytecode
formats store the value directly in the bytecode right after the opcode. These
are called *immediate* values because the bits for the value are immediately
after the instruction.

That doesn't work well for large or variable-sized constants like strings. In a
native compiler to machine code, those bigger constants are stored in a separate
"constant data" region in the binary executable. Then, the instruction for
producing that value has an address or offset pointing to where the constant is
stored in that section.

Most VMs do something similar. For example, the Java Virtual Machine specifies a
*constant pool* that is associated with each compiled class. We'll do something
similar in our VM. Each chunk will carry with it a table of values for the
constants that appear as literal values in that chunk. To keep things <span
name="immediate">simpler</span>, we'll put *all* constants in there, even simple
integers.

<aside name="immediate">

In addition to needing two kinds of constant instructions -- one for immediate
values and one for constants in the constant table -- immediates also force us
to worry about alignment, padding, and endianness. Some architectures aren't
happy if you try to say, stuff a 4-byte integer at an odd address.

</aside>

### Value arrays

The constant pool is a straight array of values. The instruction to load a
constant will look up the value by index in that array. As with bytecode, the
compiler doesn't know how big this array needs to be ahead of time. So, again,
we need a dynamic array. Since C doesn't have any kind of generic data
structures, we'll write <span name="generic">another</span> dedicated dynamic
array data structure, this time for values:

<aside name="generic">

Yeah, creating a new structure and handful of functions each time we need an
array of a different type is kind of a chore. If we really cared, we could
cobble together a set of preprocessor macros to automate this, but that would be
overkill for clox. We won't need to many more of these.

</aside>

^code value-array (1 before, 2 after)

As with the bytecode array in Chunk, this struct wraps a pointer to an array
along with its allocated capacity and the number of elements that are currently
occupied. We also need the same three functions to work with these arrays:

^code array-fns-h (1 before, 2 after)

The implementations will probably give you deja vu. First, to create a new one:

^code value-c

Once we have an empty array, we can start <span name="add">adding</span>
constants to it:

<aside name="add">

Fortunately, we don't need other operations like insertion and removal.

</aside>

^code write-value-array

The memory-management macros do let us reuse some of the logic we wrote for
chunks. Finally, we can release all memory used by the array:

^code free-value-array

Now that we have growable arrays of values, we can add one to Chunk so it can
store its constants:

^code chunk-constants (4 before, 1 after)

Don't forget the include:

^code chunk-h-include-value (1 before, 2 after)

And another one over in the chunk.c implementation file:

^code chunk-c-include-value (1 before, 2 after)

Ah, C, and it's Stone Age modularity story. Where we right? Right. When we
initialize a new chunk, we initialize its constant pool too:

^code chunk-init-constant-array (1 before, 1 after)

Likewise, we <span name="circle">free</span> the constants when we free the
chunk:

<aside name="circle">

It's like the circle of life.

</aside>

^code chunk-free-constants (1 before, 1 after)

The compiler could write to the constant array inside Chunk directly -- it's
not like C has private fields or anything -- but it's a little nicer to add an
explicit function for that. First we declare it:

^code add-constant-h (1 before, 2 after)

Then we implement it:

^code add-constant

After it adds the constant to the end of the array, it returns the index so we
can locate that same constant later.

### Constant instructions

We can *store* constants in chunks, but we also need to *execute* them. In a
piece of code like:

```lox
print 1;
print 2;
```

The compiled chunk needs to not only contain the values 1 and 2, but know when
to produce them so they can be printed. So we'll also define a constant
instruction:

<aside name="load">

I'm being vague about what it means to "load" or "produce" a constant because we
haven't learned how the virtual machine actually executes code at runtime yet.
For that, you'll have to wait for (or skip ahead to, I suppose) the [next
chapter][vm].

[vm]: a-virtual-machine.html

</aside>

^code op-constant (1 before, 1 after)

When the VM executes a constant instruction, it <span name="load">"loads"</span>
the constant for use. This new instruction is a little more complex than
`OP_RETURN`. In the above example, we load two different constants. A single
bare instruction isn't enough to know *which* constant to load.

To handle cases like this, our bytecode -- like most others -- allows
instructions to have <span name="operand">**operands**</span>. These are stored
as binary data immediately after the opcode in the instruction stream and let us
sort of parameterize what the instruction does.

The opcode determines how many operand bytes it has and what they mean. For
example, a simple operation like "return" may have no operands, where an
instruction for "load local variable" needs an operand to identify which
variable to load. Each time we add a new instruction to clox, we'll specify what
it's operands look like -- its *instruction format*.

**todo: illustrate instruction**

<aside name="operand">

Instruction operands are *not* the same as the operands you encounter in
arithmetic expressions. It gets kind of confusing because arithmetic operators
often *do* have corresponding instructions. The `+` in Lox *will* compile to an
`ADD` instruction. But the operands to the bytecode instruction control how the
instruction *itself* is processed, and not which values it operates on.

</aside>

In this case, `OP_CONSTANT` takes a single byte operand that specifies which
constant to load from the chunk's constant array. Since we don't have a compiler
yet, we'll explicitly write one in the test chunk we've been hand-crafting:

^code main-constant (1 before, 1 after)

We add the constant value itself to the chunk's constant array. That returns the
index of the constant in the array. Then we write the constant instruction,
starting with its opcode. After that, we write the one-byte constant index
operand.

If we try to run this now, the disassembler is going to yell at us because it
doesn't know how to decode the new instruction. Let's fix that:

^code disassemble-constant (1 before, 1 after)

This instruction has a different operand format, so we'll write a new helper
function to disassemble that:

^code constant-instruction (1 before, 1 after)

There's more going on here. As before, it prints out the name of the opcode.
Then it pulls out the constant index from the subsequent byte in the chunk. It
shows that index, but that isn't super useful to we human readers. So it also
looks up the actual constant value -- since constants *are* known at
compile-time after all -- and displays the value itself too.

This requires some way to print a Lox Value to stdout. That function will live
in the value module, so we'll include that:

^code debug-include-value (1 before, 2 after)

In that header, we declare:

^code print-value-h (1 before, 2 after)

And here's an implementation:

^code print-value

As you can imagine, this is going to get more complex once we add dynamic typing
to Lox and have values of different types.

Back in `constantInstruction()`, the only remaining piece is the return value:

^code return-after-operand (1 before, 1 after)

Remember that `disassembleInstruction()` also returns a number to tell the
caller how many bytes to advance to reach the beginning of the *next*
instruction. Where `OP_RETURN` was only a single byte, `OP_CONSTANT` is two --
one for the opcode and one for the operand.

## Line Information

Chunks contain code -- instructions and their operands -- and constants. That's
almost all we need for the entire runtime representation of a user's program.
It's kind of crazy to think that we can reduce all of the different AST classes
with their distinct fields in jlox down to a simple array of bytes and array of
constants. There's only kind of information we're missing. We need it, even
though the user hopes to never see it.

If a runtime error occurs in the user's program, we show them the line number in
their source code that contains the offending code. In jlox, we found that by
storing line numbers in all of the tokens and then storing tokens in the AST
nodes. We need a different solution for clox.

We don't have syntax tree nodes in bytecode, so our unit of execution is
instructions. Given <span name="error">any</span> instruction, we need to be
able to determine the line of the user's source program that it was compiled
from.

<aside name="error">

In theory, we only need to store line info for instructions that can potentially
cause runtime errors. To keep things simple, we'll assume any instruction can
cause an error and support them all uniformly.

</aside>

There are a lot of ways we could encode this. We are going to take the absolute <span name="side">simplest</span> approach I could come up with, even though it's not what I'd call memory efficient. In the chunk, we will store a separate array of integers that parallels the bytecode. For each byte of code, there will be a number in that array. When a runtime error occurs, we simply look up the line number at the same index as the instruction in the bytecode.

<aside name="side">

While this braindead approach uses a lot of memory -- up to eight times as much
as the bytecode itself on a 64-bit machine! -- it does do one thing right. It
keeps the line information in a *separate* array instead of interleaving it in
the bytecode array itself.

Putting the line numbers in the bytecode array would spread the actual
instructions farther apart. That leads to more frequence cache misses since a
given piece of memory doesn't contain as many actual instructions. This is a
particularly poor use of spatial locality given that the line info is rarely
used -- it's only accessed when a runtime error occurs.

</aside>

To implement this, we add another array to Chunk:

^code chunk-lines (1 before, 1 after)

Since it exactly parallels the bytecode array, we don't need a separate count or
capacity. Every time we touch the code array, we make a corresponding change to
the line number array, starting with initialization:

^code chunk-null-lines (1 before, 1 after)

And likewise deallocation:

^code chunk-free-lines (1 before, 1 after)

When we write a byte of code to the chunk, we need to tell it what source line
that byte comes from, so we add an extra parameter in the declaration:

^code write-chunk-with-line-h (1 before, 1 after)

And in the implementation:

^code write-chunk-with-line (1 after)

Before we can actually write to the line array, we need to ensure it's allocated
enough, so we do the same growth check we did for the bytecode:

^code write-chunk-line (2 before, 1 after)

Finally, we can add the element:

^code chunk-write-line (1 before, 1 after)

### Disassembling line information

We can try this out with our hand-authored chunk. First, since we added a new
parameter to `writeChunk()`, we need to fix those calls to pass in some
arbitrary line number:

^code main-chunk-line (1 before, 2 after)

Once we have a compiler, of course, it will actually keep track of the line
while it's parsing and pass that in. In our disassembler, it's incredibly useful
to show which source line each instruction was compiled from. That will give us
a way to map back to the original code when we're trying to figure out what some
blob of bytecode is supposed to do.

^code show-location (2 before, 2 after)

After printing the offset of the instruction -- the number of bytes from the
beginning of the chunk -- we'll show its source line. Bytecode instructions tend
to be pretty fine-grained. That means a single line of source code often
compiles to a whole sequence of instructions.

To make that a little more visually pleasant, we'll show a "|" for any
instruction that came from the same source line as a preceding one. The end
result for our artisanal chunk looks like:

```
== test chunk ==
0000  123 OP_CONSTANT         0 '1.2'
0002    | OP_RETURN
```

We have a three-byte chunk. The first two bytes are a constant instruction that
loads 1.2 from the chunk's constant pool. The first byte is the `OP_CONSTANT`
opcode and the second is the index in the constant pool. The third byte (at
offset `0x2`) is a single-byte return instruction.

In the remaining chapters, we will flesh this out with lots more kinds of
instructions. But, aside from that, we have everything we need now to completely
represent an executable piece of code at runtime in our virtual machine.
Remember that big pile of AST classes we wrote a script to generate in jlox? In
clox, we've reduced that down to three arrays: bytes of code, constant values,
and line information for debugging.

This reduction is a key reason why our new interpreter will be faster than jlox.
You can think of bytecode as a way to serialize an AST, highly optimized for
exactly what the interpreter needs to deserialize from it, and in what order. In
the [next chapter][vm], we will see how the virtual machine does exactly that.

<div class="challenges">

## Challenges

1.  Our encoding of line information is hilariously wasteful of memory. Given
    that a series of instructions often correspond to the same source line, a
    natural solution is something akin to *run-length encoding* of the line
    numbers.

    Devise an encoding that compresses the line information for a
    series of instructions on the same line. Change `writeChunk()` to write this
    compressed form, and implement a `getLine()` function that, given the index
    of an instruction, determines the line where the instruction occurs.

2.  Because `OP_CONSTANT` only uses a single byte for its operand, a single
    chunk may only contain up to 256 different constants. That's small enough
    that people writing real-world code will hit that limit. We could use two
    or more bytes to store the operand, but that makes *every* constant instruction take up more space. Most chunks won't need that many unique
    constants, so that wastes space and sacrifices some locality in the common
    case to support the edge case.

    To balance those two competing aims, many instruction formats feature
    multiple instructions that perform the same operation but with operands of
    different sizes. Leave our existing one-byte `OP_CONSTANT` instruction
    alone, and define a second `OP_CONSTANT_LONG` instruction. It stores the
    operand as a 24-bit number, which should be plenty.

    Implement this function:

    ```c
    void writeConstant(Chunk* chunk, Value value) {
      // Implement me...
    }
    ```

    It adds `value` to `chunk`'s constant array and then writes an appropriate
    instruction to load the constant. Also add support to the disassembler for
    `OP_CONSTANT_LONG` instructions.

    Defining two instructions seems to be the best of both worlds. What
    sacrifices, if any, does it force on us?

3.  This one is a doozy. Our `reallocate()` function relies on the C standard
    library for dynamic memory allocation and freeing. Let's see how it does
    that by writing our own memory manager on top of it.

    Implement `reallocate()` without calling `realloc()`, `malloc()`, or
    `free()`. You are allowed to call `malloc()` *once*, at the beginning of the
    interpreter's execution, to allocate a single big block of memory which your
    `reallocate()` function has access to. It parcels out blobs of memory from
    that single region. It's your job to define how it does that.

</div>

<div class="design-note">

## Design Note: Test Your Language

We're almost halfway through the book and one thing we haven't talked about is
*testing* your language implementation. That's not because testing isn't
important. I can't possibly stress enough how important it is to have a good,
comprehensive test suite for your language. Seriously. Test your language.

I wrote a [test suite for Lox][tests] (which you are welcome to use on your own
Lox implementation) before I wrote a single word of this book. Those tests found
countless bugs in my implementation.

[tests]: https://github.com/munificent/craftinginterpreters/tree/master/test

Tests are important in all software, but they're even more important for a
programming language for at least a couple of reasons:

*   Users expect their programming languages to be rock solid. We are so used to
    mature, stable compilers and interpreters that "It's your code, not the
    compiler" is [an ingrained part of software culture][fault]. If there are
    bugs in your language, users will go through the full five stages of grief
    before they can figure out what's going on, and you don't want to put them
    through all that.

*   A language implementation is a deeply interconnected piece of software. Some
    kinds of applications tend to be broad and shallow. If the file loading code
    is broken in your text editor, it (hopefully!) won't cause failures in the
    text rendering on screen. But languages tend to be narrower and deeper,
    especially the core of the interpreter that handles the language's actual
    semantics. (The core libraries for a language are a little more spread out.)
    That makes it easy to for subtle bugs to creep in caused by weird
    interactions because various parts of the system. It takes good tests to
    flush those out.

*   The input to a language implementation is, by design, combinatorial. There
    are infinite number of possible programs a user could write, and your
    implementation needs to run them all correctly. You obviously can't test
    that exhaustively, but you need to work hard to cover as much of the input
    space as you can.

*   Language implementations are often complex, constantly changing, and full
    of optimizations. All of that tends to lead to big gnarly code with lots of
    dark corners where bugs can hide.

[fault]: https://blog.codinghorror.com/the-first-rule-of-programming-its-always-your-fault/

All of that means you've gonna want a lot of tests. But *what* tests? Most teams
I've seen focus mostly on end-to-end "language tests". Each test is a program
written in the language along with the output (or errors) it's expected to
produce. Then you have a test runner that pushes the script through your
language implementation and validates that it does what it's supposed to.

Writing your tests in the language itself has a few nice advantages:

*   The tests aren't coupled to any particular API or internal architecture
    decisions of the implementation. This frees you to reorganize or rewrite
    parts of your interpreter or compiler without needing to update a slew of
    tests.

*   You can use the same tests for multiple implementations of the language.

*   Tests can often be terse and easy to read and maintain since they are
    simply scripts in your language.

It's not all rosy, though:

*   End-to-end tests help you determine *if* there is a bug, but not *where*
    the bug is. It can be harder to figure out where the erroneous code in the
    implementation is because all the test tells you is that it didn't produce
    the right output.

*   It can be a chore to craft a valid program that tickles some obscure corner
    of the implementation. This is particularly true for highly-optimized
    compilers where you may need to write convoluted code to ensure that you
    end up on just the right optimization path where a bug may be hiding.

*   The overhead can be high to fire up the interpreter, parse, compile, and
    run each test script. With a big suite of tests -- which you *do* want,
    remember -- that can mean a lot of time spent waiting for the tests to
    finish running.

I could go on, but I don't want this to turn into a sermon. Also, I don't
pretend to be an expert on *how* to test languages. I just want you to
internalize how important is is *that* you test yours.

</div>
