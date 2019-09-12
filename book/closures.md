^title Closures
^part A Bytecode Virtual Machine

> As the man said, for every complex problem there's a simple solution, and it's
> wrong.
> <cite>Umberto Eco, <em>Foucault's Pendulum</em></cite>

Thanks to our diligent labor in [the last chapter][last], we have a virtual
machine with working functions. What it lacks is closures. Aside from global
variables, which are their own special snowflakes, a function has no way to
reference a variable declared outside of its own body:

[last]: calls-and-functions.html

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

Run this now and it prints "global". It's supposed to print "outer". To fix
this, we need to include the entire lexical scope of all surrounding functions
when resolving a variable.

This problem is harder in clox than it was in jlox because our bytecode VM
stores locals on a stack. We used a stack because I claimed locals have stack
semantics -- variable can be discarded in the reverse order that they are
created. But with closures, that's only *mostly* true:

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

The outer function `makeClosure()` declares a local variable. Then it creates an
inner function, `closure()` that references that variable. `makeClosure()` then
returns a reference to that function. Since the closure escapes while holding
onto the local variable, `local` must outlive the function call where it was
created.

We could solve this problem by heap allocating memory for all local variables.
That's what jlox does by putting everything in those Environment objects that
float around in Java's heap. But we don't want to. Using a <span
name="stack">stack</span> is *really* fast. Most local variables are *not*
captured by closures and do have stack semantics. It would suck to make all of
those slower for the benefit of the rare local that is captured.

<aside name="stack">

There is a reason that C and Java use the stack for their local variables, after
all.

</aside>

This means a more complex approach than we used in our Java interpreter. Because
some locals have very different lifetimes, we will have two implementations
strategies, one optimized for each. For locals that aren't used in closures,
we'll keep them just as they are on the stack. When a local is captured by a
closure, we'll adopt another solution that lifts them onto the heap where they
can live as long as needed.

Closures have been around since the early Lisp days when bytes of memory and CPU
cycles were more precious than emeralds. Over the intervening decades, hackers
devised all <span name="lambda">manner</span> of ways to compile closures to
efficient runtime representations. Some are more efficient but require a much
more complex compilation process than we could easily retrofit into clox.


<aside name="lambda">

Search for "closure conversion" or "lambda lifting" to start exploring.

</aside>

The technique I explain here comes the design of the Lua VM. It is fast,
parsimonious with memory, and a small amount of code implement. Even more
impressive, it fits naturally into the single-pass compilers clox and Lua both
use. It is somewhat intricate, though. It might take a while before all the
pieces click together in your mind. So we'll build them one step at a time and
I'll try to introduce the concepts in stages.

## Closure Objects

Our VM represents functions at runtime using ObjFunction. These objects are
created by the compiler at compile time. At runtime, all the VM does is load the
function object from a constant table and bind it to a name. There is no
operation to "create" a function at runtime. Much like string and number
<span name="literal">literals</span>, they are created purely at compile time.

<aside name="literal">

In other words, a function declaration in Lox basically *is* a kind of literal
-- a piece of syntax that defines a constant value of a built in type.

</aside>

That made sense because all of the data that composes a function is known at
compile time: it's just a wrapper around a chunk of bytecode derived from the
source of the function's body, and some constants. Once we introduce closures,
though, that representation is no longer sufficient. Take a gander at:

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

The `makeClosure()` function defines and returns a function. We call it twice
and get two closures back. They are created by the same nested function
declaration, but close over different values. When we call them, they print
different strings. That implies we need *some* representation for a closure that
is created at runtime and captures the local variables surrounding the function
as they existed when the function declaration was executed.

We'll work our way up to capturing variables, but a good first step is defining
that object representation. Our existing ObjFunction type represents the <span
name="raw">"raw"</span> compile-time state of a function declaration, since all
closures created from a single declaration share the same code and constants. At
runtime, when a function declaration is executed, that gets wrapped in a new
ObjClosure structure. It has a reference to the underlying bare function along
with runtime state for variables it closes over.

<aside name="raw">

The Lua implementation refers to the raw function object containing the bytecode
as a "prototype", which is a great word to describe this, except that word also
gets overloaded to refer to [prototypal inheritance][].

[prototypal inheritance]: https://en.wikipedia.org/wiki/Prototype-based_programming

</aside>

We'll wrap every function in an ObjClosure, even if the function doesn't
actually close over and capture any surrounding local variables. This is a
little wasteful, but it simplifies the VM because we can always assume that the
function we're calling is an ObjClosure. That new struct starts out like this:

^code obj-closure

Right now, it simply points to an ObjFunction and adds the necessary object
header stuff. Grinding through the usual ceremony for adding a new object type
to clox, we declare a C function to create a new closure:

^code new-closure-h (2 before, 1 after)

Then implement it:

^code new-closure

It takes a pointer to the ObjFunction it wraps. It also initializes the type
field to a new type:

^code obj-type-closure (1 before, 1 after)

And when we're done with a closure, we release its memory:

^code free-closure (1 before, 1 after)

We only free the ObjClosure itself, not the ObjFunction. That's because the
closure doesn't *own* the function. There may be multiple closures that all
reference the same function, and none of them claims any special privilege over
it. We can't free the ObjFunction until *all* objects referencing it are gone --
including even the surrounding function whose constant table contains it.
Tracking that sounds tricky, and it is! That's why we'll write a garbage
collector soon to manage it for us.

We also have the usual <span name="macro">macros</span> for checking a value's type:

<aside name="macro">

Perhaps I should have defined a macro to make it easier to generate these
macros. Maybe that would be a little too meta.

</aside>

^code is-closure (2 before, 1 after)

And to cast a value:

^code as-closure (2 before, 1 after)

Closures are first-class objects, so you can print them:

^code print-closure (1 before, 1 after)

They display exactly as ObjFunction does. From the user's perspective, the
difference between ObjFunction and ObjClosure is purely a hidden implementation
detail.

There's a funny edge case to handle here. If you enable `DEBUG_TRACE_EXECUTION`,
the VM prints the stack after each opcode. The stack will include the special
function the compiler creates to represent top level code. That function doesn't
have a name. A user can never get a hold of this hidden function, so that isn't
usually a problem. But our diagnostic code can, so we need to not crash in that
case:

^code print-script (1 before, 1 after)

With that out of the way, we have a working but empty representation for
closures.

### Compiling to closure objects

We have closure objects but our VM never creates them. The next step is getting
the compiler to emit instructions to tell the runtime when to create a new
ClosureObj to wrap a given FunctionObj. This happens right at the end of a
function declaration:

