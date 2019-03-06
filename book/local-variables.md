^title Local Variables
^part A Bytecode Virtual Machine

In the last chapter, we added support for variables, but only of the global
variety. In this chapter, we'll extend that to support blocks, block scope, and
local variables. In jlox, we managed to pack all of that into one chapter. For
clox, we're splitting it in two partially because, frankly, everything takes
more work in C.

But an even more important reason is that our approach to local variables will
be quite different from how we implemented globals. Global variables are late
bound in Lox. "Late" in this context really means "resolved after compile time".
That's good for keeping the compiler simple, but not great for performance.

Local variables are one of the most-used <span name="params">parts</span> of the
language. If locals are slow, *everything* is slow. So we want an implementation
strategy for creating, accessing, and assigning local variables that's as
efficient as possible.

<aside name="params">

Function parameters are also heavily used, but they work pretty much like local
variables and we'll use the same implementation technique for them.

</aside>

Fortunately, lexical scoping is here to help us. As the name implies, lexical
scope means we can resolve a local variable just by looking at the text of the
program. That in turn means we can do it at compile time. Any processing work we
do in the compiler is work we *don't* have to do at runtime, so we'll pick an
implementation strategy for local variables that leans heavily on the compiler.

## Representing Local Variables

The nice thing about hacking on a programming language in modern times is
there's a long lineage of other languages to learn from. So how do C and Java
manage their local variables? Why, on the stack, of course! They typically use
the native stack mechanisms supported by the chip and OS. That's a little too
low level for us, but our VM does already have its own stack.

Right now, we only use it for holding on to **temporaries** -- short-lived blobs
of data while we're computing an expression. As long as we don't get in the way
of those, we can use stuff our local variables on the stack too. This is great
for performance. "Allocating" space for a new local requires only incrementing
the `stackTop` pointer and freeing is just a decrement. Accessing a variable if
we know its stack slot is simply loading a value from an array.

We do need to be careful, though. The VM expects the stack to behave like, well,
a stack. We have to be OK with only allocating new locals on the top of the
stack, and we have to accept that we can only discard a local when nothing is
above it on the stack. Also, we need to make sure temporaries don't get in the
way.

Fortunately, the design of Lox is in <span name="harmony">harmony</span> with
these. New locals are always created by declaration statements. Statements don't
nest inside expressions, so there are never any temporaries on the stack when a
statement begins executing. Blocks are strictly nested. When a block ends, it
always takes the innermost, most recently declared locals with us. Since those
are also the locals that came into scope last, they should be on top of the
stack where we need them.

**todo: illustrate examples**

<aside name="harmony" class="bottom">

This alignment obviously isn't coincidental. I designed Lox to be amenable to
single-pass compilation to stack-based bytecode. But I didn't have to tweak the
language too much to fit in those restrictions. Most of its design should feel
pretty natural.

This is in large part because the history of languages is deeply tied to
single-pass compilation and -- to a lesser degree -- stack-based architectures.
Lox's block scoping follows a tradition stretching back to BCPL. As programmers,
our intuition of what's "normal" in a language is informed even today by the
hardware limitations of yesteryear.

</aside>

It sounds like the stack will work for storing locals at *runtime*. But we can
go a little further than that. Not only do we know that they will be on the
stack, but we can even pin down precisely *where* on the stack they will be.
Since the compiler knows exactly which local variables are in scope at any point
in time, it can effectively simulate the stack and track how far from the <span
name="fn">bottom</span> of the stack each local variable can be found.

We'll take advantage of this by using these stack offsets as operands for the
bytecode instructions to read and store local variables. This makes working with
locals deliciously fast -- it's as simple as indexing into an array.

<aside name="fn">

In this chapter, locals will live on the bottom of the stack and be indexed from
there. When we add [functions][], that scheme gets a little more complex. Each
function needs its own region of the stack for its parameters and local
variables. But, as we'll see, that doesn't add as much complexity as you might
expect.

