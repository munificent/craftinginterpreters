^title A Virtual Machine
^part A Bytecode Virtual Machine

> Magicians protect their secrets not because the secrets are large and
> important, but because they are so small and trivial. The wonderful effects
> created on stage are often the result of a secret so absurd that the magician
> would be embarrassed to admit that that was how it was done.
>
> <cite>Christopher Priest</cite>

We've got ourselves an internal representation for the user's program -- a
sequence of binary bytecode intstructions -- but it's kind of dead and cold.
We don't know what those instructions *are* because we don't know how they
*work*. It would be hard to write a compiler that outputs bytecode when we don't
have a good understanding of how that bytecode behaves.

So, before we go back and build the front end of our new interpreter, we will
write the back end -- the virtual machine that executes instructions. It will
breathe life into the bytecode. Watching the instructions dance around will give
us a clearer picture of how the compiler will translate the user's source code
into a series of them.

## An Instruction Execution Machine

The "virtual machine" is a part of our interpreter's internal architecture. You
hand it a chunk of code -- literally a Chunk -- and it runs it. The code and
data structures for this part of clox go into a new module:

^code vm-h

As usual, we start simple. The VM will gradually acquire a decent amount of
state it needs to keep track of, so we define a struct now to stuff it all in.
Right now, all we have is the chunk that it's executing.

Like we'll do with most of the data structures we'll create, we also define
functions to create and tear down a VM. Here's the implementation:

^code vm-c

We don't have any interesting state to initialize or free yet, so the functions
are still empty. Patience, grasshopper.

The slightly more interesting line here is that declaration of `vm`. This module
is eventually going to have a whole pile of functions and it would be a chore to
pass around a pointer to the VM to all of them. Instead, we declare a single
global VM object. We only need <span name="one">one</span> anyway, and this
keeps the code in the book a little lighter on the page.

<aside name="one">

The choice to have a static VM instance is a concession for the book, but not
necessarily a good engineering choice for a real language implementation. If
you're building a VM that's designed to be embedded in other host applications,
it's gives the host more flexibility if you *do* explicitly take a VM pointer
and pass it around.

That way, the host app can manage its memory itself, run multiple VMs in
parallel, etc.

What I'm doing here is a global variable, and everything you've heard about how
bad global variable is still true when programming in the large. But when
keeping things small for a book...

**todo: overlap...**

</aside>

Before we start pumping fun code into our VM, let's go ahead and wire it up to
the interpreter's main entrypoint:

^code main-init-vm (1 before, 1 after)

We spin up the VM when the interpreter first starts. Then when we're about to
exit, we wind it down:

^code main-free-vm (1 before, 1 after)

One last ceremonial obligation:

^code main-include-vm (1 before, 2 after)

Now when you run clox, it starts up the VM before it creates that one
hand-authored chunk from the [last chapter][]. The VM is ready, so let's make it
do something.

[last chapter]: chunks-of-bytecode.html#disassembling-chunks

### Executing instructions

You summon the VM into action by commanding it to interpret a chunk of bytecode:

^code main-interpret (1 before, 1 after)

This function is the main entrypoint into the VM. It's declared like so:

^code interpret-h (1 before, 2 after)

You give it the chunk to execute, it runs it, and then tells you how it went
using this InterpretResult enum:

^code interpret-result

We aren't using the result yet, but once we have a compiler that can report
static errors and runtime semantics that can produce runtime errors, clox will
use this to know how to set the exit code of the process.

We're tiptoeing towards some actual implementation:

^code interpret

First, we store the chunk being executed in the VM. Then we call `run()`, an
internal helpful function that actually runs the bytecode instructions. Between
those two parts is an interesting line. What is this `ip` business?

As the VM works its way through the bytecode, it needs to keep track of where
it is -- which instruction it's currently working on. We don't want a local
variable for this because eventually other parts of the VM will need to access
it. Instead, we store it as a field in VM:

^code ip (1 before, 1 after)

