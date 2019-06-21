^title Calls and Functions
^part A Bytecode Virtual Machine

> Any problem in computer science can be solved with another level of
> indirection. Except for the problem of too many layers of indirection.
>
> <cite>David Wheeler</cite>

OK, this is a big chapter. I try to break things down into incremental steps,
but sometimes you gotta just hike the whole trail. The next major piece is
adding functions to clox. We could just add function declarations, but that's
not very useful if you can't call them. We could add calls, but there's nothing
to call. And all of the runtime support needed in the VM to support both of
those isn't very rewarding if they aren't hooked up to anything you can see. So
we're going to do it all. It's a lot, but it will feel good when it's in and
working.

## Function Objects

The most interesting structural change in the VM we'll make is around the stack.
We already have a stack for local variables and temporaries, so we're partway
there. But we have no notion of a *call* stack. Before we can make much
progress, we'll have to address that. But first, let's write some code. I always
feel better once I start moving.

We can't do much without having some kind of representation for functions, so
we'll start there. From the VM's perspective, what is a function?

It has a body of code that can be executed, so that means some bytecode. We
could compile the entire program and all of its function declarations into one
big monolithic chunk. Each function would have a pointer to the first
instruction of its code inside the chunk. We'd have a single constant pool for
the entire program, which means we'd need larger operands to refer to them.

This is roughly how compilation to native code works where the end result really
is one big blob of machine code. But for our bytecode VM, we can do something a
little higher level. I think a cleaner, simpler model is to give each function
its own Chunk. We'll want some other metadata too, so let's go ahead and stuff
it all in a struct now:

^code obj-function (2 before, 2 after)

Functions are first class in Lox, so they need to be actual Lox objects. Thus it
has the same `Obj obj` header all object types share. The `arity` field stores
the number of parameters the function expects. Then, in addition to the chunk,
we store the function's <span name="name">name</span>. That will be handy for
reporting readable runtime errors.

<aside name="name">

For some reason, humans don't seem to find numeric bytecode offsets particularly
illumating in crash dumps.

</aside>

This is the first time the "object" module has needed to reference Chunk, so we
need an include:

^code object-include-chunk (1 before, 1 after)

Like we did with the other object types, we've got some accoutrements to make
them a little easier to work with in C. Sort of a poor man's object orientation.
First, we'll declare a function to create a new, uh, function:

^code new-function-h (3 before, 1 after)

The implementation is over here:

^code new-function

It uses the same procedure we've seen to allocate memory and initialize the
object's header so that the VM knows what type of object it is. Instead of
passing in data to set up the function, we just initialize it in a sort of
"blank" state -- zero arity, no name, and no code. That stuff will get filled in
later after the function is created.

Since we have a new type of object, we need a new object type in the enum:

^code obj-type-function (1 before, 2 after)

When you're done with a function object, you need to return its bits it borrowed
back to the operating system:

^code free-function (1 before, 1 after)

This switch case is <span name="free-name">responsible</span> for freeing the
ObjFunction itself as well as any other memory it owns. In this case, the latter
is just the chunk, so we call Chunk's destructor-like function.

<aside name="free-name">

We don't need to explicitly free the function's name because it's an ObjString.
That means we can let the garbage collector manage its lifetime for us. Or, at
least, we'll be able to once we [implement a garbage collector][gc].

[gc]: garbage-collection.html

</aside>

Over in the function to print objects, we also have to add a case:

^code print-function (1 before, 1 after)

This shows the function's name. For example:

```lox
fun someFunction() {}

print someFunction;
```

Run that and it prints out `<fn someFunction>`. I don't know, maybe that will be
useful to someone?

Finally, we have a couple of macros for converting values to functions. To make
sure your value actually *is* a function:

^code is-function (2 before, 1 after)

Assuming that returns true, you can then safely cast the Value to an ObjFunction
pointer using:

^code as-function (2 before, 1 after)

With that, our object model knows how to represent functions. You should feel
warmed up now, so let's move on to something a little harder.

## Compiling to Function Objects

Right now, our compiler assumes it is always compiling to one single chunk. With
a chunk for each function, the compiler needs to switch to that function's chunk
before compiling its body. At the end of the function body, the compiler needs
to return to the previous chunk it was working with.

That's fine for code inside function bodies, but what about code that isn't? The
"top level" of a Lox program is also imperative code and we need a chunk to
compile that into. We can simplify the compiler and VM by placing that top level
code inside an implicitly-defined function too. That way, the compiler is always
within some kind of function body, and the VM always runs code by invoking a
function. It's as if the entire program lives inside an automatic `main()`
function.

Before we get to user-defined functions, then, let's do the reorganization to
support that implicit function. It starts with the Compiler struct. Instead of
pointing directly to a Chunk that it writes to, it will instead have a reference
to the current function object it is building:

^code function-fields (1 before, 1 after)

We also have a little FunctionType enum. This lets the compiler tell when it's
compiling top level code versus the body of a function. Most of the compiler
doesn't care about this -- that's why it's a useful abstraction -- but in one or
two places the distinction is meaningful. We'll get to one later.

^code function-type-enum

Every place in the compiler that was writing to the Chunk now needs to go
through that function pointer. Fortunately, many <span
name="current">chapters</span> ago, we encapsulated in the `currentChunk()`
helper function. We only need to fix that and the rest of the compiler is happy:

<aside name="current">

It's almost like I had a crystal ball that could see into the future and knew
we'd need to change the code later. But, really, it's because I wrote all the
code for the book before any of the text.

</aside>

^code current-chunk (2 before, 2 after)

The current chunk is always simply the chunk owned by the function we're in the
middle of compiling. Next, we need to actually create that function. Previously,
the VM passed a Chunk into the compiler for it to fill with code. Instead, the
compiler will create and return a function that contains the compiled top-level
code -- which is all we support right now -- of the user's program.

We start threading this through in `compile()`, which is the main entry point
into the compiler:

^code call-init-compiler (1 before, 2 after)

There are a bunch of changes in how the compiler is initialized. First, we clear
out the new Compiler fields:

^code init-compiler (1 after)

Then we allocate a new function object to compile into:

^code init-function (1 before, 1 after)

<span name="null"></span>

<aside name="null">

I know, it looks dumb to null the `function` field only to immediately assign it
a value a few lines later. This is one of those weird corners of the VM that
will make more sense once we have a garbage collector. We clear the field to
ensure that if a collection happens when allocating the function, the GC doesn't
see the uninitialized field.

</aside>

This might seem a little strange. A function object is the *runtime*
representation of a function, but here we are creating it at compile time. The
way to think of it is that a function is similar to string and number literals.
It forms a bridge between the compile time and runtime world. When we get to
function declarations, those really *are* literals -- they are a built in syntax
that creates a value of a built-in type.

So the <span name="closure">compiler</span> creates function objects during
compilation. Then, at runtime, they are simply invoked.

<aside name="closure">

We can create the function objects at compile time because they contain only
data that is available at compile time. The function's code, name, and arity
are all fixed. When we add closures in the [next chapter][closures], which
capture variables at runtime, the story gets more complex.

[closures]: closures.html

</aside>

Here is another strange bit of code:

^code init-function-slot (1 before, 1 after)

Remember that the compiler's `locals` array keeps track of which stack slots are
associated with which local variables or temporaries. From now on, the compiler
implicitly claims stack slot zero for the VM's own internal use. We give it an
empty name so that the user can't write an identifier that refers to it. I'll
explain what this is about when it becomes useful.

That's initializing the compiler. We also need a couple of changes in the other
end when we finish compiling some code:

^code end-compiler (1 after)

Right now, when `interpret()` calls into the compiler, it passes in a Chunk to
be written to. Now that the compiler creates the function object itself, we
return that function. We grab it from the current compiler:

^code end-function (1 before, 1 after)

And then returns it to `compile()`:

^code return-function (1 before, 1 after)

Now's a good time to make another tweak in this function. Earlier, we added some
diagnostic code to have the VM dump the disassembled bytecode so we can debug
the compiler. We need to fix that now that the generated chunk is wrapped in a
function:

^code disassemble-end (2 before, 2 after)

Bumping up a level to `compile()`, we adjust its signature:

^code compile-h (2 before, 2 after)

Instead of taking a chunk, now it returns a function. Over in the
implementation:

^code compile-signature (1 after)

Finally we get to some actual code. At the very end of the function:

^code call-end-compiler (4 before, 1 after)

We get the function object from the compiler. If there were no compile errors,
we return it. Otherwise, we signal an error by returning `NULL`. This way, the
VM doesn't try to work with a function that may contain invalid bytecode.

Eventually, we will update `interpret()` to handle the new declaration of
`compile()`, but first we have some other more significant changes to make.

## Call Frames

It's time for a big conceptual leap. Before we can implement function
declarations and calls, we need to get the VM ready to handle them. There are
two main problems we need to worry about:

### Allocating local variables

The compiler allocates stack slots for local variables. How should that work
when the set of local variables in a program is distributed across multiple
functions?

One option would be to keep them totally separate. Each function would get its
own dedicate set of slots in the VM stack that it would own <span
name="static">forever</span>, even when the function isn't in the middle of
being called. Each local variable in the entire program would have a bit of
memory in the VM that it keeps to itself.

<aside name="static">

It's basically what you'd get if you declared every local variable in a C
program using `static`.

</aside>

Believe it or not, early programming language implementations worked this way.
The first Fortran compilers statically allocated memory for each variable. The
obvious problem is that it's really inefficient. Most functions are not in the
middle of being called at any point in time, so sitting on unused memory for
them is wasteful.

The more fundamental problem, though, is <span
name="recursion">recursion</span>. With recursion, you can be "in" multiple
calls of the same function at the same time. Each needs its own memory for its
local variables.

<aside name="fortran">

Early versions of Fortran avoided this problem by disallowing recursion
entirely. Recursion was considered an advanced, esoteric feature at the time.

</aside>

In jlox, we solved this by dynamically allocating memory for an environment each
time a function was called or a block entered. In clox, we don't want that kind
of performance cost on every function call. Instead, our solution has a bit of
the flavor of Fortran's static allocation and a bit of something more dynamic.

The value array in the VM works on the observation that local variables and
temporaries have stack semantics. Fortunately for us, that's still true even
when you add function calls into the mix. Here's an example:

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

**todo: illustrate flow**