^code emit-closure (1 before, 1 after)

Before, the final bytecode for a declaration was a single `OP_CONSTANT`
instruction to load the compiled function from the surrounding function's
constant table and push it onto the stack. Now we have a new instruction:

^code closure-op (1 before, 1 after)

Like `OP_CONSTANT`, it takes a single operand that represents a constant table
index for the function. But when we get over to the runtime implementation, it
will do something a little more interesting.

First, let's be diligent VM hackers and slot in disassembler support for it:

^code disassemble-closure (3 before, 1 after)

There's more going on here than we usually have in the disassembler. By the end
of the chapter, you'll discover that `OP_CLOSURE` is quite an unusual
instruction. It's straightforward right now -- just a single byte operand -- but
we'll be adding to it. So this code here prepares for that future growth.

### Interpreting function declarations

Most of the work we need to do is in the runtime. We have to handle the new
instruction, naturally. But we also need to touch every piece of code in the VM
that works with ObjFunction and change it to use ObjClosure instead -- function
calls, callframes, etc. We'll start with the instruction, though:

^code interpret-closure (2 before, 1 after)

Like the `OP_CONSTANT` instruction we used to use, first we load the compiled
function from the constant table. Then we wrap that in a new ObjClosure and push
the result onto the stack.

Once you have a closure, you'll eventually want to call it:

^code call-value-closure (1 before, 1 after)

We remove the code for calling objects whose type is `OBJ_FUNCTION`. Since we
wrap all bare functions in ObjClosures, the runtime will never try to invoke a
raw ObjFunction anymore. Those objects only live in constant tables and get
immediately <span name="naked">wrapped</span> in closures before anything else
sees them.

<aside name="naked">

We don't want any naked functions wandering around the VM! What would the
neighbors say?

</aside>

We replace it with *very similar* code for calling a closure instead. The only
difference is the type of object we pass to `call()`. The real changes are
over in that function. First, we update its signature:

^code call-signature (1 after)

Then, in the body, we need to fix everything that referenced the function to
handle the fact that we've introduced a layer of indirection. First, the
arity checking:

^code check-arity (1 before, 1 after)

The only change is that we unwrap the closure to get to the underlying function.
The next thing `call()` does is create a new CallFrame. We change that to store
the closure in the CallFrame and get the bytecode pointer from the closure's
function:

^code call-init-closure (1 before, 2 after)

This necessitates changing the declaration of CallFrame too:

^code call-frame-closure (1 before, 1 after)

That change triggers a few other cascading changes. Every place in the VM that
accessed CallFrame's function needs to use a closure instead. First, the macro
for reading a constant from the current function's constant table:

^code read-constant (1 before, 1 after)

When `DEBUG_TRACE_EXECUTION` is enabled so we can diagnose bugs in the VM, it
needs to get to the chunk from the closure:

^code disassemble-instruction (1 before, 1 after)

Likewise, when reporting a runtime error, we need to get the function from the
closure before accessing the debug line information:

^code runtime-error-function (1 before, 1 after)

Almost there. The last piece is the blob of code that sets up the very first
CallFrame to begin executing the top level code for a Lox script:

^code interpret (4 before, 2 after)

<span name="pop">The</span> compiler still returns a raw ObjFunction when
compiling a script. That's fine, but it means we need to wrap it in an
ObjClosure here before the VM can execute it.

<aside name="pop">

The code looks a little silly here because we still push the original
ObjFunction onto the stack. Then we pop it after creating the closure, only to
then push the closure. Why put the ObjFunction on there at all? As usual, when
you see weird stack stuff going on, it's to keep the forthcoming garbage
collector aware of some heap-allocated objects.

</aside>

We are back to a working interpreter. The *user* can't tell any difference, but
the compiler now generates code to produce a closure for each function
declaration. The VM then handles executing those closures. That's the boring
stuff out of the way. Now we're ready to make these closures actually *do*
something.

## Upvalues

Our existing instructions for reading and writing local variables are limited to
a single function's stack window. Local variables from a surrounding function
are outside of the inner function's window. We're going to need some new
instructions.

The easiest approach might be an instruction that takes a relative stack slot
offset that can reach before the current function's window. That would work if
closed-over variables were always on the stack. But as we saw earlier, these
variables sometimes outlive the function where they are declared. That means
they won't always be on the stack.

**todo: illustrate relative offset idea**

The next easiest approach then would be to take any local variable that gets
closed over and have it always live on the heap. When the local variable
declaration in the surrounding function is executed, the VM allocates memory for
it dynamically. That way it can live as long as needed.

This would be a fine approach if clox didn't have a single-pass compiler. But
that restriction we chose in our implementation makes things harder. Take a look
at this example:

```lox
fun outer() {
  var x = 1;    // (1)
  x = 2;        // (2)
  fun inner() { // (3)
    print x;
  }
  inner();
}
```

**todo: turn into illustration and show generated code at each step?**

Here, the compiler compiles the declaration of `x` at `(1)` and emits code for
the assignment at `(2)`. It does that before reaching the declaration of
`inner()` at `(3)` and discovering that `x` is in fact closed over. We don't
have an easy way to go back and fix that already-emitted code to treat `x`
specially. Instead, we want a solution that allows a closed over variable to
live on the stack exactly like a normal local variable *until the point that is
closed over*.

Fortunately, such a solution exists! The approach we'll use is taken from the
implementation of Lua. We use a level of indirection that Lua calls an
**upvalue**. An upvalue refers to a local variable in an enclosing function.
Every closure maintains an array of upvalues, one for each surrounding local
variable that the closure uses.

The upvalue points back into the stack to where the variable it captured lives.
When the closure needs to access a closed-over variable, it goes through the
corresponding upvalue to reach it. When a function declaration is first executed
and we create a closure for it, the VM creates the array of upvalues and wires
them up to "capture" the surrounding local variables that the closure needs.

**todo: illustrate**

As we'll see later, when the captured variable floats off the stack onto the
heap, upvalues give us a point of indirection where we can keep track of it.
But, for now, we'll focus on captured variables that are still on the stack.

### Compiling upvalues

As usual, we want to do as much work as possible during compilation to keep
execution simple and fast. Since local variables are lexically scoped in Lox, we
have enough knowledge at compile time to resolve which surrounding local
variables a function accesses and where those locals are declared. That in turn
means we know how many upvalues a closure needs, which variables they capture,
and which stack slots contain those variables in the declaring function's stack
window.