Its type is a byte pointer. We use an actual real C pointer right into the
middle of the bytecode instead of something like an array index because it's
faster to dereference that directly than having to calculate the address from an
array index each time.

The name "IP" is traditional, and -- unlike many traditional names in CS --
actually makes sense: it's an **[instruction pointer][ip]**. Almost every
instruction set in the <span name="ip">world</span>, real and virtual, has a
register or variable like this. The other common name it is "PC" for "program
counter".

[ip]: https://en.wikipedia.org/wiki/Program_counter

<aside name="ip">

x86, x64, and the CLR call it "IP". 68k, PowerPC, ARM, p-code, and the JVM call
it "PC".

</aside>

We point it to the first byte of code in the chunk. Since we haven't executed
that instruction yet, it means it's pointing to the *next* instruction to
execute. This will be true during the entire time the VM is running: the IP
always points to the *next* instruction to be executed, not the current one
being handled. That might seem a little odd but most machines work this way.

The real fun happens in `run`():

^code run

This is the single most important function in all of clox, by far. When the
interpreter is running, it will spend something like 90% of the time inside it.
It is the beating heart of the VM.

Despite that dramatic intro, it's conceptually pretty simple. You have an outer
loop that goes and goes until we're done executing instructions. Each iteration
through that loop, we read and execute a single bytecode instruction.

To execute an instruction, we first need to figure out what kind of instruction
we're dealing with. We use this `READ_BYTE` macro to read the byte of code that
`ip` is currently pointing at and then advance `ip` to the next one.

Each instruction has its own semantics, so each will get its own handful of C
code to emulate that instruction's behavior. Given a numeric opcode, we need to
get to the right C code that implements that instruction. This process is called
"decoding" or "dispatching" the instruction.

We have to do that process for every single instruction, every single time it's
executed, so it is the most performance critical part of the entire virtual
machine. There is decades of research into how to do bytecode <span
name="dispatch">dispatch</span> efficiently and there are tons of clever
techniques.

<aside name="dispatch">

If you want some terms to search for, start with "direct threaded code", "jump
table", and "computed goto".

</aside>

Alas, most of the fastest approaches require either non-standard extensions to
C, or hand-written assembly code. For clox, we'll keep it simple and use a good
old switch statement. Just like our disassembler, we have a single giant switch
statement with a case for each opcode. The body of each case implements that
opcode's behavior.

For now, that's just OP_RETURN, and the only thing it does is exit the loop
entirely. Eventually, that instruction will be used to return from the current
Lox function, but we don't have functions yet, so we'll repurpose temporarily to
mean the end of the chunk of bytecode.

We can go ahead and add in our one other instruction next:

^code op-constant (1 before, 1 after)

We don't have enough machinery in place yet to do anything useful with a
constant. For now, we'll just print it standard out so we interpreter hackers
can see what's going on inside our VM. That call to `printf()` necessitates an
include:

^code vm-include-stdio (1 after)

We also have a new macro to define:

^code read-constant (1 before, 2 after)

It reads the next byte from the bytecode, treats the resulting number as an
index, and looks up the corresponding location in the chunk's constant table. In
later chapters, we'll add a few more instructions that have operands that refer
to constants, so we'll get some more mileage out of this macro.

Like the previous `READ_BYTE` macro, `READ_CONSTANT` is only used inside
`run()`. To make that more explicit, the macro definitions themselves are scoped
to that function. We <span name="macro">define</span> them at the beginning and
actually take the trouble to *undefine* them at the end:

^code undef-read-constant (1 before, 1 after)

<aside name="macro">

This is an uncommon way to use the C preprocessor, but it works. Right now,
these macros *could* be at the top level, since they only refer to `vm`, which
is itself a module-level variable. But later, these macros will refer to
variables local to `run()` itself and then this scoping will make more sense.

Even then, we could still define the macros outside of the function -- the C
preprocessor doesn't care about C scope at all and is happy to let you define
macros that refer to things that aren't in scope. But C is a language that
punishes careless use, so we'll try to be fastidious.