As execution flows through the two calls, every local variable obeys the
principle that any variable declared later will be discarded before the variable
itself needs to be. This is true even across calls. We know we'll be done with
`c` and `d` before we are `a`. So we should still be able to allocate local
variables on the VM's value stack.

Ideally, we still determine *where* on the stack each variable will go at
compile time. That keeps the bytecode instructions for working with variables
simple and fast. In the above example, we could <span
name="imagime">imagine</span> doing so in a straightforward way:

<aside name="imagine">

I say "imagine" because the compiler can't actually figure this out. Because
functions are first class in Lox, we can't determine which functions call which
others at compile time.

</aside>

**todo: illustrate that each local goes in one slot**

But that doesn't always work out. Consider:

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

In the first call to `second()`, `c` and `d` would go into slots 1 and 2. But in
the second call, we need to have made room for `b`, so `c` and `d` need to be in
slots 2 and 3. Thus the compiler can't pin down an exact slot for each local
variable across function calls. But *within* a given function, the *relative*
locations of each local variable are fixed. Variable `d` is always in the slot
right after `c`. This is the key insight.

When a function is called, we don't know where the top of the stack will be
because it can be called from different contexts. But, wherever that top happens
to be, we do know where all of the function's local variables will be relative
to that top. So, like many problems, we solve our allocation problem with a
level of indirection.

At the beginning of each function call, the VM records the location of the first
slot where that function's own locals begin. The instructions for working with
local variables access them by an offset relative to that. At compile time, we
calculate those offsets. At runtime, we just need to adjust the offset by the
location we recorded when the function call started.

**todo: illustrate window**

It's as if the function gets a "window" or "frame" within the larger stack where
it can store its locals. The position of the *call frame* is determined at
runtime, but within and relative to that region, we know where to find things.
The historical name for this recorded location where the function's locals start
is a "frame pointer" because it points to the beginning of the function's call
frame. Sometimes you hear "base pointer", because it points to the base stack
slot on top of which all of the function's variables live.

That's our first piece of data we need to track. Every time we call a function,
the VM needs to track of the first stack slot where that function's variables
begin.

### Return addresses

Right now, the VM works its way through instruction sequence by incrementing the
`ip` field. The only interesting logic in there is the control flow instructions
which offset the `ip` by larger amounts. *Calling* a function is pretty
straightforward -- simply set `ip` to point to the first instruction in that
function's chunk. But what about when the function is done?

The VM needs to return back to the chunk where the function was called from and
resume execution at the instruction immediately after the call. This, for each
function call, we need to track where we jump back to when the call completes.
This is called a "return address" because it's the address of the instruction
that the VM switches to after the call.

Again, thanks to <span name="return">recursion</span>, there may be multiple
return addresses for a single function, so this is a property of each
*invocation* and not the function itself.

**todo: illustrate code with arrows pointing back to where it returns**

<aside name="return">

The authors of early Fortran compilers had a clever trick for implementing
return addresses. Since they *didn't* support recursion, any given function only
needed a single return address at any point in time. So when a function was
called at runtime, the program would *modify its own code* to change a jump
instruction at the end of the function to jump back to its caller. Sometimes the
line between genius and madness is hair thin.

</aside>

### The call stack

So for each live function invocation -- each call that hasn't returned yet -- we
need to track where on the stack that function's locals begin, and where the
caller should resume. We'll put this, along with some other stuff, in a new
struct:

^code call-frame (1 before, 2 after)

Each CallFrame represents a single ongoing function call. The `slots` field
points directly into the VM's value stack. It points to the first slot that this
function can use. I gave it a plural name because -- thanks to C's weird
"pointers are sort of arrays" thing -- we'll treat it like an array.

The implementation of return addresses is a little different from what I
described above. Instead of storing the return address in the callee's frame,
the caller stores its own `ip`. When we return from a function, the VM will jump
to the `ip` of the caller's CallFrame and resume from there.

I also stuffed a pointer to the function being called in here. We'll use that to
look up constants in its chunk and for a few other things.

Each time a function is called, we need to create one of these structs. We could
<span name="heap">dynamically</span> allocate them on the heap, but that's slow.
Function calls are a core operation, so they need to be as fast as possible.
Fortunately, we can make the same observation we made for variables: function
calls have stack semantics. If `first()` calls `second()`, the call to
`second()` must complete because `first()` does.

<aside name="heap">

Many Lisp implementations dynamically allocate stack frames because it
simplifies implementing continuations.

</aside>

So over in the VM, we can create an array of these CallFrame structs up front
and treat it like a stack, just like we do with the Value array:

^code frame-array (1 before, 1 after)

This array replaces the `chunk` and `ip` fields we used to have directly in the
VM. Now each CallFrame has its own `ip` and its own pointer to the ObjFunction
that it's executing. From there, we can get to the function's chunk.

The new `frameCount` field in the VM stores the current height of the stack -- the
number of ongoing function calls. To keep clox simple, the array's capacity is
fixed. This means, as in many language implementations, there is a maximum call
depth we can handle. For clox, it's:

^code frame-max (2 before, 2 after)

We also define the value stack's size in terms of that to make sure we have
plenty of stack slots even in very deep call trees. When the VM starts up, the
CallFrame stack is empty:

^code reset-frame-count (1 before, 1 after)

Now, we've got some grunt work ahead of us. We've moved `ip` out of the VM
struct and into CallFrame. We need to fix every bit of code in the VM that
touches `ip` to handle that. Also, the instructions that access local variables
by stack slot need to be updated to do so relative to the current CallFrame's
`slots` field.

We'll start at the top and plow through it:

^code run (1 before, 1 after)

First, we store the curreent topmost CallFrame in a <span
name="local">local</span> variable inside the main bytecode execution function.
We replace the macros for reading bytecode with versions that read from that
frame's `ip` since that will reliably point to the current chunk being executed.

<aside name="local">

We could access the current frame by going through the CallFrame array every
time, but that's verbose. More importantly, storing the frame in a local
variable encourages the C compiler to keep that pointer in a register. That
speeds up access to the frame's `ip`. There's no *guarantee* that the compiler
will do this, but there's a good chance it will.

</aside>

Now onto each instruction:

^code push-local (2 before, 1 after)

Previously, this read the given local slot directly from the VM's stack array,
which meant it read the given slot starting at the very bottom of the stack.
Now, it access's the current frame's `slots` array, which means it accesses the
given numbered slot relative to the beginning of that frame.

Setting a local variable works the same way:

^code set-local (2 before, 1 after)

The jump instructions used to modify the VM's `ip` field. Now, they do the same
for the current frame's `ip`:

^code jump (2 before, 1 after)

And the conditional jump:

^code jump-if-false (2 before, 1 after)

And our backwards-jumping loop instruction:

^code loop (2 before, 1 after)

We have some diagnostic code that prints each instruction as it executes to help
us debug our VM. That needs to work with the new structure too:

^code trace-execution (1 before, 1 after)

Instead of passing in the VM's `chunk` and `ip` fields, now we read from the
current CallFrame.

You know, that wasn't too bad, actually. Most instructions just use the macros
so didn't need to be touched. Next, we jump up a level to the code that calls
`run()`:

^code interpret-stub (1 before, 2 after)

We finally get to wire up our earlier compiler changes to the back end changes
we just made. First we pass the source code to the compiler. It returns us the
ObjFunction it created containing the compiled top level code. If it returns
`NULL`, it means there was some compile-time error which it has already
reported. In that case, we bail out since we can't run anything.

Otherwise, we set up the prepare an initial CallFrame to execute that function's
code. We point to the function, initialize frame's the `ip` to point to the
beginning of the function's bytecode, and set up its stack window to start at
the very bottom of the VM's value stack.

This gets the interpreter ready to start executing code. When it finishes, it
used to free the hardcoded chunk in the VM. Now that the ObjFunction owns that
code, we don't need to do that anymore, so the end of `interpret()` is simply:

^code end-interpret (2 before, 1 after)

Assuming we did all of that correctly, we finally got clox back to a runnable
state. Fire it up and it does... exactly what it did before. We haven't added
any new features yet. But all of the infrastructure is there ready for us now.
Let's take advantage of it.

## Function Declarations

Before we can do call expressions, we need something to call, so we'll do
function declarations first. The <span name="fun">fun</span> gets started with
a keyword:

<aside name="fun">

Yes, I am proud of myself for this, thank you for asking.

</aside>

^code match-fun (1 before, 1 after)

That passes off control to:

^code fun-declaration

Functions are first class values and a function declaration simply creates and
stores one in a newly-declared variable. So we parse the name just like any
other variable declaration. A function declaration at the top level will bind
the function to a global variable. Inside a block or other function, a function
declaration creates a local variable.

In an earlier chapter, I explained how variables [get defined in two
stages][stage]. This ensures you can't access a variable's value inside the
variable's own initializer. That would be bad because the variable doesn't
*have* a value yet.

[stage]: local-variables.html#another-scope-edge-case

Functions don't suffer from this problem. It's safe for a function to refer to
its own name inside its body. You can't *call* the function and execute the body
until after it's fully defined, so you'll never see the variable in an
uninitialized state. Practically speaking, it's useful to allow this in order to
support recursive local functions.

To make that work, we mark the function declaration's variable as initialized as
soon as we compile the name, before we call the body. That way it can be
accessed inside the body without generating an error.

Next, we compile the function itself -- its parameter list and block body. For
that, we use a separate helper function. That function generates code that
leaves the resulting function object on top of the stack. After that, we call
`defineVariable()` to store that function back into the variable we declared for
it.

We use a separate helper function to compile the parameters and body because
we'll reuse it later for parsing method declarations inside classes. Let's build
it incrementally, starting with this:

^code compile-function

For now, we won't worry about parameters. It parses an empty pair of parentheses
followed by the body. The body starts with a left curly brace, which we parse
here. Then we call our existing `block()` function which knows how to compile
the rest of a block including the closing brace.

The interesting part is the compiler stuff. The Compiler struct stores data like
which slots are owned by which local variables, how many blocks of nesting we're
currently in, etc. All of that is specific to a single function. But now the
front end needs to handle compiling multiple functions <span
name="nested">nested</span> within each other. How do we manage that?

<aside name="nested">

Remember that the compiler treats top level code as the body of an implicit
function, so as soon as we add *any* function declarations, we're in a world of
nested functions.

