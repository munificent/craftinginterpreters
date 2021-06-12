> And as imagination bodies forth<br>
> The forms of things unknown, the poet's pen<br>
> Turns them to shapes and gives to airy nothing<br>
> A local habitation and a name.
> <cite>William Shakespeare, <em>A Midsummer Night's Dream</em></cite>

The [last chapter][] introduced variables to clox, but only of the <span
name="global">global</span> variety. In this chapter, we'll extend that to
support blocks, block scope, and local variables. In jlox, we managed to pack
all of that and globals into one chapter. For clox, that's two chapters worth of
work partially because, frankly, everything takes more effort in C.

<aside name="global">

There's probably some dumb "think globally, act locally" joke here, but I'm
struggling to find it.

</aside>

[last chapter]: global-variables.html

But an even more important reason is that our approach to local variables will
be quite different from how we implemented globals. Global variables are late
bound in Lox. "Late" in this context means "resolved after compile time". That's
good for keeping the compiler simple, but not great for performance. Local
variables are one of the most-used <span name="params">parts</span> of a
language. If locals are slow, *everything* is slow. So we want a strategy for
local variables that's as efficient as possible.

<aside name="params">

Function parameters are also heavily used. They work like local variables too,
so we'll use the same implementation technique for them.

</aside>

Fortunately, lexical scoping is here to help us. As the name implies, lexical
scope means we can resolve a local variable just by looking at the text of the
program -- locals are *not* late bound. Any processing work we do in the
compiler is work we *don't* have to do at runtime, so our implementation of
local variables will lean heavily on the compiler.

## Representing Local Variables

The nice thing about hacking on a programming language in modern times is
there's a long lineage of other languages to learn from. So how do C and Java
manage their local variables? Why, on the stack, of course! They typically use
the native stack mechanisms supported by the chip and OS. That's a little too
low level for us, but inside the virtual world of clox, we have our own stack we
can use.

Right now, we only use it for holding on to **temporaries** -- short-lived blobs
of data that we need to remember while computing an expression. As long as we
don't get in the way of those, we can stuff our local variables onto the stack
too. This is great for performance. Allocating space for a new local requires
only incrementing the `stackTop` pointer, and freeing is likewise a decrement.
Accessing a variable from a known stack slot is an indexed array lookup.

We do need to be careful, though. The VM expects the stack to behave like, well,
a stack. We have to be OK with allocating new locals only on the top of the
stack, and we have to accept that we can discard a local only when nothing is
above it on the stack. Also, we need to make sure temporaries don't interfere.

Conveniently, the design of Lox is in <span name="harmony">harmony</span> with
these constraints. New locals are always created by declaration statements.
Statements don't nest inside expressions, so there are never any temporaries on
the stack when a statement begins executing. Blocks are strictly nested. When a
block ends, it always takes the innermost, most recently declared locals with
it. Since those are also the locals that came into scope last, they should be on
top of the stack where we need them.

<aside name="harmony">

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

Step through this example program and watch how the local variables come in and
go out of scope:

<img src="image/local-variables/scopes.png" alt="A series of local variables come into and out of scope in a stack-like fashion." />

See how they fit a stack perfectly? It seems that the stack will work for
storing locals at runtime. But we can go further than that. Not only do we know
*that* they will be on the stack, but we can even pin down precisely *where*
they will be on the stack. Since the compiler knows exactly which local
variables are in scope at any point in time, it can effectively simulate the
stack during compilation and note <span name="fn">where</span> in the stack each
variable lives.

We'll take advantage of this by using these stack offsets as operands for the
bytecode instructions that read and store local variables. This makes working
with locals deliciously fast -- as simple as indexing into an array.

<aside name="fn">

In this chapter, locals start at the bottom of the VM's stack array and are
indexed from there. When we add [functions][], that scheme gets a little more
complex. Each function needs its own region of the stack for its parameters and
local variables. But, as we'll see, that doesn't add as much complexity as you
might expect.

[functions]: calls-and-functions.html

</aside>

There's a lot of state we need to track in the compiler to make this whole thing
go, so let's get started there. In jlox, we used a linked chain of "environment"
HashMaps to track which local variables were currently in scope. That's sort of
the classic, schoolbook way of representing lexical scope. For clox, as usual,
we're going a little closer to the metal. All of the state lives in a new
struct.

^code compiler-struct (1 before, 2 after)