[functions]: calls-and-functions.html

</aside>

That's the model we'll use to represent locals both at compile time and runtime.
There's a lot of state we need to track in the compiler to make the whole thing
go, so let's get started there. In order to resolve an identifier expression, we
know to know what local variables are currently in scope.

In jlox, we used a linked chain of "environment" HashMaps. That's sort of the
classic, schoolbook way of representing lexical scope. For clox, as usual, we're
gonna rock something a little closer to the metal. The state all lives in this
new struct:

^code compiler-struct (1 before, 2 after)

We have a simple flat array of all locals that are currently in scope at any
point during the compilation process. The are <span name="order">ordered</span>
in the array in the order that their declarations appear in the code. The array
has a fixed size. The operand we'll use to encode a reference to a local in a
bytecode instruction is a single byte, so our VM has a hard limit on the number
of locals that can be in scope at once:

<aside name="order">

We're writing a single pass compiler, so it's not like we have *too* many other
options.

</aside>

^code uint8-count (1 before, 2 after)

The `localCount` field tracks how many locals are in scope -- how many of those
array slots are in use. We also track the "scope depth". This is the number of
blocks surrounding the current bit of code we're compiling. We need to know
which block each local belongs to so that we know which locals to discard when a
block ends.

Our Java interpreter used a chain of maps to keep each block's variables
separate. This time, we'll simply number them with the level of nesting where
they appear. Zero is the global scope, one is the first top-level block, two is
inside that, you get the idea. This `scopeDepth` field then also tells us
whether we're in a local scope at all, or still in the global scope.

Each local in the array is one of these:

^code local-struct (1 before, 2 after)

We store the name of the variable. When we're resolving an identifier, we need
to compare the identifier's lexeme with the name of each local variable to find
a match. It's pretty hard to resolve a variable if you don't know its name.

The depth is the scope depth of the block that the local variable was declared
in. And that's all the state we need for now. This is a very different
representation from what we had in jlox, but it still lets us answer all of the
same questions our compiler needs to ask of the lexical environment.

The question is how does the compiler get at this state? If we were <span
name="thread">principled</span> engineers, we'd probably pass a pointer to a
Compiler around between all of the various functions in the front end. But that
would mean a lot of boring changes to the code we already wrote, so here's a
global variable:

<aside name="thread">

In particular, if we ever want to use our compiler in a multi-threaded
application, possibly with multiple compilers running in parallel, then using a
global variable is a *bad* idea.

</aside>

^code current-compiler (2 before, 2 after)

Here's a handy function to initialize a new compiler:

^code init-compiler

When we first start up the compiler, we call it to get everything into a clean
state:

^code compiler (1 before, 2 after)

Our compiler has the data it needs, but not the operations on that data. There's
no way to create and destroy scopes, or add and resolve variables. Don't worry,
we'll add those as we need them. Let's start building some language features.

## Block Statements

Before we can have any local variables, we need some local scopes. These come
from two things: function bodies and blocks. Functions are a big chunk of work
that we'll tackle in [another chapter][functions], so for now we're just going
to do blocks. As usual, we start with the syntax:

^code parse-block (2 before, 1 after)

Blocks are a kind of statement, and they start with a curly brace. After the
initial `{`, we use this <span name="helper">helper</span> function to compile
the rest of the block:

<aside name="helper">

We'll use this helper function again later for compiling function bodies.

</aside>

^code block

It keeps parsing declarations and statements until it hits the closing brace. As
we do with any loop in the parser, we also check for the end of the token
stream. This way, if there's a malformed program with a missing closing curly,
the compiler doesn't get stuck in a loop.

Each statement leaves the stack just like it started, so it's fine to compile a
series of them like this, just as we did when compiling the top level of the
program.

The semantically interesting thing blocks do is create scopes. Before we compile
the body of a block, we call this function to enter a new local scope:

^code begin-scope