</aside>

The trick, as you can see, is to create a separate Compiler for each function
being compiled. When we start compiling a function declaration, we create a new
Compiler on the C stack and initialize it. `initCompiler()` sets it to be the
current function, so when we compile the body, it will write to the chunk owned
by the new compiler's function.

After we reach the end of the function's block body, we call `endCompiler()`.
That yields the newly compiled function object. We then store that as a constant
in the surrounding function's constant table. But, wait, how do we get back to
the surrounding function? We lost it when `initCompiler()` overwrote the current
compiler pointer.

We fix that by treating the series of nested Compiler structs as a stack. Unlike
the Value and CallFrame stacks in the VM, we won't use an array. Instead, we use
a linked list. Each Compiler points back to the Compiler for the function that
encloses it, all the way back to the root Compiler for the top level code:

^code enclosing-field (1 before, 1 after)

When initializing a new compiler, we capture the current one in that pointer:

^code store-enclosing (1 before, 1 after)

Then when a compiler finishes, it pops itself off the stack by restoring the
previous compiler as the current one:

^code restore-enclosing (4 before, 1 after)

Because our compiler uses <span name="compiler">recursive</span> descent and
these Compiler structs are stored right on the C stack, we don't even need to
dynamically allocate them.

<aside name="compiler">

Using the native stack for Compiler structs does mean our compiler has a
practical limit on how deeply nested function declarations can be. Go too far
and you could overflow the C stack. If we want the compiler to be more robust
against pathological or even potentially malicious code -- a very real concern
for tools like JavaScript VMs -- it would be good to have our compiler
artificially limit the amount of function nesting it permits.

</aside>

Functions aren't very useful if you can't pass them parameters, so let's do that
next:

^code parameters (1 before, 1 after)

Semantically, a parameter is just a local variable declared in the outermost
lexical scope of the function body. We can use the existing compiler support for
declaring named local variables to parse and compile parameters. Unlike with
local variables which have initializers, there's no code here to initialize the
parameter's value. We'll see how that works later when we do argument passing in
function calls.

We also keep track of the function's arity by counting how many parameters we
parse. The other piece of metadata we store with a function is its name. When
parsing a function, we call `initCompiler()` right after we parse the function's
name. That means we can grab the name right then from the previous token:

^code init-function-name (1 before, 2 after)

Note that we're careful to create a copy of the name string. Remember, the
lexeme points directly into the original source code string. That string may be
freed once the code is finished compiling. The function object we create in the
compiler outlives the compiler and persists until runtime. So it needs its own
heap allocated name string that it can keep around.

Rad. Now we can compile function declarations, like:

```lox
fun areWeHavingItYet() {
  print "Yes we are!";
}

print areWeHavingItYet;
```

We just can't do anything <span name="useful">useful</span> with them.

<aside name="useful">

We can print them! I guess that's not very useful, though.

</aside>

## Function Calls

We're making good progress. One more section and we'll actually start to see
some interesting behavior. The next piece is being able to call functions. We
don't usually think of it this way, but a function call expression is sort of
like an infix `(` operator. You have a high precedence expression the left for
the thing being called -- usually just a single identifier. Then the `(` in the
middle, followed by the argument expressions separated by commas, then a final
`)` to wrap it up at the end.

It's a weird perspective, but it explains how to hook the syntax into our
parsing table:

^code infix-left-paren (1 before, 1 after)

When the parser encounters a left parenthesis following an expression, it calls:

^code compile-call

We've already consumed the `(` token, so next we compile the arguments using a
separate `argumentList()` helper. That returns the number of arguments it
compiled. Each argument expression will leave its value the stack in preparation
for the call. After that, we emit a new `OP_CALL` instruction to invoke the
function. We use the number of passed arguments as an operand. That way, the VM
knows how many slots on top of the stack are used for the call.

We parse the arguments using this guy:

^code argument-list

It's similar to what we wrote for jlox. It keeps chewing through arguments as
long as it finds a comma after each expression. Once it runs out, it consumes
the final closing parenthesis and we're done.

Well, almost. Back in jlox, we added a compile-time check that you don't pass
more than 255 arguments to a call. At the time, I said that was because clox
would need a similar limit. Now you understand why -- since we stuff the
argument count into the bytecode as a single-byte operand, we can only go up to
255. We need to verify that in this compiler too:

^code arg-limit (1 before, 1 after)

That's the front end. Let's skip over to the back end, with a quick stop in the
middle to declare the new instruction:

^code op-call (1 before, 1 after)

Before we get to the implementation, let's think about what the stack looks like
at the point of a call and what we need to do from there. When we reach the call
instruction, we have already executed the expression for the function being
called followed by its arguments. So, if our program looks like:

```lox
fun addThree(a, b, c) {
  print a + b + c;
}

addThree(1, 2, 3);
```

If we pause the VM right on the `OP_CALL` instruction for that call to
`addThree()`, the stack looks like this:

**todo: illustrate**

Now let's ruminate on the callee side. When the compiler compiled the
`addThree()` function itself, it automatically allocated slot zero. Then, after
that, it allocated local slots for each parameter, in order. To perform a call
to `addThree()`, we need to create a CallFrame and initialize it with the
function being called and a region of stack slots that it can use. Then we need
to collect the arguments passed to the function and get them into the
corresponding slots for the parameters.