We have a simple, flat array of all locals that are in scope during each point in
the compilation process. They are <span name="order">ordered</span> in the array
in the order that their declarations appear in the code. Since the instruction
operand we'll use to encode a local is a single byte, our VM has a hard limit on
the number of locals that can be in scope at once. That means we can also give
the locals array a fixed size.

<aside name="order">

We're writing a single-pass compiler, so it's not like we have *too* many other
options for how to order them in the array.

</aside>

^code uint8-count (1 before, 2 after)

Back in the Compiler struct, the `localCount` field tracks how many locals are
in scope -- how many of those array slots are in use. We also track the "scope
depth". This is the number of blocks surrounding the current bit of code we're
compiling.

Our Java interpreter used a chain of maps to keep each block's variables
separate from other blocks'. This time, we'll simply number variables with the
level of nesting where they appear. Zero is the global scope, one is the first
top-level block, two is inside that, you get the idea. We use this to track
which block each local belongs to so that we know which locals to discard when a
block ends.

Each local in the array is one of these:

^code local-struct (1 before, 2 after)

We store the name of the variable. When we're resolving an identifier, we
compare the identifier's lexeme with each local's name to find a match. It's
pretty hard to resolve a variable if you don't know its name. The `depth` field
records the scope depth of the block where the local variable was declared.
That's all the state we need for now.

This is a very different representation from what we had in jlox, but it still
lets us answer all of the same questions our compiler needs to ask of the
lexical environment. The next step is figuring out how the compiler *gets* at
this state. If we were <span name="thread">principled</span> engineers, we'd
give each function in the front end a parameter that accepts a pointer to a
Compiler. We'd create a Compiler at the beginning and carefully thread it
through each function call... but that would mean a lot of boring changes to
the code we already wrote, so here's a global variable instead:

<aside name="thread">

In particular, if we ever want to use our compiler in a multi-threaded
application, possibly with multiple compilers running in parallel, then using a
global variable is a *bad* idea.

</aside>

^code current-compiler (1 before, 1 after)

Here's a little function to initialize the compiler:

^code init-compiler

When we first start up the VM, we call it to get everything into a clean state.

^code compiler (1 before, 1 after)

Our compiler has the data it needs, but not the operations on that data. There's
no way to create and destroy scopes, or add and resolve variables. We'll add
those as we need them. First, let's start building some language features.

## Block Statements

Before we can have any local variables, we need some local scopes. These come
from two things: function bodies and <span name="block">blocks</span>. Functions
are a big chunk of work that we'll tackle in [a later chapter][functions], so
for now we're only going to do blocks. As usual, we start with the syntax. The
new grammar we'll introduce is:

```ebnf
statement      → exprStmt
               | printStmt
               | block ;

block          → "{" declaration* "}" ;
```

<aside name="block">

When you think about it, "block" is a weird name. Used metaphorically, "block"
usually means a small indivisible unit, but for some reason, the Algol 60
committee decided to use it to refer to a *compound* structure -- a series of
statements. It could be worse, I suppose. Algol 58 called `begin` and `end`
"statement parentheses".

<img src="image/local-variables/block.png" alt="A cinder block." class="above" />

</aside>

Blocks are a kind of statement, so the rule for them goes in the `statement`
production. The corresponding code to compile one looks like this:

^code parse-block (2 before, 1 after)

After <span name="helper">parsing</span> the initial curly brace, we use this
helper function to compile the rest of the block:

<aside name="helper">

This function will come in handy later for compiling function bodies.

</aside>

^code block

It keeps parsing declarations and statements until it hits the closing brace. As
we do with any loop in the parser, we also check for the end of the token
stream. This way, if there's a malformed program with a missing closing curly,
the compiler doesn't get stuck in a loop.

Executing a block simply means executing the statements it contains, one after
the other, so there isn't much to compiling them. The semantically interesting
thing blocks do is create scopes. Before we compile the body of a block, we call
this function to enter a new local scope:

^code begin-scope

In order to "create" a scope, all we do is increment the current depth. This is
certainly much faster than jlox, which allocated an entire new HashMap for
each one. Given `beginScope()`, you can probably guess what `endScope()` does.

^code end-scope

That's it for blocks and scopes -- more or less -- so we're ready to stuff some
variables into them.

## Declaring Local Variables