In order to "create" a scope, all we do is increment the current depth. This is
certainly much faster than jlox which allocated an entire new HashMap for
each one. Given `beginScope()`, you can probably guess what `endScope()` does:

^code end-scope

That's it for blocks and scopes -- more or less -- so we're ready to stuff some
variables into them.

## Declaring Local Variables

Our compiler already supports the syntax and semantics for parsing and compiling
variable declarations. We've already got `var` statements, identifier
expressions and assignment in there. It's just that it assumes all variables are
global. So now, we don't need any new parsing support. We just need to hook up
the new scoping semantics to the existing code behaves.

`varDeclaration()` is the main function for parsing a variable declaration. It
relies on a couple of other functions. First, `parseLocal()` consumes the
identifier token for the variable name and adds its lexeme to the chunk's
constant table as a string. It returns the index of the string in the constant
table. Then, after `varDeclaration()` compiles the initializer, it calls
`defineVariable()` to emit the bytecode for storing the variable's value in the
global variable hash table.

Both of those helpers need a few changes to support local variables. First, in
`parseLocal()`, we add:

^code parse-local (1 before, 1 after)

First, we "declare" the variable. We'll get to what that means in a second. At
runtime, locals aren't looked up by name, so there's no need to stuff the
variable's name into the constant table. So, if the declaration is inside a
local scope, we skip that part and return a dummy table index instead.

Then over in `defineVariable()`, we need to emit the code to store a local
variable if we're in a local scope. It looks like this:

^code define-variable (1 before, 1 after)

Wait, what? Yup. That's it. There is no code to create a local variable at
runtime. Think about what state the VM is in. It has already executed the code
for the variable's initializer (or the implicit `nil` if the user omitted an
initializer), and that value is sitting right on top of the stack as the only
remaining temporary. We also know that new locals are allocated at the top of
the stack... right where that value already is. Thus, there's nothing to do. The
value is already right where we need it. It doesn't get much more efficient than
that!

**todo: illustrate.**

OK, so what's "declaring" about? Here's what that does:

^code declare-variable

This is the point where the compiler records the existence of the variable. We
only need to do this for locals, so if we're in the top level global scope, we
just bail out. Because global variables are late bound, the compiler doesn't
keep track of which declarations for them its seen.

But for local variables, the compiler does need to remember that the variable
exists. That's what declaring it does -- it adds it to the compiler's list of
variables in the current scope, using:

^code add-local

This creates a new Local and adds it to the compiler's array of variables. It
stores the <span name="lexeme">name</span> of the variable marks which scope
owns the variable. Now the compiler knows about the variable.

<aside name="lexeme">

In case you're laying awake at night worrying about the lifetime of the lexeme
string for the identifier, here's how it's managed. The Local directly stores a
copy of the Token struct for the identifier. Recall that Tokens store a pointer
to the first character of their lexeme and the lexeme's length. That pointer
points right into the original source string for the entire script or REPL entry
being compiled.

As long as that one monolithic string stays around during the entire compilation
process -- which it must since, you know, we're compiling it -- then all of the
tokens pointing into it are fine. This does mean that once compilation is
*done*, we do need to make sure we don't have any tokens laying around because
the source string they point at may be freed after that.

**todo: overlap**

</aside>

This code is fine for a correct Lox program, but what about invalid code? We
need to make this a little more robust. The first error to handle is not really
the user's fault, but more a limitation of the VM. The instructions to work with
local variables refer to them by slot index. That index is stored in a
single-byte operand, which means we can only support up to 255 local variables
in scope at one time.

If we try to go over that, not only could we not refer to them at runtime, the
compiler would overrwrite its own locals array. So first we check for that:

^code too-many-locals (1 before, 2 after)

The next case is trickier. Consider:

```lox
{
  var a = "first";
  var a = "second";
}
```

At the top level, Lox allows redeclaring a variable with the same name as a
previous declaration because that's useful for the REPL. But inside a local scope, that's a pretty <span name="rust">weird</span> thing to do. It's likely to be a mistake and many languages, including our own Lox, enshrine that assumption by making this an error.