When the VM starts executing the body of `addThree()`, we want its stack window
to look like this:

**todo: illustrate**

Do you notice how the argument slots the caller sets up and the parameter slots
the callee needs are both in exactly the right order? How convenient! This is no
coincidence. When I talked about each CallFrame having its own window into the
stack, I never said those windows would be *disjoint*.

There's nothing preventing us from overlapping them, like this:

**todo: illustrate**

<span name="lua">The</span> top of the caller's stack contains the function
being called followed by the arguments in order. We know the caller doesn't have
any other slots above those in use because any temporaries needed when
evaluating argument expressions have been discarded by now. Then the bottom of
the callee's stack overlaps so that the parameter slots exactly line up with
where the argument values already live.

<aside name="lua">

Different bytecode VMs and real CPU architectures have different *calling
conventions*, which is the specific mechanism they use to pass arguments, store
the return address, etc. The mechanism I use here is based very heavily on Lua's
very clean, fast virtual machine.

</aside>

This means that we don't need to do *any* work to "bind an argument to a
parameter". There's no copying values between slots or across environments. The
arguments automagically land exactly where they need to be. It's hard to beat
that.

Time to implement it:

---

^code interpret-call (3 before, 1 after)

We need to know the function being called and how many arguments are being
passed to it. We get the latter from the instruction's operand. That also tells
where to find the function on the stack since it appears right before the first
operand.

All of the work happens in a separate `callValue()` function. If that returns
`false`, it means the call caused some sort of runtime error. When that happens,
we abort the interpreter.

If it's successful, there will be a new frame on the CallFrame stack for the
called function. The `run()` function has its own cached pointer to the current
frame, so we need to update that:

^code update-frame-after-call (2 before, 1 after)

Since the bytecode dispatch loop reads from that `frame` variable, when it goes
to execute the next instruction, it will read the `ip` from the newly called
function and jump to its code. We set up the call frame here:

^code call-value

There's more going on here than just initializing a new CallFrame. Because Lox
is dynamically typed, there's nothing to prevent a user from writing bad code
like:

```lox
var notAFunction = 123;
notAFunction();
```

If that happens, the runtime needs to safely report an error and halt. So the
first thing we do is check the type of the value that we're trying to call like
a function. If it's not one, we error out. Otherwise, the actual call happens
here:

^code call

This simply initializes the next CallFrame on the stack. It stores a pointer to
the function being called and points the frame's `ip` to the beginning of the
function's bytecode. Finally, it sets up the `slots` pointer to give the frame
its window into the stack. The arithmetic there ensures that the arguments
already on the stack line up with the function's parameters:

**todo: illustrate window with offsets for `argCount` and 1**

The `+ 1` is a little funny. That's corresponds to local slot zero, which the
compiler automatically allocates for private use. It's a little pointless right
now, but will be useful when we get to methods later.

Let's take a quick side trip. Now that we have a nice little function for
initiating a CallFrame, we may as well use it to set up the first frame for
executing the top level code:

^code interpret (2 before, 2 after)

OK, now back to calls...

### Runtime error checking

The overlapping stack windows work based on the assumption that the function's
declared number of parameters and the actual number of arguments passed are in
agreement. But, again, because Lox ain't statically typed, a foolish user could
pass too many or too few arguments. In Lox, we've defined that to be a runtime
error, which we report like so:

^code check-arity (1 before, 1 after)

Pretty straightforward. This is why we store the arity of each function inside
the FunctionObj for it.

There's another error we need to report that's less to do with the user's
foolishness than our own. Because the CallFrame array has a fixed size, we need
to ensure a deep call chain doesn't overflow it:

^code check-overflow (2 before, 1 after)

In practice, if a program gets close to this limit, it's most likely to be a bug
in some runaway recursive code. While we're on the subject of runtime errors,
let's spend a little time making them more useful. Stopping on a runtime error
is important to prevent the VM from crashing and burning in some ill-defined
way. But simply aborting doesn't help the user fix their code which *caused*
that error.

The classic tool to aid debugging runtime failures is a *stack trace* -- a print
out of each function that was still executing when the program died and where
the execution was at the point that it died. Now that we have a call stack and
we've convienently stored each function's name, we can show that entire stack on
a runtime error. It looks like this:

^code runtime-error-stack (2 before, 2 after)

After printing the error message itself, we walk the call stack from <span
name="top">top</span> (the most recently called function) to bottom (the top
level code). For each frame, we find the line number that corresponds to the
current `ip` inside that frame's function. Then we print that along with the
function name.

<aside name="top">

There seems to be some disagreement on which order stack frames should be shown
in a trace. Most put the innermost function as the first line and work their way
towards the bottome of the stack. Python prints them out in the opposite order.
So reading from top to bottom tells you how your program got to where it is and
the last line is where the error actually occurred.

There's a logic to that style. On the other hand, the "[inverted pyramid][]"
from journalism tells us we should put the most important information *first* in
a block of text. In a stack trace, that's the innermost function where the error
actually occurred. Most other language implementations do that.

[inverted pyramid]: https://en.wikipedia.org/wiki/Inverted_pyramid_(journalism)