Currently, when the compiler resolves an identifier, it walks the block scopes
for the current function from innermost to outermost. If we don't find the
variable in that function, we assume the variable must be a global variable. We
don't consider the local scopes of any enclosing functions -- they get skipped
right over. The first change then is inserting a resolution step where we look
through the local scopes in the chain of enclosing functions:

^code named-variable-upvalue (1 before, 1 after)

This new `resolveUpvalue()` function looks for a local variable declared in any
of the surrounding functions. If it finds one, it returns an "upvalue index" for
that variable. (We'll get into what that means later.) Otherwise, it returns -1
to indicate the variable wasn't found. If it was found, we use these two new
instructions for reading or writing to the variable through its upvalue:

^code upvalue-ops (1 before, 1 after)

We're implementing this sort of top-down, so I'll show you how these work at
runtime soon. The most interesting part is how the compiler actually resolves
the identifier:

^code resolve-upvalue

We call this after failing to resolve a local variable in the current function's
scope, so we know the variable isn't in the current compiler. Recall that
Compiler stores a pointer to the Compiler for the enclosing function and these
pointers form a linked chain that goes all the way to the root Compiler for the
top-level code. Thus, if the enclosing Compiler is `NULL`, we know the variable
must be <span name="undefined">global</span>, so we return -1.

<aside name="undefined">

It might end up being an entirely undefined variable and not even global. But in
Lox, we don't detect that error until runtime, so from the compiler's
perspective, it's "optimistically global."

</aside>

Otherwise, we try to resolve the identifier as a *local* variable in the
*enclosing* compiler. In other words, we look for it right outside the current
function. For example:

```lox
fun outer() {
  var x = 1;
  fun inner() {
    print x; // (1)
  }
  inner();
}
```

When compiling the identifier expression at `(1)`, `resolveUpvalue()` looks for
a local variable `x` declared in `outer()`. If found (like it is here), then
we've successfully resolved the variable. We create an upvalue so that the inner
function can access the variable through that. The upvalue is created here:

^code add-upvalue

The compiler keeps an array of upvalue structures to track the closed-over
identifiers that it has resolved in the body of each function. Remember how the
compiler's Local array mirrors the stack slot indexes where locals live at
runtime? The upvalue array works the same way. The indexes in the compiler's
array match the indexes where upvalues will live in the ObjClosure at runtime.
At the end of the function declaration when the compiler emits code to create
the closure, it uses its upvalue array to generate code that captures the right
variables to populate the closure's runtime upvalue array.

This function adds a new upvalue to that array. It also keeps track of the
number of upvalues the function uses. It stores that count directly in the
ObjFunction itself because we'll also <span name="span">need that number for use
at runtime.

<aside name="span">

Like literal values and function arity, the upvalue count is another one of
those little pieces of data that forms the bridge between the compiler and
runtime.

</aside>

The `index` field tracks the closed-over local variable's slot index. That way
the compiler knows *which* variable in the enclosing function needs to be
captured. We'll circle back to what that `isLocal` field is for before too long.
Then `addUpvalue()` returns the index of the created upvalue in the function's
upvalue list. That index becomes the operand to the `OP_GET_UPVALUE` and
`OP_SET_UPVALUE` instructions.

That's the basic idea for resolving upvalues, but the function isn't very
robust. A closure may reference the same variable in a surrounding function
multiple times. In that case, we don't want to waste time and memory creating a
separate upvalue for each identifier expression. To fix that, before we add a
new upvalue, we first check to see if the function already has an upvalue that
closes over that variable:

^code existing-upvalue (1 before, 1 after)

If we find an upvalue in the array whose local index matches the one we're
adding, we just return that *upvalue* index and reuse it. Otherwise, we fall
through and add the new upvalue.

These two functions access and modify a bunch of new state, so let's define
that. First, we add the upvalue count to ObjFunction:

^code upvalue-count (1 before, 1 after)

We're conscientious C programmers, so we zero-initialize that when an
ObjFunction is first allocated:

^code init-upvalue-count (1 before, 1 after)

In the compiler, we add a field for its upvalue array:

^code upvalues-array (1 before, 1 after)

For simplicity, I gave it a fixed size. The `OP_GET_UPVALUE` and
`OP_SET_UPVALUE` instructions encode an upvalue index using a single byte
operand, so there's a restriction on how many a function can have. Given that,
we can afford a static array that large. We also need to make sure the compiler
doesn't overflow that limit:

^code too-many-upvalues (5 before, 1 after)

Finally, the Upvalue struct type itself:

^code upvalue-struct

The `index` field stores which local slot the upvalue is capturing. The
`isLocal` field is more interesting. To learn what that's all about, read
on...

### Flattening upvalues

In the example I showed before, the closure is accessing a variable declared in
the immediately enclosing function. Lox also supports accessing local variables
declared in *any* enclosing scope, as in:

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

Here, we're accessing `x` in `inner()`. That variable is defined not in
`middle()`, but all the way out in `outer()`. We need to handle cases like this
too. You *might* think that this isn't much harder since the variable will
simply be somewhere farther down on the stack. But consider this more pernicious
example:

```lox
fun outer() {
  var x = "value";
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

outer()()();
```

When you run this, it should print:

```text
return from outer
create inner closure
value
```

**todo: illustrate execution flow and stack**

I know, it's convoluted. It's worth tracing through the execution carefully to
see exactly what order things happen in. The important part is that `outer()`
-- where `x` is declared -- returns and pops all of its variables off the stack
before the *declaration* of `inner()` executes. So, at the point in time that
we create the closure for `inner()`, `x` is already off the stack.

We really have two problems:

1.  We need to resolve local variables that are declared in surrounding
    functions beyond the immediately enclosing one.

2.  We need to be able to capture variables that have already left the stack.

Fortunately, we're in the middle of adding upvalues to the VM, and upvalues are
explicitly designed for solving the second problem. So, in a clever bit of
self-reference, we can use upvalues to allow upvalues to capture variables
declared outside of the immediately surrounding function.

The solution is to allow an upvalue to capture either a local variable or
*another upvalue* in the enclosing function. If a deeply nested function
references a local variable declared several hops away, we'll thread it through
each of the intermediate functions by having each one capture it to an upvalue
for the next function to grab it.

In the above example, `middle()` captures the local variable `x` in the
enclosing function `outer()` and stores it in its own upvalue. It does this even
though `middle()` itself doesn't reference `x`. Then, when the declaration of
`inner()` executes, its closure grabs the upvalue from `middle()` that captured
`x`. A function only ever captures -- either a local or upvalue -- from the
immediately surrounding function, which is guaranteed to still be around at the
point that the inner function declaration executes.

**todo: illustrate**

In order to implement this, `resolveUpvalue()` becomes recursive:

^code resolve-upvalue-recurse (3 before, 2 after)

It's only another three lines of code, but I found this function really
challenging to get right the first time. This in spite of the fact that I wasn't
inventing anything new, just porting the concept over from Lua. Most recursive
functions either do all their work before the recursive call (a *pre-order
traversal*, or "on the way down"), or they do all the work after the recursive
call (a *post-order traversal*, or "on the way back up"). This function does
both. The recursive call is right in the middle.

We'll walk through it slowly. First, we look for a matching local variable in
the enclosing function. If we find one, we capture that local and return. That's
the <span name="base">base case.

<aside name="base">

The other base case, of course, is if there is no enclosing function. In that
case, the variable can't be resolved lexically and is treated as global.

</aside>

**todo: muddled. rewrite?**

Otherwise, we look for a local variable beyond the immediately enclosing
function. We do that by recursively calling `resolveUpvalue()` on the
*enclosing* compiler, not the current one. This series of `resolveUpvalue()`
calls works its way along the chain of nested compilers until it hits one of
the base cases -- either it finds an actual local variable to capture or it
runs out of compilers.

Assuming it does find a local variable, the mostly deeply <span
name="outer">nested</span> call to `resolveUpvalue()` captures it as a local and
returns the upvalue index. That returns to the next call for the inner function
declaration. That call captures the *upvalue* from the surrounding function, and
so on. As each nested call to `resolveUpvalue()` returns, we drill back down
into the innermost function declaration where the identifier we are resolving
appears. At step along the way, we add an upvalue and pass it down to the next
function.

<aside name="outer">

Each recursive call to `resolveUpvalue()` walks *out* one level of function
nesting. So an inner *recursive call* refers to an *outer* nested declaration.
The innermost recursive call to `resolveUpvalue()` that finds the local variable
will be for the *outermost* function where that variable is actually declared.

</aside>

It might help to walk through the original example:

```lox
resolveUpvalue(inner, x)
  not local in enclosing (middle)
  resolveUpvalue(middle, x)
    find local in outer
    add upvalue to middle to capture
    return upvalue index
  found upvalue in middle
  add upvalue to inner to capture middle's upvalue
  return upvalue index
```

**todo: turn into illustration**

Note that the new call to `addUpvalue()` passes `false` for the `isLocal`
parameter. That flag controls whether the closure captures a local variable or
an upvalue from the surrounding function.

By the time the compiler reaches the end of a function declaration, every
variable reference has been resolved as either a local, an upvalue, or a global.
Each upvalue may in turn capture a local variable from the surrounding function,
or an upvalue in the case of transitive closures. Now we have enough data to
emit bytecode which creates a closure at runtime that captures all of the
correct variables:

^code capture-upvalues (1 before, 1 after)

The `OP_CLOSURE` instruction is unique in that it has a variably-sized encoding.
For each upvalue the closure captures, there are two single-byte operands. Each
pair of operands specifies what that upvalue captures. If the first byte is 1,
it captures a local variable in the enclosing function. If zero, it captures one
of the function's upvalues. The next byte is the local slot or upvalue index to
capture.

This odd encoding means we need some bespoke support in the disassembler:

^code disassemble-upvalues (2 before, 1 after)

For example, take this script:

```lox
fun outer() {
  var a = 1;
  var b = 2;
  fun middle() {
    var c = 3;
    var d = 4;
    fun inner() {
      print a + c + b + d;
    }
  }
}
```

When we disassembly the instruction that creates the closure for `inner()` we
get:

```text
0004    9 OP_CLOSURE          2 <fn inner>
0006      |                     upvalue 0
0008      |                     local 1
0010      |                     upvalue 1
0012      |                     local 2
```

We have two other, simpler instructions to add disassembler support for:

^code disassemble-upvalue-ops (2 before, 1 after)

These both have a single byte operand, so there's nothing exciting going on.

Our compiler is where we want it. For each function declaration, it outputs an
`OP_CLOSURE` instruction followed by a series of pairs of operand bytes for each
upvalue we need to create for the closure object at runtime. It's time to hop
over to that side of the VM and get things running.

## Upvalue Objects

Each `OP_CLOSURE` instruction is now followed by the series of bytes that
specify the upvalues the ObjClosure should own. Before we process those
operands, we need a runtime representation for upvalues:

^code obj-upvalue

A key requirement for upvalues managing closed-over variables that no longer
live on the stack. That implies some amount of dynamic heap allocation. The
easiest way to do that in our VM is by building on top of the object system we
already have in place. That way, when we implement a garbage collector in [the
next chapter][gc], the GC can manage memory for upvalues too.

[gc]: garbage-collection.html

Thus, our runtime upvalue structure is an ObjUpvalue with the typical Obj
header field. Following that is the field that points to the closed-over
variable. Note that this is a *pointer* to a Value, not a Value itself. It's a
reference to a *variable*, not a *value*. This is important because it means
that when we assign to the variable the upvalue captures, we're assigning to
the actual variable, not a copy. For example:

```lox
fun outer() {
  var x = "before";
  fun inner() {
    x = "assigned";
  }
  inner();
  print x;
}
```

This program should print "assigned" even though the closure assigns to `x` and
the surrounding function accesses it.

Because upvalues are objects, we've got all the usual object machinery. A
constructor-like function:

^code new-upvalue-h (1 before, 1 after)

It takes a pointer to slot where the closed-over variable lives. Here is the
implementation:

^code new-upvalue

We initialize the object and store the pointer. A new object type:

^code obj-type-upvalue (1 before, 1 after)

A destructor-like function:

^code free-upvalue (3 before, 1 after)

Multiple closures can close over the same variable, so ObjUpvalue does not own
the variable it references. So the only thing to free is the ObjUpvalue itself.

And, finally, to print:

^code print-upvalue (3 before, 1 after)

This isn't practically useful. Upvalues are objects so that we can take
advantage of the VM's memory management, but they aren't first-class values that
a Lox user can directly access in a program. This code will never actually
execute. But it keeps the compiler from yelling at us about an unhandled switch
case so here we are.

### Upvalues in closures

When I first introduced upvalues, I said each closure has an array of them.
We've finally worked our way back to implementing that:

^code upvalue-fields (1 before, 1 after)

<span name="count">Different</span> closures may have a different number of
upvalues, so we need a dynamic array. The upvalues themselves are dynamically
allocated too, so we end up with a double pointer -- a pointer to a dynamically
allocated array of pointers to upvalues. We also store the number of elements in
the array.

<aside name="count">

Storing the upvalue count in the closure is redundant because the ObjFunction
that the ObjClosure references also keeps that count. As usual, when you see
some seemingly useless code, it is to appease the GC. The collector may need to
know how big an ObjClosure's upvalue array is after the closure's corresponding
ObjFunction has already been freed.

</aside>

When we create an ObjClosure, we allocate an upvalue array of the proper size:

^code allocate-upvalue-array (1 before, 1 after)

Before creating the closure object itself, we allocate the array of upvalues and
initialize them all to `NULL`. This weird ceremony around memory is a careful
dance to please the (forthcoming) garbage collection deities. It ensures the
memory manager never sees uninitialized memory.

Then we store the array in the new closure, as well as copying the count over
from the ObjFunction:

^code init-upvalue-fields (1 before, 1 after)

When an ObjClosure is freed, we also free the upvalue array:

^code free-upvalues (1 before, 1 after)

ObjClosure does not own the ObjUpvalue objects itself, but it does own the array
that contains *pointers* to those upvalues. We fill the upvalue array over in
the interpreter when it creates a closure. This is where we walk through all of
the operands after `OP_CLOSURE` to see what kind of upvalue each slot captures:

^code interpret-capture-upvalues (1 before, 1 after)

This code is the magic moment when a closure comes to life and captures
variables. We iterate over each upvalue the closure expects. For each one, we
read a pair of operand bytes. If the upvalue closes over a local variable in
the enclosing function, we let `captureUpvalue()` do the work. Otherwise, we
capture an upvalue from the surrounding function.

An `OP_CLOSURE` instruction is emitted at the end of a function declaration. At
the moment that we are executing that declaration, the *current* function is the
surrounding function. In other words, the current function is the closure stored
in the CallFrame at the top of the callstack. So, to grab an upvalue from the
enclosing function, we can read it right from the `frame` local variable, which
caches a reference to that CallFrame.

Closing over a local variable is a little more interesting. Most of the work
happens in a separate function, but first we calculate the argument to pass to
it. We need to grab a pointer to the local variable in the given slot in the
surrounding function's stack window. That window begins at `frame->slots`, which
points to slot zero. Adding `index` offsets that to the local slot we want to
capture. Then we go to:

^code capture-upvalue

This seems a little silly. All it does is create a new ObjUpvalue that captures
the given stack slot and returns it. Did we need a separate function for this?
Well, no, not *yet*. But soon it will get more interesting.

First, let's wrap up what we're working on. Back in the interpreter code for
handling `OP_CLOSURE`, we eventually finish iterating through the upvalue
array and initialize each one. When that completes, we have a new closure with
an array full of upvalues pointing to variables.

With that in hand, we can implement the instructions for using those upvalues:

^code interpret-get-upvalue (1 before, 2 after)

The operand is the index into the current function's upvalue array. So we simply
look up the corresponding upvalue and dereference its value pointer to read the
value in that slot. Writing is similar:

^code interpret-set-upvalue (1 before, 2 after)

We <span name="assign">take</span> the value on top of the stack and store it
into the slot pointed to by the chosen upvalue. Just as with the instructions
for local variables, it's important for these instructions to be fast. User
programs are constantly reading and writing variables, so if that's slow,
everything is slow. And, as usual, the way we make them fast is by making them
as simple as possible. These two new instructions are pretty good: no control
flow, no complex arithmetic, just a couple of pointer indirections and a
`push()`.

<aside name="assign">

The set instruction doesn't *pop* the value from the stack because, remember,
assignment is an expression in Lox. So the result of the assignment -- the
assigned value -- needs to remain on the stack for the surrounding expression.

</aside>

This is a milestone. As long as all of the variables remain on the stack, we
have working closures:

```lox
fun outer() {
  var x = "outside";
  fun inner() {
    print x;
  }
  inner();
}
```

Run this and it correctly prints "outside".

## Closed Upvalues

Of course, a key feature of closures is that they hold onto the variable as long
as needed, even after the function that declares the variable has returned.
Here's another example that *should* work:

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

But if you run it right now... who knows what it does. At runtime, it will end
up reading from a stack slot that longer contains the closed-over variable. Like
I've mention a few times, the crux of the issue is that variables in closures
don't have stack semantics. That means we've got to hoist them off the stack
when the function where they were declared returns. This final section of the
chapter does that.

### Values and variables

Before we get to writing code, I want to dig into an important semantic point.
Does a closure close over a *value* or a *variable?* This isn't purely an <span
name="academic">academic</span> question, and I'm not just splitting hairs.
Consider:

<aside name="academic">

If Lox didn't allow assignment, it would be an academic question.

</aside>

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

The outer `main()` function creates two closures and stores them in <span
name="global">global</span> variables so that they outlive the execution of
`main()` itself. Both of those closures capture the same variable. The first
closure assigns a new value to it and the second closure reads the variable.

<aside name="global">

The fact that I'm using a couple of global variables isn't significant. I needed
some way to return two values from a function and without any kind of collection
type in Lox, my options were limited.

</aside>

What does the call to `globalGet()` print? If closures capture *values* then
each closure gets its own copy of `a` with the value that `a` had at the point
in time that the closure's function declaration executed. The call to
`globalSet()` will modify `set()`&rsquo;s copy of `a`, but `get()`&rsquo;s copy
will be unaffected. Thus, the call to `globalGet()` will print "initial".

If closures close over variables, then `get()` and `set()` will both capture
the *same mutable variable*. When `set()` changes `a`, it changes the same `a`
that `get()` reads from. There is only one `a`. That in turn implies the call
to `globalGet()` will print "updated".

Which is it? The answer for Lox and most other languages I know with closures is
the latter. Closures capture variables. You can think of them as capturing *the
place the value lives*. This is important to keep in mind as we deal with
closed-over variables that are no longer on the stack. When a variable moves to
the heap, we need to ensure that all closures capturing that variable retain a
reference to its *one* new location. That way, when the variable is mutated, all
closures see the change.

### Closing upvalues

We know that local variables always start out on the stack. This is faster, and
ensures that emitted code that uses the variable before the compiler discovers
that a closure captures it still functions. We also know that closed-over
variables need to move to the heap if the closure outlives the function where
the captured variable is declared.

Following Lua, we'll call upvalues that refer to local variables on the stack
**open upvalues**. When the variable moves to the heap, we are *closing* the
upvalue and the result is, naturally, a **closed upvalue**. The two questions
we need to answer are:

1.  Where on the heap does the closed-over variable go?
2.  When do we close the upvalue?

The answer to the first question is easy. We already have a convenient object on
the heap that represents a reference to a variable -- ObjUpvalue itself. The
closed-over variable will move into a new field right inside the ObjValue
struct. That way we don't need to do any additional heap allocation to close an
upvalue.

The second question is straightforward too. As long as the variable is on the
stack, there may be code that refers to it there and that code must work
correctly. So the logical time to hoist the variable to the heap is as late as
possible. If we move the local variable right when it goes out of scope, we are
certain that no code after that point will try to access it from the stack.
<span name="after">After</span> the variable is out of scope, the compiler will
have reported an error if any code tried to use it.

<aside name="after">

By "after" here, I mean in the lexical or textual sense -- code past the `}`
for the block containing the declaration of the closed-over variable.

</aside>

The compiler already emits an `OP_POP` instruction when a local variable goes
out of scope. If a variable is captured by a closure, we will instead emit a
different instruction to hoist that variable out of the stack and into its
corresponding upvalue. To do that, the compiler needs to know which <span
name="param">locals</span> are closed over.

<aside name="param">

The compiler doesn't pop parameters and locals declared immediately inside the
body of a function. We'll handle those too, in the runtime.

</aside>

The compiler already maintains an array of Upvalue structs for each local
variable in the function to track exactly that state. That array is good for
answering "Which variables does this closure use?" But it's poorly suited for
answering, "Does *any* function capture this local variable?" In particular,
once the Compiler for some closure has finished, the Compiler for the enclosing
function whose variable has been captured no longer has access to any of the
upvalue state.

In other words, the compiler maintains pointers from upvalues to the locals they
capture, but not in the other direction. So we first need to add some extra
tracking inside the existing Local struct so that we can tell if a given local
is captured by a closure:

^code is-captured-field (1 before, 1 after)

This field is `true` if the local is captured by any later nested function
declaration. Initially, all locals are not captured:

^code init-is-captured (1 before, 1 after)

<span name="zero">Also</span>, the special "slot zero local" that the compiler
implicitly declares:

<aside name="zero">

Later in the book, it *will* become possible for a user to capture this
variable.

</aside>

^code init-zero-local-is-captured (1 before, 1 after)

When resolving an identifier, if we end up creating an upvalue for some
enclosing local variable, we mark the local as captured:

^code mark-local-captured (1 before, 1 after)

Now, at the end of a block scope when the compiler is emitting code to free the
stack slots for the locals, we can tell which ones need to get hoisted onto the
heap. We'll use a new instruction for that:

^code end-scope (3 before, 2 after)

The instruction requires no operand. We know that the variable will always be
right on top of the stack at the point that this instruction executes. We
declare the instruction:

^code close-upvalue-op (1 before, 1 after)

And add trivial disassembler support for it:

^code disassemble-close-upvalue (1 before, 1 after)

Excellent. Now the generated bytecode tells the runtime exactly when each
captured local variable must move to the heap. Better, it only does so for the
locals that *are* used by a closure and need this special treatment. This aligns
with our general performance goal that we only want users to pay for
functionality that they use. Variables that aren't used by closures live and die
entirely on the stack just as they did before.

### Tracking open upvalues

Let's move over to the runtime side. Before we can interpret `OP_CLOSE_UPVALUE`
instructions, we have an issue to resolve. Earlier, when we talked about whether
closures capture variables or values, I said it was important that if multiple
closures close over the same variable that they end up with a reference to the
exact same storage location in memory. That way if one closure writes to the
variable, the other closure sees the change.

Right now, if two closures capture the same <span name="indirect">local</span>
variable, the VM creates a separate Upvalue for each one. That breaks that
sharing. When we move the variable off the heap, if we move into both of those
upvalues, it will get "split" and each closure will end up with its own copy.

<aside name="indirect">

The VM *does* share upvalues if one closure captures an *upvalue* from a
surrounding function. The nested case works correctly. But if two *sibling*
closures capture the same local variable, they each create a separate
ObjUpvalue.

</aside>

**todo: illustrate**

To fix that, whenever the VM needs an upvalue that captures a particular local
variable slot, we will first search for an existing upvalue pointing to that
slot. If found, we reuse that. The challenge is that all of the previously
created upvalues are squirreled away inside the upvalue arrays of the various
closures. Those closures could be anywhere in the VM's memory.

The first step is to give the VM its own list of all open upvalues that point to
variables still on the stack. Seaching a list each time the VM needs an upvalue
sounds like it might be slow, but in practice, it's not bad. The number of
variables on the stack that actually get closed over tends to be small. And
function declarations that <span name="create">create</span> closures are rarely
on performance critical execution paths in the user's program.

<aside name="create">

Closures are frequently *invoked* inside hot loops. Think about the closures
passed to typical higher-order functions on collections like [`map()`][map] and
[`filter()`][filter]. That should be fast. But the function declaration that
*creates* the closure only happens once and is usually outside of the loop.

[map]: https://en.wikipedia.org/wiki/Map_(higher-order_function)
[filter]: https://en.wikipedia.org/wiki/Filter_(higher-order_function)

</aside>

Even better, we can order the list of open upvalues by the stack slot index they
point to. The common case is that a slot has *not* already been captured --
sharing variables between closures is uncommon -- and closures tend to capture
locals near the top of the stack. If we store the open upvalue array in stack
slot order, as soon as we step past the slot where the local we're capturing
lives, we know it won't be found. When that local is near the top of the stack,
we can exit the loop pretty early.

**todo: illustrate**

Maintaining a sorted list requires inserting elements in the middle efficiently.
That suggests using a linked list instead of a dynamic array. Since we defined
the ObjUpvalue struct ourselves, the easiest implementation is an intrusive list
that puts the next pointer right inside the ObjUpvalue struct itself:

^code next-field (1 before, 1 after)

When we allocates a new upvalue, it is not attached to any list yet, so the link
is `NULL`:

^code init-next (1 before, 1 after)

The VM owns the list, so the head pointer goes right inside the main VM struct:

^code open-upvalues-field (1 before, 3 after)

The list starts out empty:

^code init-open-upvalues (1 before, 1 after)

Whenever we close over a local variable, before creating a new upvalue, we look
for an existing one in the list:

^code look-for-existing-upvalue (1 before, 1 after)

We start at the <span name="head">head</span> of the list, which is the upvalue
closest to the top of the stack. We walk through the list, using a little
pointer comparison to iterate past every upvalue pointing to slots above the one
we're looking for. While we do that, we keep track of the preceding upvalue on
the list. We'll need to update that node's `next` pointer if we end up inserting
a node after it.

<aside name="head">

It's a singly-linked list. It's not like we have any other choice than to start
at the head and go forward from there.

</aside>

There are three ways we can exit the loop:

1.  The local slot for the upvalue we stopped at is the slot we're looking for.
    We found an existing upvalue for the slot, so we reuse it.

2.  We ran out of upvalues to search. When `upvalue` is `NULL`, it means every
    open upvalue in the list points to locals above the slot we're looking for,
    or (more likely) the upvalue list is empty. Either way, we didn't find an
    upvalue for our slot.

3.  We found an upvalue whose local slot is *below* the one we're looking for.
    Since the list is sorted, that means we've gone past the slot we are closing
    over and thus there must not be an existing upvalue for it.

In the first case, we're done and we've returned. Otherwise, we need to create
a new upvalue for our local slot and insert it into the list at the right
location:

^code insert-upvalue-in-list (1 before, 1 after)

We already create the upvalue in the first incarnation of this function, so we
only need to add code to insert it in the list. We exited the list traversal by
either going past the end of the list, or by stopping on the first upvalue whose
stack slot is below the one we're looking for. In either case, that means we
need to insert the new upvalue *before* `upvalue` (which may be `NULL` if we hit
the end of the list).

**todo: illustrate**

Like you learned in Data Structures 101, to insert a node into a linked list,
you set the `next` pointer of the previous node to point to your new one. We
have been conveniently keeping track of that node as we walked the list. We also
need to handle the <span name="double">special</span> case where we are
inserting a new upvalue at the head of the list, in which case the "next"
pointer is the VM's head pointer.

<aside name="double">

There is a shorter implementation that handles updating either the head pointer
or the previous upvalue's `next` pointer uniformly by using a pointer to a
pointer, but that kind of code confuses almost everyone who hasn't reached some
Zen master level of pointer expertise. I went with the basic if statement
approach.

</aside>

With this updated function, the VM now ensures that there is only ever a single
ObjUpvalue for any given local slot. If two closures capture the same variable,
they will get the same upvalue. We're ready to move those upvalues off the
stack now.

### Closing upvalues at runtime

The compiler helpfully tells the interpreter exactly when it needs to hoist a
local variable into the heap by emitting an `OP_CLOSE_UPVALUE` instruction:

^code interpret-close-upvalue (2 before, 1 after)

At the point in time that we execute that, the variable we are hoisting is right
on top of the stack. We call a helper function, passing a pointer to that stack
slot. That function is responsible for closing the upvalue and moving the local
from the stack to the heap. After that, the VM is free to discard the stack
slot, which it does by calling `pop()`.

The fun stuff happens here:

^code close-upvalues

This function takes a pointer to a stack slot. It closes every open upvalue it
can find that points to that slot or any slot above it on the stack. Right now,
we only pass a pointer to the top slot on the stack, so the "or above it" part
doesn't come into play, but it will soon.

To do this, we walk the VM's list of open upvalues, again from top to bottom. If
an upvalue's local slot is within the range of slots we're closing, we close the
upvalue. Otherwise, once we reach an upvalue outside of the range, we know the
rest will be too so we stop iterating.

The way an upvalue gets closed is pretty <span name="cool">cool</span>. First,
we copy the variable's value into the `closed` field in the ObjUpvalue.
Remember, I said that's where closed-over variables will live in the heap.
The `OP_GET_UPVALUE` and `OP_SET_UPVALUE` instructions need to look for the
variable there after it's been moved. We could add some conditional logic in the
code that interprets those instructions to check some flag for whether the
upvalue is open or closed.

But there is already a level of indirection in play
-- those instructions dereference the `value` pointer to get to the variable's
value. In other words, the `value` field represents the *location* of a
variable. When the variable moves from the stack to the `closed` field, we
simply update that location by setting `value` to the address of the
ObjUpvalue's *own `closed` field.*

<aside name="cool">

I'm not praising myself here. This is all the Lua dev team's innovation.

</aside>

**todo: illustrate**

We don't need to change how `OP_GET_UPVALUE` and `OP_SET_UPVALUE` are
interpreted at all. That keeps them simple, which in turn keeps them fast. We do
need to add the new field to ObjUpvalue, though:

^code closed-field (1 before, 1 after)

And we should zero it out when we create an ObjUpvalue so there's no garbage
data floating around in the heap:

^code init-closed (1 before, 1 after)

Whenever the compiler reaches the end of a block, it discards all local
variables in that block and emits an `OP_CLOSE_UPVALUE` for each local variable
that was closed over. The compiler <span name="close">does</span> *not* emit any
instructions for the end the initial block scope that surrounds a function and
contains the function's parameters and any locals declared immediately inside
the body of the function. Those need to get closed too.

<aside name="close">

There's nothing *preventing* us from closing the outermost function scope in the
compiler and emitting `OP_POP` and `OP_CLOSE_UPVALUE` instructions. Doing so was
just unnecessary because the runtime discards all of the stack slots used by the
function implicitly when it pops the callframe.

</aside>

This is the reason `closeUpvalues()` accepts a pointer to a stack slot. When a
function returns, we call that and pass in the first stack slot owned by the
function:

^code return-close-upvalues (1 before, 2 after)

The `closeUpvalues()` function loops through all of the open upvalues and closes
any it finds that are at or above that slot. By passing the first slot in the
function's stack window, we close every remaining open upvalue owned by the
returning function. And with that, we now have a fully functioning closure
implementation. Closed-over variables live as long as needed by the
functions that capture them.

This was a lot of work! In jlox, closures fell out naturally from our
environment representation. In clox, we had to add large amount of code -- new
bytecode instructions, more data structures in the compiler, and new runtime
objects. The VM very much treats variables in closures as different from other
variables.

There is a rationale for that. In terms of implementation complexity, jlox gave
us closures "for free". But in terms of *performance*, jlox's closures are
anything but free. By allocating *all* environments on the heap, jlox pays a
significant performance price for *all* local variables, even the majority which
are never captured by local variables.

With clox, we have a more complex system, but that allows us to tailor the
implementation to fit the two use patterns we observe for local variables. For
most variables which do have stack semantics, we allocate them entirely on the
stack which is simple and fast. Then, for the few local variables where that
doesn't work, we have a second slower path we can opt in to as needed.

Fortunately, users don't perceive the complexity. From their perspective, local
variables in Lox are simple and uniform. The *language itself* is as simple as
jlox's implementation. But under the hood, clox is watching what the user does
and optimizing for their specific uses. As your language implementations grow in
sophistication, you'll find yourself doing this more. A large fraction of
"optimization" is about adding special case code that detects certain usage
patterns and provides a custom-build faster path for code that fits that
pattern.

We have lexical scoping fully working in clox now, which is a major milestone.
And, now that we have functions and variables with complex lifetimes, we also
have a *lot* of objects floating around in clox's heap, with a web of pointers
stringing them together. The [next step][] is figuring out how to manage that
memory so that we can free some of those objects when they're no longer needed.

[next step]: garbage-collection.html

<div class="challenges">

## Challenges

1.  Wrapping every ObjFunction in an ObjClosure introduces a level of
    indirection that has a performance cost. That cost isn't necessary for
    functions that do not close over any variables, but it does let the runtime
    treat all calls uniformly.

    Change clox to only wrap functions in ObjClosures that need upvalues. How
    does the code complexity and performance compare to always wrapping
    functions? Take care to benchmark programs that do and do not use closures.
    How should you weight the importance of each benchmark? If one gets slower
    and one faster, how do you decide what trade-off to make to choose an
    implementation strategy?

2.  Read the design note below. I'll wait. Now, how do you think Lox *should*
    behave? Change the implementation to create a new variable for each loop
    iteration.

3.  A [famous koan][koan] teaches us that "objects are a poor man's closure"
    (and vice versa). Our VM doesn't support objects yet, but now that we have
    closures we can approximate them. Using closures, write a Lox program that
    expresses two-dimensional vector "objects". It should:

    *   Define a "constructor" function to create a new vector with the given
        *x* and *y* coordinates.

    *   Provide "methods" to access the `x` and `y` coordinates of the value
        returned from that constructor.

    *   Define an addition "method" that adds two vectors and produces a third.


[koan]: http://wiki.c2.com/?ClosuresAndObjectsAreEquivalent

</div>

<div class="design-note">

## Design Note: Closing Over the Loop Variable

Closures capture variables. If two different variables capture the same
variable, then they share a reference to the same underlying storage location.
This fact is visible when new values are assigned to the variable. Obviously, if
two closures capture *different* variables, there is no sharing:

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

This prints "one" then "two". In cases like this, it's pretty clear that the two
variables are different. But it's not always so obvious. Consider:

```lox
var globalOne;
var globalTwo;

fun main() {
  for (var a = 1; a <= 2; a = a + 1) {
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

The code is convoluted because Lox has no collection types. Our clox
implementation doesn't even have objects yet. The important part is that the
`main()` function does two iterations of a for loop. Each time through the loop,
it creates a closure that captures the loop variable. It stores the first
closure in `globalOne` and the second in `globalTwo`.

There are definitely two different closures. Do they close over two different
variables? Is there only one `a` for the entire duration of the loop, or does
each iteration get its own distinct `a` variable?

The script here is strange and contrived, but this does show up in real code
in languages that aren't as minimal as clox. Here's a JavaScript example:

```js
var closures = [];
for (var i = 1; i <= 2; i++) {
  closures.push(function () { console.log(i); });
}

closures[0]();
closures[1]();
```

Does this print "1" then "2", or does it print <span name="three">"3"</span>
twice? You may be surprised to hear that it prints "3" twice. In this JavaScript
program, there is only a single `i` variable whose lifetime includes all
iterations of the loop.

<aside name="three">

You're wondering how *three* enters the picture? After the second iteration,
`i++` is executed, which increments `i` to three. That's what causes `i <= 2` to
evaluate to false and end the loop. If `i` never reached three, the loop would
run forever.

</aside>

If you're familiar with JavaScript, you probably know that variables declared
using `var` are implicitly *hoisted* to the surrounding function or top level
scope. It's as if you really wrote:

```js
var closures = [];
var i;
for (i = 1; i <= 2; i++) {
  closures.push(function () { console.log(i); });
}

closures[0]();
closures[1]();
```

At that point, it's clearer that there is only a single `i`. Now consider if
you change the program to use the newer `let` keyword:

```js
var closures = [];
for (let i = 1; i <= 2; i++) {
  closures.push(function () { console.log(i); });
}

closures[0]();
closures[1]();
```

Does it behave the same? Nope. In this case, it prints "1" then "2". Each
closure gets its own `i`. That's sort of strange when you think about it. The
increment clause is `i++`. That looks very much like it is assigning to and
mutating an existing variable, not creating a new one.

Let's try some other languages. Here's Python:

```python
closures = []
for i in range(1, 3):
  closures.append(lambda: print(i))

closures[0]()
closures[1]()
```

Python doesn't really have block scope. Variables are implicitly declared and
are automatically scoped to the surrounding function. Kind of like hoisting in
JS, now that I think about it. So this prints "3" twice.

What about Ruby? Ruby has two typical ways to iterate numerically. Here's the
classic imperative style:

```ruby
closures = []
for i in 1..2 do
  closures << lambda { puts i }
end

closures[0].call
closures[1].call
```

This, like Python, prints "3" twice. But the more idiomatic Ruby style is using
a higher-order `each()` method on range objects:

```ruby
closures = []
(1..2).each do |i|
  closures << lambda { puts i }
end

closures[0].call
closures[1].call
```

If you're not familiar with Ruby, the `do |i| ... end` part is basically a
closure that gets created and passed to the `each()` method. The `|i|` is the
parameter signature for the closure. The `each()` method invokes that closure
twice, passing in 1 for `i` the first time and 2 the second time.

In this case, the "loop variable" is really a closure parameter. And, since each
iteration of the loop is a separate invocation of the closure, those are
definitely separate variables for each call. So this prints "1" then "2".

If a language has a higher-level iterator-based looping structure like `foreach`
in C#, Java's "enhanced for", `for-of` in JavaScript, `for-in` in Dart, etc.
then I think it's natural to the reader to have each iteration create a new
variable. The code *looks* like a new variable because the loop header looks
like a variable declaration. And there's no increment expression that looks like
it's mutating that variable to advance to the next step.

If you dig around StackOverflow and other places, you can find clear evidence
that this is what users expect because they are very surprised when they *don't*
get it. In particular, C# originally did *not* create a new loop variable for
each iteration of a `foreach` loop. This was such a frequent source of user
confusion that they took the very rare step of shipping a breaking change to the
language. In C# 5, each iteration creates a fresh variable.

Old C-style for loops are harder. The increment clause really does look like
mutation. That implies there is a single variable that's getting updated each
step. But it's almost never *useful* for each iteration to share a loop
variable. The only time you can even detect this is when closures capture it.
And it's rarely helpful to have a closure that references a variable whose value
is whatever value caused you to exit the loop.

The pragmatically useful answer is probably to do what JavaScript does with
`let` in for loops. Make it look like mutation but actually create a new
variable each time because that's what users want. It is kind of weird when you
think about it, though.

</div>