<aside name="rust">

Interestingly, the Rust programming language *does* allow this and its idiomatic
to do so.

</aside>

Note that the above program is different from this one:

```lox
{
  var a = "outer";
  {
    var a = "inner";
  }
}
```

It's OK to have two variables with the same name in *different* scopes, even
when the scopes overlap such that both are visible at the same time. That's
shadowing, and Lox does allow that. It's only an error to have two variables
with the same name in the *same* local scope.

We detect that error like so:

^code existing-in-scope (1 before, 2 after)

Local variables are appended to the array when they're declared, which means the
current scope is always at the end of the array. When we declare a new variable,
we start at the end and work backwards looking for an existing variable with the
same name. If we find one in the current scope, we report the error. Otherwise,
if we reach the beginning of the array, or a variable owned by another scope,
then we know this scope is OK.

To see if two identifiers are the same we use:

^code identifiers-equal

Since we know the lengths of both lexemes, we check that first. That will fail
quickly for many non-equal strings. If the lengths are the same, we check the
characters using `memcmp()`. It would be nice if we could check their hashes,
but tokens aren't full LoxStrings, so we haven't calculated their hash yet.

To get to `memcmp()`, we need an include:

^code compiler-include-string (1 before, 2 after)

With this, we're able to bring variables into being. But, like ghosts, they
linger on beyond the scope where they are declared. When a block ends, we need
to put them to rest:

^code pop-locals (1 before, 1 after)

Whenever we pop a scope, we walk backwards through the local array looking for
any variables declared at the scope depth we just left. We discard them simply
by decrementing the length of the array.

There is a runtime component to this too. Local variables occupy slots on the
stack. When a local variable goes out of scope, that slot is no longer needed
and should be freed. So, for each variable that we discard, we also emit an
`OP_POP` <span name="pop">instruction</span> to pop it from the stack.

<aside name="pop">

When a number of local variables go out of scope at once, this emits a series of
`OP_POP` instructions, which each get interpreted one at a time. A simple
optimization you could add to your Lox implementation is a specialized `OP_POPN`
instruction that takes an operand for the number of slots to pop and pops them
all at once.

</aside>

## Using Locals

We can now compile and execute local variable declarations. At runtime, they
are sitting where they need to be on the stack. Let's start using them. We'll
do both variable access and assignment at the same time since they touch the
same functions in the compiler.

We already have code for getting and setting global variables, and -- like good
little software engineers -- we want to reuse as much of that existing code as
we can. So we do:

^code named-local (1 before, 2 after)

Instead of hardcoding the bytecode instructions emitted for variable access and
assignment, we use a couple of variables. First, we try to find a local variable
with the given name. If we find one, we emit the instructions for working with a
local variable. Otherwise, we assume it's a global variable and use the existing
bytecode instructions for globals.

A little further down, we use those variables to emit the right instructions.
For assignment:

^code emit-set (2 before, 1 after)

And for access:

^code emit-get (2 before, 1 after)

The real heart of this chapter, the part where we resolve a local variable, is
here:

^code resolve-local

For all that, it's straightforward. We walk the list of locals that are
currently in scope. If one has the same name as the identifier token, the
identifier must refer to that variable. We've found it! We walk the array
backwards so that we found the *last* declared variable with the identifier.
That ensures that inner local variables correctly shadow earlier locals with the
same name.

At runtime, we load and store locals using the stack slot index, so that's what
the compiler needs to calculate after it resolves the variable. Whenever a
variable is declared, we append it to the locals array in Compiler. That means
the first local variable is at index variable, the next one is at index one, and
so on. When a block ends, some number of trailing locals from the end of the
array are discarded.

In other words, the locals array in the compiler has the *exact* same layout as
the VM's stack will have at runtime. The variable's index in the locals array is
the same as its stack slot. How convenient!