</aside>

For example, if you run this broken program:

```lox
fun a() { b(); }
fun b() { c(); }
fun c() {
  c("too", "many");
}

a();
```

It prints out:

```text
Expected 0 arguments but got 2.
[line 10] in c()
[line 6] in b()
[line 2] in a()
[line 13] in script
```

That doesn't look too bad, does it?

### Returning from functions

We're getting close. We can call functions and the VM will execute them. But we
can't *return* from them yet. We've had an `OP_RETURN` instruction for quite
some time, but it's always had some kind of temporary code hanging out in it
just to get us out of the bytecode loop. The time has arrived for a real
implementation:

^code interpret-return (1 before, 1 after)

When a function returns a value, that value will be on top of the stack. We're
about to discard the called function's entire stack window, so we pop that off
and hang on to it first.

Then we discard the CallFrame for the returning function. If that was the very
last CallFrame, it means we've finished executing the top level code and the
entire program is done, so we exit the interpreter.

Otherwise, we need to clean up the callee's leftover bits on the stack. We
discard all of the stack slots used by the called function for its local
variables and parameters. We're also done with the arguments that the callee
placed on the stack. In short, the top of the stack ends up being right at the
beginning of the returning function's stack window. Everything below that point
is still live slots used by the caller that we're returning to.

**todo: illustrate**

We push the return value back onto the stack at that new lower location. Then we
update the `run()` function's cached copy of the current frame pointer. Just
like when we began a call, in the next iteration of the bytecode dispatch loop,
it will read `ip` from that frame and execution will jump back to the caller,
right where it left off immediately after the `OP_CALL` instruction.

Note that we assume here that the function *did* actually return a value, but
a function can implicitly return by reaching the end of its body:

```lox
fun noReturn() {
  print "Do stuff";
  // No return here.
}

print noReturn();
```

We need to handle that correctly too. The language is specified to implicitly
return `nil` in that case. To make that happen, we add this:

^code return-nil (1 before, 2 after)

The compiler calls this to emit the `OP_RETURN` at the end of a function body.
Now, before that, we emit an instruction to push `nil` onto the stack.

We're almost done with the core support. As usual, the new instruction means the
disassembler needs to handle it too:

^code disassemble-call (1 before, 1 after)

We have working function calls! They can even take parameters! It almost looks
like we know what we're doing here.

## Return Statements

Being able to pass data to functions is nice, but it would be nice if they could
pass data *back* too. Some languages implicitly return the value of the last
expression in the body. Lox requires an explicit return statement for a function
to produce a value. We'll add that next:

^code match-return (1 before, 1 after)

That jumps to:

^code return-statement

The return value is optional, so the parser looks for a semicolon token to tell
if a value was provided. If not, it implicitly returns `nil` by emitting an
instruction for that. Otherwise, it compiles the return value expression. In
either case, it ends with an `OP_RETURN` instruction.

This new statement gives us a compile error to worry about too. Returns are
useful for returning from functions but the top level of a Lox program is
imperative code too. You shouldn't be able to <span name="return">return</span>
from there:

```lox
return "What?!";
```

<aside name="return">

Allowing this isn't the worst idea in the world. It would give you a natural way
to terminate a script early. You could maybe even use returning a number to
indicate the process's exit code.

</aside>

We've specified that it's a compile error to have a return statement outside of
any function:

^code return-from-script (1 before, 1 after)

This is one of the reasons we added that FunctionType enum to the compiler.

Our VM is getting more and more powerful. We've got functions, calls,
parameters, returns. You can define lots of different functions that can call
each other in interesting ways. But, ultimately, they can't really *do*
anything. The only user visible thing a Lox program can do, regardless of its
complexity, is print. To add more capabilities, we need to expose them to the
user...

## Native Functions

If users want to write programs that check the time, read user input, or access
the file system, we could conceivably add new instructions and keywords for each
operation, but that obviously doesn't scale well. Instead, most languages expose
functionality by provided "built in", "native" , or "primitive" functions. From
the user's perspective, these look like regular functions in the language that
they can call. But the functions are *implemented* in the underlying host
language -- in our case C. That lets them access functionality that isn't
otherwise exposed to the language. These functions are the windows between the
world of Lox and the world of C.

Lox feels like a "toy" language mainly because it has almost no built in
capabilities. At the language level, it's fairly complete -- it's got closures,
classes, inheritance, and other fun stuff. It's really the lack of native
functions that make it feel small and not useful for real work. Right now, clox
has *no* native functions. Adding a suite of them will go a very long way
towards turning it into a useful language.

But adding a long list of platform capabilities isn't actually very educational.
Once you've seen how to bind one bit of C code to Lox, you get the idea. But
even supporting *one* native function requires us to build out all the
machinery. So we'll go through that and do all the hard work. Then, when that's
done, we'll add one tiny native function just to show it works.

The reason we need new machinery is because, from the implementation's
perspective, native functions are different from Lox functions. They have no
bytecode chunk. Instead, they somehow reference a piece of native C code. When
they are called, they don't push a CallFrame, because there's no bytecode code
for that frame to point to.

We handle this by defining native functions as an entirely different object
type:

^code obj-native (1 before, 2 after)