Usually we start with parsing here, but our compiler already supports parsing
and compiling variable declarations. We've got `var` statements, identifier
expressions and assignment in there now. It's just that the compiler assumes
all variables are global. So we don't need any new parsing support, we just need
to hook up the new scoping semantics to the existing code.

<img src="image/local-variables/declaration.png" alt="The code flow within varDeclaration()." />

Variable declaration parsing begins in `varDeclaration()` and relies on a couple
of other functions. First, `parseVariable()` consumes the identifier token for
the variable name, adds its lexeme to the chunk's constant table as a string,
and then returns the constant table index where it was added. Then, after
`varDeclaration()` compiles the initializer, it calls `defineVariable()` to emit
the bytecode for storing the variable's value in the global variable hash table.

Both of those helpers need a few changes to support local variables. In
`parseVariable()`, we add:

^code parse-local (1 before, 1 after)

First, we "declare" the variable. I'll get to what that means in a second. After
that, we exit the function if we're in a local scope. At runtime, locals aren't
looked up by name. There's no need to stuff the variable's name into the
constant table, so if the declaration is inside a local scope, we return a dummy
table index instead.

Over in `defineVariable()`, we need to emit the code to store a local variable
if we're in a local scope. It looks like this:

^code define-variable (1 before, 1 after)

Wait, what? Yup. That's it. There is no code to create a local variable at
runtime. Think about what state the VM is in. It has already executed the code
for the variable's initializer (or the implicit `nil` if the user omitted an
initializer), and that value is sitting right on top of the stack as the only
remaining temporary. We also know that new locals are allocated at the top of
the stack... right where that value already is. Thus, there's nothing to do. The
temporary simply *becomes* the local variable. It doesn't get much more
efficient than that.

<span name="locals"></span>

<img src="image/local-variables/local-slots.png" alt="Walking through the bytecode execution showing that each initializer's result ends up in the local's slot." />

<aside name="locals">

The code on the left compiles to the sequence of instructions on the right.

</aside>

OK, so what's "declaring" about? Here's what that does:

^code declare-variable

This is the point where the compiler records the existence of the variable. We
only do this for locals, so if we're in the top-level global scope, we just bail
out. Because global variables are late bound, the compiler doesn't keep track of
which declarations for them it has seen.

But for local variables, the compiler does need to remember that the variable
exists. That's what declaring it does -- it adds it to the compiler's list of
variables in the current scope. We implement that using another new function.

^code add-local

This initializes the next available Local in the compiler's array of variables.
It stores the variable's <span name="lexeme">name</span> and the depth of the
scope that owns the variable.

<aside name="lexeme">

Worried about the lifetime of the string for the variable's name? The Local
directly stores a copy of the Token struct for the identifier. Tokens store a
pointer to the first character of their lexeme and the lexeme's length. That
pointer points into the original source string for the script or REPL entry
being compiled.

As long as that string stays around during the entire compilation process --
which it must since, you know, we're compiling it -- then all of the tokens
pointing into it are fine.

</aside>

Our implementation is fine for a correct Lox program, but what about invalid
code? Let's aim to be robust. The first error to handle is not really the user's
fault, but more a limitation of the VM. The instructions to work with local
variables refer to them by slot index. That index is stored in a single-byte
operand, which means the VM only supports up to 256 local variables in scope at
one time.

If we try to go over that, not only could we not refer to them at runtime, but
the compiler would overwrite its own locals array, too. Let's prevent that.

^code too-many-locals (1 before, 1 after)

The next case is trickier. Consider:

```lox
{
  var a = "first";
  var a = "second";
}
```

At the top level, Lox allows redeclaring a variable with the same name as a
previous declaration because that's useful for the REPL. But inside a local
scope, that's a pretty <span name="rust">weird</span> thing to do. It's likely
to be a mistake, and many languages, including our own Lox, enshrine that
assumption by making this an error.

<aside name="rust">

Interestingly, the Rust programming language *does* allow this, and idiomatic
code relies on it.

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

<aside name="negative">

Don't worry about that odd `depth != -1` part yet. We'll get to what that's
about later.

</aside>

Local variables are appended to the array when they're declared, which means the
current scope is always at the end of the array. When we declare a new variable,
we start at the end and work backward, looking for an existing variable with the
same name. If we find one in the current scope, we report the error. Otherwise,
if we reach the beginning of the array or a variable owned by another scope,
then we know we've checked all of the existing variables in the scope.

To see if two identifiers are the same, we use this:

^code identifiers-equal

Since we know the lengths of both lexemes, we check that first. That will fail
quickly for many non-equal strings. If the <span name="hash">lengths</span> are
the same, we check the characters using `memcmp()`. To get to `memcmp()`, we
need an include.

<aside name="hash">

It would be a nice little optimization if we could check their hashes, but
tokens aren't full LoxStrings, so we haven't calculated their hashes yet.

</aside>

^code compiler-include-string (1 before, 2 after)

With this, we're able to bring variables into being. But, like ghosts, they
linger on beyond the scope where they are declared. When a block ends, we need
to put them to rest.

^code pop-locals (1 before, 1 after)

When we pop a scope, we walk backward through the local array looking for any
variables declared at the scope depth we just left. We discard them by simply
decrementing the length of the array.

There is a runtime component to this too. Local variables occupy slots on the
stack. When a local variable goes out of scope, that slot is no longer needed
and should be freed. So, for each variable that we discard, we also emit an
`OP_POP` <span name="pop">instruction</span> to pop it from the stack.

<aside name="pop">

When multiple local variables go out of scope at once, you get a series of
`OP_POP` instructions that get interpreted one at a time. A simple optimization
you could add to your Lox implementation is a specialized `OP_POPN` instruction
that takes an operand for the number of slots to pop and pops them all at once.

</aside>

## Using Locals

We can now compile and execute local variable declarations. At runtime, their
values are sitting where they should be on the stack. Let's start using them.
We'll do both variable access and assignment at the same time since they touch
the same functions in the compiler.

We already have code for getting and setting global variables, and -- like good
little software engineers -- we want to reuse as much of that existing code as
we can. Something like this:

^code named-local (1 before, 2 after)

Instead of hardcoding the bytecode instructions emitted for variable access and
assignment, we use a couple of C variables. First, we try to find a local
variable with the given name. If we find one, we use the instructions for
working with locals. Otherwise, we assume it's a global variable and use the
existing bytecode instructions for globals.

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
backward so that we find the *last* declared variable with the identifier. That
ensures that inner local variables correctly shadow locals with the same name in
surrounding scopes.

At runtime, we load and store locals using the stack slot index, so that's what
the compiler needs to calculate after it resolves the variable. Whenever a
variable is declared, we append it to the locals array in Compiler. That means
the first local variable is at index zero, the next one is at index one, and so
on. In other words, the locals array in the compiler has the *exact* same layout
as the VM's stack will have at runtime. The variable's index in the locals array
is the same as its stack slot. How convenient!

If we make it through the whole array without finding a variable with the given
name, it must not be a local. In that case, we return `-1` to signal that it
wasn't found and should be assumed to be a global variable instead.

### Interpreting local variables

Our compiler is emitting two new instructions, so let's get them working. First
is loading a local variable:

^code get-local-op (1 before, 1 after)

And its implementation:

^code interpret-get-local (1 before, 1 after)

It takes a single-byte operand for the stack slot where the local lives. It
loads the value from that index and then pushes it on top of the stack where
later instructions can find it.

<aside name="slot">

It seems redundant to push the local's value onto the stack since it's already
on the stack lower down somewhere. The problem is that the other bytecode
instructions only look for data at the *top* of the stack. This is the core
aspect that makes our bytecode instruction set *stack*-based.
[Register-based][reg] bytecode instruction sets avoid this stack juggling at the
cost of having larger instructions with more operands.

[reg]: a-virtual-machine.html#design-note

</aside>

Next is assignment:

^code set-local-op (1 before, 1 after)

You can probably predict the implementation.

^code interpret-set-local (1 before, 1 after)

It takes the assigned value from the top of the stack and stores it in the stack
slot corresponding to the local variable. Note that it doesn't pop the value
from the stack. Remember, assignment is an expression, and every expression
produces a value. The value of an assignment expression is the assigned value
itself, so the VM just leaves the value on the stack.

Our disassembler is incomplete without support for these two new instructions.

^code disassemble-local (1 before, 1 after)

The compiler compiles local variables to direct slot access. The local
variable's name never leaves the compiler to make it into the chunk at all.
That's great for performance, but not so great for introspection. When we
disassemble these instructions, we can't show the variable's name like we could
with globals. Instead, we just show the slot number.

<aside name="debug">