If we make it through the whole array without finding a variable with the given
name, it must not be a local. In that case, we return `-1` to signal that it
wasn't found and should be assumed to be a global variable instead.

That's it for compiler support. Moving along the execution pipeline, we have two
new instructions. First is loading a local variable:

^code get-local-op (1 before, 1 after)

And its implementation:

^code interpret-get-local (1 before, 2 after)

It takes a single byte operand for the stack <span name="slot">slot</span> where
the local lives. It loads the value from that index and then pushes it on top of
the stack where later instructions can find it.

<aside name="slot">

It probably seems to redundant to push the local's value back onto the stack
since it's already in there lower down somewhere. The problem is that all of our
other bytecode instructions only know to find the data they need at the top of
the stack. This is the core aspect that makes our bytecode instruction set
*stack*-based. Some bytecode instruction sets are [register-based][reg] instead
and avoid this redundancy at the cost of having larger instructions with more
operands.

[reg]: a-virtual-machine.html#design-note

</aside>

Next is assignment:

^code set-local-op (1 before, 1 after)

You can probably predict the implementation:

^code interpret-set-local (1 before, 2 after)

It takes the assigned value from the top of the stack and stores it in the stack
slot corresponding to the local variable. Note that it doesn't pop the value
from the stack. Remember, assignment is an expression, and every expression
produces a value. The value of an assignment expression is the assigned value
itself, so the VM just leaves the value on the stack.

Most assignments are nestled right inside an expression statement. In that case,
the next instruction will be the `OP_POP` instruction emitted at the end of the
expression statement and the value disappears like we expect.

---

Our disassembler is incomplete without support for these two new instructions:

^code disassemble-local (1 before, 1 after)

The compiler compiles local variables to direct slot access. The local
variable's name never leaves the compiler to make it into the chunk at all.
That's great for performance, but not so great for <span
name="debug">introspection</span>. When we disassemble these instructions, we
can't show the variable's name like we could with globals. Instead, we just show
the slot:

<aside name="debug">

The fact that local variable names are erased by the compiler is a real issue if
we ever want to implement a debugger for our VM. When users step through code,
they expect to see all the local variables organized by name with their current
values.

In order to support, we would need to output some additional diagnostic
information the tracks the name of each local variable at each stack slot. Since
local variables come and go throughout the body of a function as blocks begin
and end, we'll multiple of these tables for regions of the function.

**todo: overlap**

</aside>

^code byte-instruction

## Another Scope Edge Case

We already sunk some time into handling a couple of weird edge cases around
scopes. We made sure shadowing works correctly. We report an error if two
variables in the same local scope have the same name. For reasons that aren't
entirely clear to me, variable scoping seems to have a lot of these wrinkles.
I've never seen a language where it feels completely elegant.

We've got one more to deal with before we end this chapter. Take a look at this
strange little monster:

```lox
{
  var a = "outer";
  {
    var a = a;
  }
}
```

I know, right? What was the <span name="author">author</span> even thinking when
they wrote this? But, really, though, what *were* they thinking? What do they
expect this to do? If anyone were to actually write this code, they probably
expect the `a` in the initializer to refer to the *outer* `a`. In other words,
they are initialized the shadowing inner `a` with the value of the shadowed one.
I suppose that's something someone could want to do.

<aside name="author">

I guess this is a rhetorical question since *I'm* the author of this dollop of
code. All I had in mind was showing you some ugly corner of the language that we
are obliged to deal with.

</aside>

But what about:

```lox
{
  var a = a;
}
```

The key question is when, precisely, does a variable come into scope. Is a
variable in scope in its own initializer? If the answer is "yes", that's
definitely not <span name="lambda">*useful*</span>. The variable doesn't *have*
a value yet... since we haven't finished executing its initializer.

<aside name="lambda">

It *could* be useful if Lox supported lambdas -- function expressions. In that
case, the initializer could be a function that referred to the variable being
initialized. As long as the function isn't *invoked* until after the initializer
completes, this would be a useful way to have a recursive local function.