The representation is simpler than ObjFunction -- just an Obj header and a
pointer to the C function that implements the native behavior. The native
function gets passed the argument count and a pointer to the first argument on
the stack. It accesses the arguments through that pointer. Once it's done, it
returns the result value.

As always, a new object type carries some accoutrement with it. To create an
ObjNative, we declare a constructor-like function:

^code new-native-h (1 before, 1 after)

And an implementation:

^code new-native

The constructor takes a C function pointer to wrap in an ObjNative. It sets up
the object header and stores the function. For the header, we need a new object
type:

^code obj-type-native (2 before, 2 after)

The VM also needs to know how to deallocate a native function object:

^code free-native (4 before, 1 after)

There isn't much here since ObjNative doesn't own any extra memory. The other
capability all objects support is being printed:

^code print-native (2 before, 1 after)

In order to support dynamic typing, we have a macro to see if a value is a
native function:

^code is-native (1 before, 1 after)

Assuming that returns true, this macro converts a Value to an ObjNative:

^code as-native (1 before, 1 after)

All of this baggage lets the VM treat native functions like any other object.
You can store them in variables, pass them around, you get the idea. Of course,
the operation we actually care about is *calling* them -- using one as the left
hand operand in a call expression.

Over in `callValue()` we add another type case:

^code call-native (2 before, 2 after)

If the object being called is a native function, we invoke the C function right
then and there. There's no need to muck with CallFrames or anything. We just
hand off to C, get the result and stuff it back in the stack. This makes native
functions as fast as we can get.

With this, users should be able to call native functions, but there aren't any
to call. Unlike regular Lox functions, they have no way to create their own.
That's our job as VM implementers. We'll start with a utility function to define
a new native function exposed to Lox programs:

^code define-native

It takes a pointer to a C function and the name for the function. We wrap the
function in an ObjFunction and then store that in a global variable with the
given name.

You're probably wondering why we push and pop the name and function on the
stack. That's kind of weird looking, right? This is the kind of stuff you have
to worry about when <span name="worry">garbage</span> collection gets involved.
Both `copyString()` and `newNative()` dynamically allocate memory. That means
once we have a GC, they can potentially trigger a collection. If that happens,
we need to ensure the collector knows we're not done with the name and
ObjFunction so that it doesn't free them out from under us. Storing them on the
value stack accomplishes that.

<aside name="worry">

Don't worry if you didn't follow all that. It will make a lot more sense once we
get around to implementing the GC.

</aside>

It feels a little silly, but after all of that work, we're only going to add one
little native function:

^code clock-native (1 before, 2 after)

This returns the elapsed time since the program started running in seconds. It's
handy for benchmarking Lox programs. In Lox, we'll name it `clock()`:

^code define-native-clock (1 before, 1 after)

The "vm" module needs a couple of includes to wire this all up. First to access
the c standard library `clock()`:

^code vm-include-time (1 before, 2 after)

And then to access the object stuff:

^code vm-include-object (2 before, 1 after)

Phew, that was a long hike. Let's see how far we've come. Type this in and try
it out:

```lox
fun fib(n) {
  if (n < 2) return n;
  return fib(n - 2) + fib(n - 1);
}

var start = clock();
print fib(35);
print clock() - start;
```

We can write a really inefficient recursive Fibonacci function. Even better, we
can measure just *how* ineffecient it is. This is, of course, not the smartest
way to calculate a Fibonacci number. But it is a good way to stress test a
language implementation's support for function calls. On my machine, this in
clox about five times faster than it does in jlox. That's quite an improvement.
You can start to see how our stack-based bytecode architecture pays its way.

<div class="challenges">

## Challenges

1.  Reading and write the `ip` field is one of the most frequent operations
    inside the bytecode loop. Right now, we access it through a pointer to the
    current CallFrame. That requires a pointer indirection. That can force the
    computer to bypass the cache and hit main memory, which can be a real
    performance sink.

    Ideally, we'd be able to keep the `ip` in a native CPU register. C doesn't
    let us *require* that without dropping into inline assembly, but we can
    structure the code to encourage the compiler to make that optimization. If
    we store the `ip` directly in a C local variable and mark it `register`,
    there's a good chance the C compiler will listen to our polite request.

    This does mean we need to be careful to load and store it back into the
    correct CallFrame when starting and ending function calls. Implement this
    optimization. Write a couple of benchmarks and see how it affects the
    performance. Do you think the extra code complexity is worth it?

2.  Right now, there's no way for a native function to signal a runtime error.
    In a real implementation, this is something we'd need to support because
    native functions live in the statically-typed world of C but are called
    from dynamically-typed Lox land. If a user, say, tries to pass a string to
    `sin()`, that native function needs to report a runtime error.

    Extend the native function system to support that. How does this capability
    affect performance of native calls?

3.  Add some more native functions to do things you find useful. Write some
    programs using those. What did you add? How do they affect the feel of the
    language and how useful it is?

4.  Instead of a separate CallFrame stack, most native CPU architectures and
    many virtual machines use a single stack to track both values and calls.
    They push the base pointer and return address to the stack at the beginning
    of a call and pop them when the call completes. Implement that. (You may
    find it easier to implement this on top of the solution to challenge 1.) Do
    a little benchmarking and see how it affects function call performance.

</div>