</aside>

If you run clox now, it executes the chunk we hand-authored in the last chapter
and spits out `1.2` to your terminal. We can see that it's working, but that's
only because our implementation of `OP_CONSTANT` has temporary code to log the
value. Once that instruction is doing what it's supposed to do and plumbing that
constant along to other places that want to consume it, the VM will become a
black box. That can make our lives as VM implementers harder.

To help ourselves out, we'll add optional diagnostic logging to the VM like we
did with chunks themselves. In fact, we'll even reuse the same code. We don't
want this logging enabled all the time -- it's just for we VM hackers, not Lox
users -- so first we'll define a flag to hide it behind:

^code define-debug-trace (1 before, 2 after)

When this flag is defined the VM with disassemble and print each instruction
right before it executes it. Where our previous disassembler walk an entire
chunk once, statically, this disassembles instructions on the fly:

^code trace-execution (1 before, 1 after)

Since `disassembleInstruction()` takes an integer byte *offset* and we store the
current instruction reference as a direct pointer, we first do a little pointer
math to convert `ip` back to a relative offset from the beginning of the
bytecode. Then we simply disassemble the instruction starting at that byte.

Of course, we also need the declaration of the function before we can compile
it:

^code vm-include-debug (1 before, 1 after)

I know this code isn't super impressive -- it's literally a switch statement
wrapped in a for loop but, believe it or not, this one of the two major
components of our VM. With this, we can imperatively execute instructions.

## A Value Stack Manipulator

If all our VM did was produce side effects, we'd be done. But, of course, Lox
has expressions that produce, modify, and consume values. Thus our compiled
bytecode needs a way to shuttle values around between the different instructions
that need them. For example, in:

---

```lox
print 3 - 2;
```

We obviously need instructions for the constants 3 and 2, the print statement,
and the substraction. But how does the subtraction know that 3 is the <span
name="word">minuend</span> and 2 is the subtrahend? How does the print
instruction know to print the result of that?

<aside name="word">

Yeah, I had to look those up in a dictionary. But aren't they delightful words?
"Minuend" sounds like some kind of Elizabethan dance and "subtrahend" might be
some sort of underground Paleolithic monument.

</aside>

To put a finer point on it, look at this thing right here:

```lox
fun echo(n) {
  print n;
  return n;
}

print echo(echo(1) + echo(2)) + echo(echo(4) + echo(5));
```

I wrapped each subexpression in a call to `echo()` that prints and returns its
argument. That side effect means that the exact order of operations is visible.

**todo: illustrate ast**