</aside>

We could answer "no" and say the variable doesn't come into being until after
the initializer completes. That would enable the prior example. But, honestly,
code like that is really confusing. Programmers expect code to roughly execute
from left to right. It would feel strange if the variable didn't come into scope
as soon as you went "past" its name.

The safest solution is to simply make this a compile error. Tell the user they
did something odd and let them sort it out. In practice, you almost never see
code like this anyway, so it's safer to prohibit then try to support it with
some semantic few users are likely to guess correctly.

The way we make this work is by making "come into scope" a two-phase process. As
soon as the variable declaration begins -- in other words, before its
initializer -- the name is declared in the current scope. The variable exists,
but it's in a special "uninitialized" state.

Then we compile the initializer. If at any point in that expression we resolve
an identifier expression that points back to this variable, we'll see that its
uninitialized and flag it as an error. After we finish compiling the
initializer, we mark the variable as initialized and ready for normal use.

**todo: illustrate "var a = b + c;" with markers at "a" saying "declared
uninitialized" and a marker after "c" saying "ready for use".**

To implement this, when we declare a local, we need to indicate this
"uninitialized" state somehow. We could add a new field, but let's be a little
more parsimonious with memory. Instead, we'll set the variable's scope depth to
a special sentinel value, `-1`:

^code declare-undefined (1 before, 2 after)

Later, once the variable's initializer has been compiled, we mark it as being
initialized:

^code define-local (1 before, 2 after)

That calls:

^code mark-initialized

So this is *really* what "declaring" and "defining" a variable means in the
compiler. "Declaring" is when it's added to the scope, and "defining" is when it
becomes available for use.

When we resolve a reference to a local variable, we check the scope depth to see
if it's fully defined:

^code own-initializer-error (1 before, 1 after)

If the variable has the sentinel depth, it must be a reference to a variable in
its own initializer, and we report that as an error. And that's it for this
chapter!

We added blocks, local variables, and real, honest-to-God lexical scoping. Given
that we introduced an entirely different runtime representation for variables,
we didn't have to write a lot of code. The implementation ended up being pretty
clean and efficient.

You'll notice that almost all of the code we wrote is in the compiler. Over in
the runtime, it's just two little instructions. You'll see this as a continuing
<span name="static">trend</span> in clox compared to jlox. One of the biggest
hammers in the optimizer's toolbox is pulling work forward into the compiler so
that you don't have to do it at runtime. In this chapter, that meant resolving
exactly which stack slot every local variable occupies. That way, at runtime, no
"look up" or resolution needs to happen.

<aside name="static">

You can look at static types as an extreme example of this trend. A statically
typed language takes all of the type analysis and type error handling and sorts
it all out during compilation. Then the runtime doesn't have to waste any time
checking that values have the proper type for their operation. In fact, in some
statically typed languages, you don't even *know* the type at runtime. The
compiler completely erases any representation of a value's type leaving just the
bare bits.

</aside>

<div class="challenges">

## Challenges

1.  Our simple local array makes it simple to calculate the stack slot of each
    local variable. But it means that when the compiler resolves a reference to
    a variable, we have to do a linear scan through the array.

    Come up with something more efficient. Do you think the additional
    complexity is worth it?

1.  How do other languages handle code like:

        :::lox
        var a = a;

    What would you do? Why?

1.  Many languages make a distinction between variables that can be reassigned
    and those that can't. In Java, the `final` modifier prevents you from
    assigning to a variable. In JavaScript, a variable declared with `let` can
    be assigned but one declared using `const` can't. Swift treats `let` as
    single-assignment and uses `var` for assignable variables. Scala and Kotlin
    use `val` and `var`.

    Pick a keyword for a single-assignment variable form to add to Lox. Justify
    your choice, then implement it. An attempt to assign to a variable declared
    using your new keyword should cause a compile error.

</div>