Erasing local variable names in the compiler is a real issue if we ever want to
implement a debugger for our VM. When users step through code, they expect to
see the values of local variables organized by their names. To support that,
we'd need to output some additional information that tracks the name of each
local variable at each stack slot.

</aside>

^code byte-instruction

### Another scope edge case

We already sunk some time into handling a couple of weird edge cases around
scopes. We made sure shadowing works correctly. We report an error if two
variables in the same local scope have the same name. For reasons that aren't
entirely clear to me, variable scoping seems to have a lot of these wrinkles.
I've never seen a language where it feels completely <span
name="elegant">elegant</span>.

<aside name="elegant">

No, not even Scheme.

</aside>

We've got one more edge case to deal with before we end this chapter. Recall this strange beastie we first met in [jlox's implementation of variable resolution][shadow]:

[shadow]: resolving-and-binding.html#resolving-variable-declarations

```lox
{
  var a = "outer";
  {
    var a = a;
  }
}
```

We slayed it then by splitting a variable's declaration into two phases, and
we'll do that again here:

<img src="image/local-variables/phases.png" alt="An example variable declaration marked 'declared uninitialized' before the variable name and 'ready for use' after the initializer." />

As soon as the variable declaration begins -- in other words, before its
initializer -- the name is declared in the current scope. The variable exists,
but in a special "uninitialized" state. Then we compile the initializer. If at
any point in that expression we resolve an identifier that points back to this
variable, we'll see that it is not initialized yet and report an error. After we
finish compiling the initializer, we mark the variable as initialized and ready
for use.

To implement this, when we declare a local, we need to indicate the
"uninitialized" state somehow. We could add a new field to Local, but let's be a
little more parsimonious with memory. Instead, we'll set the variable's scope
depth to a special sentinel value, `-1`.

^code declare-undefined (1 before, 1 after)

Later, once the variable's initializer has been compiled, we mark it
initialized.

^code define-local (1 before, 2 after)

That is implemented like so:

^code mark-initialized

So this is *really* what "declaring" and "defining" a variable means in the
compiler. "Declaring" is when the variable is added to the scope, and "defining"
is when it becomes available for use.

When we resolve a reference to a local variable, we check the scope depth to see
if it's fully defined.

^code own-initializer-error (1 before, 1 after)

If the variable has the sentinel depth, it must be a reference to a variable in
its own initializer, and we report that as an error.

That's it for this chapter! We added blocks, local variables, and real,
honest-to-God lexical scoping. Given that we introduced an entirely different
runtime representation for variables, we didn't have to write a lot of code. The
implementation ended up being pretty clean and efficient.

You'll notice that almost all of the code we wrote is in the compiler. Over in
the runtime, it's just two little instructions. You'll see this as a continuing
<span name="static">trend</span> in clox compared to jlox. One of the biggest
hammers in the optimizer's toolbox is pulling work forward into the compiler so
that you don't have to do it at runtime. In this chapter, that meant resolving
exactly which stack slot every local variable occupies. That way, at runtime, no
lookup or resolution needs to happen.

<aside name="static">

You can look at static types as an extreme example of this trend. A statically
typed language takes all of the type analysis and type error handling and sorts
it all out during compilation. Then the runtime doesn't have to waste any time
checking that values have the proper type for their operation. In fact, in some
statically typed languages like C, you don't even *know* the type at runtime.
The compiler completely erases any representation of a value's type leaving just
the bare bits.

</aside>

<div class="challenges">

## Challenges

1.  Our simple local array makes it easy to calculate the stack slot of each
    local variable. But it means that when the compiler resolves a reference to
    a variable, we have to do a linear scan through the array.

    Come up with something more efficient. Do you think the additional
    complexity is worth it?

1.  How do other languages handle code like this:

    ```lox
    var a = a;
    ```

    What would you do if it was your language? Why?

1.  Many languages make a distinction between variables that can be reassigned
    and those that can't. In Java, the `final` modifier prevents you from
    assigning to a variable. In JavaScript, a variable declared with `let` can
    be assigned, but one declared using `const` can't. Swift treats `let` as
    single-assignment and uses `var` for assignable variables. Scala and Kotlin
    use `val` and `var`.

    Pick a keyword for a single-assignment variable form to add to Lox. Justify
    your choice, then implement it. An attempt to assign to a variable declared
    using your new keyword should cause a compile error.

1.  Extend clox to allow more than 256 local variables to be in scope at a time.

</div>