Don't worry about the VM for a minute. Think about just the semantics of Lox
itself. The operands to an arithmetic operator obviously need to be evaluated
before we can perform the operation itself. (It's pretty hard to add `a + b` if
you don't know what `a` and `b` are yet.) We haven't specified this yet, but
I'll go ahead and <span name="undefined">declare</span> that in Lox, the
left-hand side of a binary operator is evaluated before the right.

<aside name="undefined">

We could leave this unspecified and say that it's up to each implementation to
decide which order to evaluate things. That leaves the door open for optimizing
compilers to reorder arithmetic expressions for efficiency, even in cases where
the operands have visible side effects. C and Scheme leave it unspecified. Java
specifies left-to-right evaluation, like we do for Lox.

I think nailing down stuff like this is generally better for users. Otherwise,
when things are not evaluated in the order they intuit -- possibly in different
orders across different implementations! -- it can be a burning hellscape of
pain to figure out what's going on.

</aside>

Given left-to-right evaluation, and the way the expressions our nested, any
correct Lox implementation *must* print these numbers in this order:

```lox
1  // echo(1)
2  // echo(2)
3  // echo(1 + 2)
4  // echo(4)
5  // echo(5)
9  // echo(4 + 5)
12 // print 3 + 9
```

Our old jlox interpreter accomplishes this by recursively traversing the AST. It
does a post-order traversal. First it recurses down the left operand branch,
then the right operand, then finally it evaluates the node itself.

After it evaluates the left operand, it needs to store that result somewhere
temporarily while it's busy traversing down through the right operand tree. In
jlox, we used a local variable in Java to store that. Our recursive tree-walk
interpreter creates a unique Java call frame for each node being evaluated, so
we could have as many of these local variables as we needed.

Our evaluator now does a linear walk over the instructions, so we don't have the
luxury of using C local variables. How and where should we store these temporary
values? You can probably <span name="guess">guess</span> already, but I want to
really drill into this because it's an aspect of programming that we take for
granted, but we rarely learn *why* computers are architected this way.

<aside name="guess">

Hint: it's in the name of this section, and it's how Java and C manage recursive
calls to functions.

</aside>

Let's do a weird exercise. We'll step through the execution of the above program
a step at a time. Whenever a value is produced, we'll note it, and also when it
later gets consumed by some other operation. Between those two points, we'll
track the lifetime of the value. This shows us which state we need to hang onto
when:

**todo: draw?**

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

On the left is the bit of the program that just executed. On the right are the
values we're tracking. A number is when a value is first produced -- either a
constant or the result of an addition. A vertical bar tracks when a
previously-produced value needs to be kept around. An asterisk is when it gets
consumed by an operation.

As you step through, you see values appear and then later get eaten. The
interesting ones are the values that end up as the left-hand side of an
addition. Those stick around while we are working through the right-hand operand
expression.

In the above diagram, I gave each unique number its own visual column. Let's be
a little more parsimonious. Once a number is consumed, we'll allow its slot to
be reused for another later value. In other words, we take all of those empty
areas up there and shift over numbers from the right the fill them in:

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

There's some interesting stuff going on here. When we shifted everything over,
each number still managed to stay in a single column for its entire life. Also,
there are no gaps left. In other words, if a number appears early than another,
then it will live at least as long as that second one. The first number to
appear is the last to be consumed. Hmm... last-in, first-out... Why, that's a
stack!

In the second diagram here, each time we introduce a number, we push it onto the
stack from the right. When numbers are consumed, they are always popped off from
rightmost to left.

Since the temporary values we need to track naturally have stack-like behavior,
our VM will use a stack to manage them. When an instruction "produces" a value,
it pushes it onto a stack. When it needs to consume one or more values, it gets
them by popping them off the stack.

**todo: illustrate stack**

---

### The VM's Stack

I know maybe it doesn't seem like a revelation, but I *love* stack-based VMs.
You know how when you first see a magic trick, it fills you with wonder? But
then when you learn how it works, it's usually some mechanical artifice or
distraction, and the bubble bursts and the magic is gone.

There are a <span name="wonder">couple</span> of ideas in computer science that
had that sense of wonder for me and even after I pulled them apart and learned
all the ins and outs, some of that sparkle of magic remains. Stack-based VMs are
one of those.

<aside name="wonder">

Heaps -- the data structure, not the memory management thing -- are another. And
Vaughan Pratt's top-down operator precedence parsing scheme, which we'll learn
about [in due time][pratt].

[pratt]: compiling-expressions.html

</aside>

As you'll see in this chapter, executing instructions in a stack-based VM is
dead simple. In later chapters, you'll also discover that compiling a source
language to a stack-based instruction set is equally easy. And, yet, it's still
fast enough to be useful in real language implementations. It almost feels like
<span name="cheat">cheating</span> at the programming language game.

<aside name="cheat">

To take a bit of the sheen off: stack-based instruction sets aren't a panacea.
The instructions are small and simple (good!) but it means the VM has a fairly
large overhead spent dispatching instructions and manipulating the stack. This
is why the Lua folks eventually [switched to a register-based instruction
set][lua] (PDF).

[lua]: https://www.lua.org/doc/jucs05.pdf

</aside>

We covered the general idea, so now let's get concrete. The VM stores a stack of
values:

^code vm-stack (1 before, 1 after)

We're implementing the stack ourselve using a primitive C array. The bottom of
the stack -- the first pushed value and the last to be popped -- is at element
zero in the array, and later pushed values follow it.

**todo: illustrate**

Since the stack grows and shrinks as values are pushed and popped, we need to
track where the top of the stack is in the array. We use a direct pointer to the
element instead of an integer index. It's a little faster to dereference the
pointer than calculate the offset from the index each time we need it.

The pointer points at the array element just *past* the element containing the
top value on the stack. That seems a little odd, but almost every implementation
does this. It means we can indicate that the stack is empty by pointing at
element zero in the array. If we pointing to the top element, for an empty
stack, we'd need to point at element -1. That's <span
name="defined">undefined</span> in C.

**todo: illustrate stackTop pointer into various stacks**

I remember it like this: `stackTop` points to where the next value to be pushed
will go.

<aside name="defined">

What about when the stack is *full*, you ask, Clever Reader? The C standard is
one step ahead of you. It *is* allowed and well-specified to have an array
pointer that points just past the end of the array.

</aside>

The maximum number of values we can store on the stack (for now, at least) is:

^code stack-max (1 before, 2 after)

Giving our VM a fixed stack size means it's possible for some sequence of
instructions to push too many values and run out of stack space -- the classic
"stack overflow". We could instead grow the stack dynamically as needed, but for
now we'll keep it simple. Since VM uses Value, we need to include its
declaration:

^code vm-include-value (1 before, 2 after)

Our VM has some interesting state, so we must properly initialize it:

^code call-reset-stack (1 before, 1 after)

That uses this helper function:

^code reset-stack

Since the stack array is declared directly inline in the VM struct, we don't
need to allocate it. We don't even need to clear the unused cells in the array
-- we simply won't use them. The only initialization we need is to set
`stackTop` to point to the beginning of the array to indicate that the stack is
empty.

The stack protocol supports two operations:

^code push-pop (1 before, 2 after)

You can push a new value onto the top of the stack, and you can pop the most
recently pushed value back off. Here's the first:

^code push

If you're rusty on your C pointer syntax and operations, this is good practice
to start warming up. The first line stores the value in the array element at the
top of the stack. Remember, `stackTop` points just *past* the last used element,
at the next available one. This stores the value in that slot.

Then we increment the pointer itself to point to the next unused slot in the
array now that the previous slot is full.

^code pop

Popping is the mirror image. First we move the stack pointer *back* to get to
the most recent used slot in the array. Then we look up the value at that
position in the array and return it. We don't need to explicitly "remove" it
from the array -- moving `stackTop` down is enough to mark that slot as no
longer in use.

**todo: illustrate operations and stackTop pointer**

We have a working stack, but it's hard to *see* that it's working. When we start
implementing more complex instructions and compiling and running larger pieces
of code, we'll end up with a lot of values crammed into that stack array. It
would make our lives as VM hackers easier if we had some visibility into the
stack.

To that end, whenever we're tracing instructions, we'll also trace the current
contents of the stack before we execute each instruction:

^code trace-stack (1 before, 1 after)

We loop, printing each value in the array, starting at the first (bottom of the
stack) and ending when we reach the top. This lets us observe the VM's execution
one step at a time, and see the effect of each step on the stack. The output is
pretty verbose, but it's useful when we're tracking down a bug deep in the
sewers of the interpreter.

Stack in hand, let's revisit our two instructions. First up:

^code push-constant (1 before, 1 after)

In the last chapter, I was hand-wavey about how the OP_CONSTANT instruction
"loads" a constant. Now that we have a stack you know what it means to actually
*load* one: it gets pushed onto the stack.

^code print-return (1 before, 1 after)

Then we make `OP_RETURN` pop the stack and print the top value before exiting.
When we add support for real functions to Lox, we'll change this code. But, for
now, it gives us a way to get the VM executing simple instruction sequences and
displaying the result.

## An Arithmetic Machine

The heart and soul of our VM are in place now. The bytecode loop dispatches and
executes instructions. The stack grows and shrinks as values flow through it.
The two pieces function, but it's hard to get a feel for how cleverly they
interact with only the two dumb instructions we have so far. So let's teach our
interpreter to do arithmetic.

We'll start with the simplest arithmetic operation, unary negation:

```lox
var a = 123;
print -a; // -123.
```

The prefix `-` operator takes one operand, the value to negate. It produces a
single result. We aren't fussing with the syntax yet, but we can add the
bytecode instruction the syntax will compile to:

^code negate-op (1 before, 1 after)

We execute it like so:

^code op-negate (1 before, 1 after)

It needs a value to operate on, so it gets it by popping it from the stack. It
negates that, then pushes the result back on for later instructions to use. We
complete the job by adding it to the disassembler too:

^code disassemble-negate (2 before, 1 after)

We can see it in action by adding a negation to our test chunk:

^code main-negate (1 before, 2 after)

After it loads the constant, but before it returns, it executes the negation
instruction. That replaces the constant on the stack with its negation. Then the
return instruction prints that out:

```text
-1.2
```

### Binary operators

OK, unary operators aren't that impressive. We still only ever have a single
value on the stack. To really see some depth, we need binary operators. Lox has
four binary <span name="ops">arithmetic</span> operators: addition, subtraction,
multiplication, and division. We'll go ahead and implement them all at the same
time:

<aside name="ops">

Other languages have modulo, bit shifting, bitwise arithmetic, etc. but I pared
it down for Lox. Lox does have some other binary operators -- comparison and
equality -- but those don't produce numbers as a result, so we aren't ready to
implement them yet.

</aside>

^code binary-ops (1 before, 1 after)

Back in the bytecode loop, they look like:

^code op-binary (1 before, 1 after)

The only difference between these four operators is which underlying C operator
they ultimately use to combine the two operands. Surrounding that is some
boilerplate code to pull values off the stack and push the result. In fact, when
we later add dynamic typing, that boilerplate will grow. To avoid repeating all
of that code four times, I wrapped it up in a macro:

^code binary-op (1 before, 1 after)

I admit this is a fairly <span name="macro">adventurous</span> use of the C
preprocessor. I hesitated to do it like this, but I think it ends up being worth
when we get to later chapters and also need to add the type checking for each
operand and stuff. It would be a chore to walk you through the same code four
times.

<aside name="macro">

Did you even know you can pass an *operator* as an argument to a macro? Now you
do. The preprocessor doesn't care that operators aren't first class in C. As far
as it's concerned, it's all just text tokens.

I know, you can just *feel* the temptation to abuse this, can't you?

</aside>

If you aren't familiar with the trick already, that outer do-while loop probably
looks really weird. This macro needs to expand to a series of statements. To be
careful macro authors, we want to ensure those statements all end up in the same
context where the macro is expanded. Imagine if you defined:

```c
#define WAKE_UP() makeCoffee(); drinkCoffee();
```

And then used it like:

```c
if (morning) WAKE_UP();
```

The goal is to execute the entire macro body only if `morning` is true. But it
expands to:

```c
if (morning) makeCoffee(); drinkCoffee();
```

Oops. The `if` only attaches to the first statement. You might think you can fix
this using a block:

```c
#define WAKE_UP() { makeCoffee(); drinkCoffee(); }
```

That's better, but you still risk:

```c
if (morning)
  WAKE_UP();
else
  sleepIn();
```

Now you get a compile error on the `else` because of that trailing `;` after the
macro's block. Using a do-while loop in the macro looks funny, but it gives you
a way to contain multiple statements inside a block that *also* permits a
semicolon at the end.

Where were we? Right, so what the body of that macro does is straightforward. A
binary operator takes two operands, so it pops twice. It performs the operation
on those two values and then pushes the result.

Pay close attention to the *order* of the two pops. Note that we assign the
first popped operand to `b`, not `a`. It looks backwards. When the operands
themselves are calculated, the left is evaluated first, then the right. That
means the left operand gets pushed before the right operand. So the right
operand will be on top of the stack. Thus, the first value we pop is `b`.

**todo: illustrate**

As we did with the other macros inside `run()`, we clean up after ourselves at
the end of the function:

^code undef-binary-op (1 before, 1 after)

Last is disassembler support:

^code disassemble-binary (2 before, 1 after)

---

The arithmetic instructions are simple, like OP_RETURN. Even though the
arithmetic *operators* take operands -- which are found on the stack -- the
arithmetic *bytecode instructions* do not.

Let's put some of our new instructions through their paces by evaluating a
larger expression:

**todo: show expr ast**

^code main-chunk (1 before, 2 after)

The addition goes first. The instruction for the left constant, 1.2, is already
there, so we add another for 3.4. Then we add those two using OP_ADD, leaving it
on the stack. That covers the left side of the subtraction. Next we push the
5.6, and finally subtract it from the result of the addition.

Note how the result of the OP_ADD implicitly flows into being an operand of
OP_SUBTRACT without either instruction being directly coupled to each other.
That's the magic of the stack. It lets us freely compose instructions together
without them needing any complexity or awareness of the data flow. The stack is
acts like a shared workspace that they all read from and write to.

In this tiny example chunk, the stack still only gets two values tall, but when
we start compiling Lox source to bytecode, we'll have chunks that use much more
of the stack. In the meantime, try playing around with this hand-authored chunk
to calculate different nested arithmetic expressions and see how values flow
through the instructions and stack.

You may as well get it out of your system now. This is the last chunk we'll
build by hand. When we next revisit bytecode, we will be writing a compiler to
generate it for us.

<div class="challenges">

## Challenges

1.  What bytecode instruction sequence would you generate to evaluate:

        :::lox
        1 + 2 * 3 - 4 / -5

    Can you come up with multiple instruction sequences that calculate that
    expression?

1.  If we really wanted a minimal instruction set, we could eliminate either
    OP_NEGATE or OP_SUBTRACT. Show the bytecode instruction sequence you would
    generate for:

        :::lox
        4 - 3 * -2

    First, without using OP_NEGATE. Then, without using OP_SUBTRACT.

    Given the above, do you think it makes sense to have both instructions? Why
    or why not? Are there any other redundant instructions you would consider
    adding?

1.  Our VM's stack is has a fixed-size, and we don't check that pushing a value
    does not overflow it. This means the wrong series of instructions could
    cause our interpreter to crash or go into undefined behavior. Avoid that by
    dynamically growing the stack as needed.

    What are the costs and benefits of doing so?

</div>

<div class="design-note">

## Design Note: Register-Based Bytecode

- register-based vm

- spend book on stack based
- other family bytecode, register based
- despite name, not exactly like machine reg
- machine fixed number
- shared across fn calls
- spend lot of time moving in and out of reg

- reg bytecode higher level
https://en.wikipedia.org/wiki/Register_window

- still have stack
- but inst can read inputs from anywhere in stack
- can write result anywhere
- most instr have operands for "register" index into stack where value

- ex:

```lox
var a = 1;
var b = 2;
var c = a + b;
```

- in clox, last statement bytecode

```lox
local a : read variable and push onto stack
local b : read variable and push onto stack
add     : pop two, add, push result
store c : pop stack and store in local variable
```

- four insts, four times through interp loop, four decode and dispatches
- at 7 byte - 4 for opcodes, 3 to identify locals

- in reg lang, locals accessible as reg
- add instr reads directly from reg and writes to
- has three operands for which regs to read and write

```lox
add <operand a> <operand b> <result>
```

- one instr with three operands
- one decode and dispatch
- no stack manip
- decode more complex because of operands, but still less work

- lua got faster when switched to reg-based
- neat model
- more complex to compile for
- still need push/pop stack for temporaries
- if generate non-optimal instr seq, can lose benefits
- too complex for book
- good next step to learn about
- stepping stone to native code

- nice thing is bytecode usually impl detail of vm
- can switch from one to another without breaking users
- lua did

- link to lua paper

</div>
