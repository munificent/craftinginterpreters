^title Types of Values
^part A Bytecode Virtual Machine

> When you are a Bear of Very Little Brain, and you Think of Things, you find
> sometimes that a Thing which seemed very Thingish inside you is quite
> different when it gets out into the open and has other people looking at it.
>
> <cite>A.A. Milne</cite>

The past few chapters were huge. Towering floors of scaffolding to set up. Deep
complex concepts to grapple with. This chapter is shorter and simpler. There's
only one real new concept to learn -- tagged unions -- and a scattering of
straightforward code. Relax, you're earned a respite.

Lox is dynamically typed. A single variable can hold a Boolean, number, or
string at different points in time. At least, that's the idea. <span
name="unityped">Right</span> now, in clox, the only values we support are
numbers.

<aside name="unityped">

There is a third category next to "statically typed" and "dynamically typed":
**"unityped"**. In that category, all variables have a single type, usually a
machine register integer. Languages like this aren't common today, but some
Forths and BCPL, the language that inspired C, were unityped.

</aside>

In this chapter, we'll get dynamic typing working by introducing two new types,
Booleans and `nil`. While those aren't super complex types, they force us to
define a value representation that can handle values of different types.

## Tagged Unions

The nice thing about working in C is that we can build our data structures from
the raw bits up. The bad thing is that we *have* to do that. C doesn't give you
much for free at compile time and even less at runtime. As far as C is
concerned, the universe is an undifferentiated array of bytes. It's up to us to
decide how many of those bytes to use and what they mean.

In order to choose a value representation, we need to answer two key questions:

1.  How do we represent the type of a value? If you try to, say, multiple a
    number by `true`, we need to detect that error at runtime and report it. In
    order to do that, we need to be able tell what a value's type is.

2.  How do we store the value itself? We need to not only be able to tell that
    3 is a number, but that it's a different number from 4. Likewise, we
    need to be able to tell that `true` and `false` are both Booleans, but not
    obviously not the same ones.

    **todo: move next para lower?**

    This is trickier because different types need different amounts of storage.
    We need <span name="nan">64</span> bits to store all the different double
    values, but only a single bit to tell `true` from `false`. If we have an
    array of values of mixed types, how big is each array element?

    <aside name="nan">

    As we'll see in the [last chapter], 64 bits for a double actually gives us a
    bunch of extra bits we can use for other things.

    </aside>

[last chapter]: optimization.html

As usual, our job as language implementers implies an extra constant question:
how do we solve the above efficiently?

Language implementations over the years have come up with a variety of clever
ways to pack the above information into as few bits as possible. For now, we'll
start with the simplest, classic solution, a **tagged union**.

A value contains two parts: a type "tag", and a payload for the actual value. To
store the value's type, we define an enum for each kind of value the VM
supports:

^code value-type (2 before, 2 after)

<span name="user-types">For</span> now, we only have a couple of cases, but this
will grow as we add strings, functions, and classes to the language.

<aside name="user-types">

Note that the cases here are for each kind of value that has unique built-in
support in the VM. When we get to adding classes to the language, each class the
user defines doesn't need its own entry here. As far as the VM is concerned,
every instance of a class is the same type: "instance".

</aside>

In addition to the type, we also need to store the data for the value, the
`double` for a number, `true` or `false` for a Boolean. We could define a struct
with fields for each possible type:

**todo: illustrate struct fields**

But this is a waste of memory. A value can't simultaneously be both a number and
a Boolean. So at any point in time, only one of those fields will be used. C
lets you optimize this by defining a union. In case you haven't used them
before, a union looks like a struct except that all of its fields overlap in
memory.

**todo: illustrate union fields and showing largest dictates size**

The size of a union is the size of its largest field. Since the fields all reuse
the same bits, you have to be very careful when using them. If you store data
using one field and then access it using <span
name="reinterpret">another</span>, you will reinterpret what the underlying bits
mean.

<aside name="reinterpret">

Using a union to reinterpret bits as multiple different types is the
quintessence of C. It opens up a number of clever optimizations and lets you
really make the most of out of each byte of memory in ways that most memory-safe
languages disallow.

But it can also open a pit that leads to undefined behavior and crawling hordes
of memory corruption bugs if you aren't careful.

</aside>

As the name "tagged union" implies, are value representation combines these two
parts in a single struct:

^code value (2 before, 2 after)

There's a field for the value's type, and then a second field containing the
union of all of the value types. A smart language hacker gave me the idea to use
"as" for the name of this field because it reads nicely, almost like a cast,
when you pull the value out.

**todo: illustrate**

On a 64-bit machine, one of these values is two 64-bit words or 16 bytes. One
word is for the largest union field, the double. And then another integer word
for the type tag. A 64-bit integer is beyond <span name="pad">overkill</span>
for an enum that only has three cases, so that's pretty wasteful. We'll improve
this later, but this is good enough for now. It has the real advantages of being
simple and easy to look at in a debugger.

<aside name="pad">

We could shrink the tag field to something like a byte, but in practice that
doesn't buy us much. On most architectures, the double value field needs to be
aligned to a 64-bit boundary. To accomplish that, the C compiler will add
padding between the 1-byte tag field and the union. We could move the tag field
after the union, but that doesn't help much either. Whenever we create an array
of Values -- which is where most of our memory usage for Values will be -- the C
compiler has to insert padding between each element to keep the doubles aligned.

</aside>

Because values are only two words, they're still plenty small enough to store on
the C stack and pass around by value. Doing that is also safe because the only
types we support so far are **immutable**. If we pass a copy of a Value containing
the number 3 to some function, we don't need worry about the caller seeing
modifications to the value. You can't "modify" three. It's three forever.

## Lox Values and C Values

That's it for our new value representation, but we're not done yet. Right now,
the rest of clox assumes Value is the same as `double`. We have code that does a
straight C cast from one to the other. That code is all broken now.

With our new representation, a Value can *contain* a double, but it's not
*equivalent* to it. There is an explicit conversion step needed to go from one
to the other. We need to go through the code and insert those conversions to fix
it all.

We'll implement these operations as a handful of macros, one for each type and
operation. First, to convert a raw C value to a clox Value:

^code value-macros (1 before, 2 after)

Each one of these takes a C value of the appropriate type and produces a Value
that has the correct type tag and contains the underlying value. I'm using the
C99 struct initializer syntax here because that's the easiest way to write an
expression that creates a struct.

So this hoists statically typed values up into clox's dynamically-typed world.
In order to *do* anything with a Value, though, we need to unpack it and get the
raw value back out:

^code as-macros (1 before, 2 after)

These macros go in the opposite direction. Given a Value of the right type, they
unwrap it and return the corresponding raw C value. The "right type" bit is
important! These macros directly access the union fields. If we were to do
something like:

```c
Value value = BOOL_VAL(true);
double number = AS_NUMBER(value);
```

Then we run the risk of opening a doorway to the Shadow Realm. It's not safe to
use any of the `AS_` macros unless we know the value contains the correct type.
To that end, we define a last few macros to check a value's type:

^code is-macros (1 before, 2 after)

These macros return `true` if the value has that type. Any time we call one of
the `AS_` macros, we need to <span name="check">guard</span> it behind a call to
one of these first. With these macros, we can now safely move data between Lox's
dynamic world and C's static one.

<aside name="check">

We do these checks at runtime, every time some operation is performed on a
value. This adds significant runtime overhead, and is a key part of why
dynamically-typed languages tend to be slower than their statically-typed
bretheren.

In a statically-typed language, the type system figures out the type of each
variable at compile time. That means the implementation *knows* what type a
value has so doesn't need to check it at runtime.

</aside>

## Dynamically-typed Numbers

We've got our value representation and the tools to convert to and from it. All
that's left to get clox working again is to grind through the code and fix every
place where data moves across that boundary. This is one of those sections of
the book that isn't exactly mind-blowing, but I promised I'd show you every
single line of code, so here we ware. Let's get through it fast.

The first values we create are the constants generated when we compile number
literals. After we convert the lexeme to a C double, we simply wrap it in a
Value before storing it in the constant table:

^code const-number-val (1 before, 1 after)

Over in the runtime, we have a function to print values:

^code print-number-value (1 before, 1 after)

Right before we send the value to `printf()`, we unwrap it and extract the
double value. We'll revisit this shortly when we add the other types, but let's
get our existing code unbroken first.

### Unary negation

The next simplest operation is unary negation. It pops a value off the stack,
negates it, and pushes the result. Now that we're thinking about other types of
values, we can't assume the operand is a number anymore. The use could just as
well do:

```lox
print -false; // Uh...
```

We need to handle that gracefully, which means it's time for **runtime errors**.
Before performing an operation that requires a certain type, we need to check
that the value has that type.

For unary negate, that check looks like this:

^code op-negate (1 before, 1 after)

First, we check to see if the value on top of the stack is a number. If we not,
we report the runtime error and <span name="halt">stop</span> the interpreter.
Otherwise, we keep going. Only after this validation can we unwrap the operand,
negate it, wrap the result and push it.

<aside name="halt">

Lox's approach to error-handling is... minimal, to say the least. All errors are
fatal and immediately halt the interpreter. There's no exception-handling or
other way to programmatically recover from an error. If Lox were a real
language, this is one of the first things I would remedy.

</aside>

To look at the value on the stack, we use:

^code peek

It returns a value on the stack but doesn't <span name="peek">pop</span> it. The
argument is how far down from the top of the stack to look: zero is the top, one
is one slot down, etc.

<aside name="peek">

Why not just pop the operand and then validate it? We could do that here. In
later chapters, it will be important to leave operands on the stack as long as
possible to ensure the garbage collector can still find them if a collection is
triggered in the middle of the operation. Doing the same thing here is mostly
habit.

</aside>

The runtime error is reported using this function that we'll be getting a lot of
mileage out of over the remainder of the book:

^code runtime-error

You've certainly *called* variadic functions -- ones that take an varying number
of arguments -- in C before: `printf()` is one. But you may not have *defined*
your own. This book isn't a C tutorial, so I'll skim it here, but basically the
`...` and `va_list` stuff let us pass a varying number of arguments to
`runtimeError()`. We forward those on to `vfprintf()`, which is the version of
`printf()` that takes a `va_list`.

In other words, callers can pass a format string to `runtimeError()` followed by
a number of arguments, just like they can when calling `printf()` directly.
`runtimeError()` will then format and print those arguments. We won't take
advantage of that in this chapter, but later chapters will produce runtime error
messages that contain other data in them.

After we show the hopefully helpful error message, we tell the user which <span
name="stack">line</span> of their code was being executed when the error
occurred. This is similar to how we report the location of a compile error.
There, we looked up the line information from the current token. We don't have
tokens anymore. Instead, the compile passed along that line information into the
chunk as it wrote each bytecode instruction.

<aside name="stack">

Just showing the immediate line where the error occurred doesn't provide much
context. Better would be a full stack trace. But we don't even have functions to
call yet, so there is no stack to trace.

</aside>

Now we read that back out and look up the line number associated with the
current bytecode instruction offset. If our compiler did its job right, that
will correspond to the line of source code that the bytecode was compiled from.

In order to use `va_list` and the macros for working with it, we need to bring
in a standard header:

^code include-stdarg (1 after)

OK, now our VM can not only do the right thing when you negate numbers (again),
but it also safely handles trying to negate other types.

### Binary arithmetic operators

Now that we have our runtime error machinery in place, fixing the binary
operators will be easier even though they're more complex. We support four
binary operators today: `+`, `-`, `*`, and `/`. The only difference between them
is which underlying C operator they rely on. To minimize redundant code between
the four operators, we wrapped up the commonality in a big preprocessor macro
that takes the operator as a parameter.

It felt like overkill when we built that in the [last chapter][], but we get to
benefit from it today. It lets us add the necessary type checking and
conversions in one place:

[last chapter]: compiling-expressions.html

^code binary-op (2 before, 2 after)

Yeah, I realize that's a monster of a macro. It's not what I'd normally consider
good C practice, but let's roll with it. The changes are similar to what we did
for unary negate. First we check that the two operands are both numbers. If
either isn't, we report a runtime error and yank the ejection seat level.

If the operands are fine, we pop them both and unwrap them. Then we apply the
given operator, wrap the result, and push it back on the stack. Note that we
don't wrap the result by directly using `NUMBER_VAL()`. Instead, the wrapper is
passed in as a `valueType` parameter.

For our existing arithmetic operators, the result is a number, so we pass in the
`NUMBER_VAL` macro:

^code op-arithmetic (1 before, 1 after)

The comparison operators `>` and `<` take numbers as *operands* but the *result*
is a Boolean. Taking the wrapping macro as an operand let's us reuse the
`BINARY_OP` macro for those operators too.

## Two New Types

Speaking of Booleans, it's time to add some new types. All of our existing clox
code is back in working order. We've got a running numeric calculator that now
does a number of pointless paranoid runtime type checks. We can represent other
types internally, but there's no way for a user's program to ever create a value
of one of those types.

Not until now, that is. We'll start by adding compiler support for the three new
literals: `true`, `false`, and `nil`. They're all pretty simple, so we'll do all
three in a single batch.

With number literals, we had to deal with the fact that there are billions of
possible numeric values. We attended to that by creating a constant for the
value in the chunk's constant table and emitting a bytecode instruction that
simply loaded that constant. We <span name="small">could</span> do the same
thing for the new types. We'd store, say, `true`, as a Value in the constant
table, and use an `OP_CONST` to read it out.

But given that there are literally only three possible values we need to worry
about with these new types, it seems like gratuitous -- and slow! -- to waste a
two-byte instruction and a const table entry on them. Instead, we'll define
three dedicated instructions to push the Boolean and null constants on the
stack:

<aside name="small">

The efficiency argument is real. A bytecode spends much of its execution time
reading and decoding instructions. The smaller and simpler you can make the
instructions, the faster it goes. Creating short instructions dedicated to
common operations is a classic optimization.

For example, the Java bytecode instruction set has dedicated instructions for
loading 0.0, 1.0, 2.0, and the integer values from -1 through 5. (This ends up
being a vestigial optimization given that most mature JVMs now JIT-compile the
bytecode to machine code before execution anyway.)

**todo: overlap**

</aside>

^code literal-ops (1 before, 1 after)

Our scanner already treats `true`, `false`, and `null` as keywords, so we can
skip right to the parser. With our table-based Pratt parser, we just need to
slot parser functions into the rows associated with those keyword token types.
We'll use the same function in all three slots:

^code table-false (1 before, 1 after)

And:

^code table-true (1 before, 1 after)

And finally:

^code table-nil (1 before, 1 after)

When the parser encounters `false`, `nil`, or `true`, in prefix position, it
calls:

^code parse-literal

Since `parsePrecedence()` has already consumed the keyword token, all we need to
do is output the proper instruction. We <span name="switch">look</span> at the
token type to see which literal it was. With this, our front end can now compile
Boolean and null literals to bytecode. Moving down the execution pipeline, we
reach the interpreter:

<aside name="switch">

We could have used separate parser functions for each literal and saved
ourselves a switch but that felt needlessly verbose to me. I think it's mostly a
matter of taste.

</aside>

^code interpret-literals (5 before, 1 after)

This is pretty self-explanatory. Each instructions summons the appropriate value
and pushes it onto the stack. We shouldn't forget our disassembler either:

^code disassemble-literals (2 before, 1 after)

With this in place, we can run this program:

```lox
true
```

Except that when the interpreter tries to print the result, it blows up. We need
to extend `printValue()` to handle the new types too:

^code print-value (1 before, 1 after)

There we go! Now we have some new types. They just aren't very useful yet. Aside
from the literals, you can't really *do* anything with them. It will be a while
before `null` comes into play, but we can start putting Booleans to work. We've
got enough in place now to support the comparison and equality operators.

---

### Logical not and falsiness

The simplest logical operator is our friend "bang", unary not:

```lox
print !true; // "false"
```

This new operation gets a new instruction:

^code not-op (1 before, 1 after)

We can reuse the `unary()` parser function we wrote for unary `-` to compile a
prefix `!` expression. We just need to slot it into the parsing table:

^code table-not (1 before, 1 after)

Because I knew were going to do this, that function already has a switch on the
token type to figure out which instruction to generate for the operator. All we
need to do now is add another case:

^code compile-not (1 before, 4 after)

That's it for the front end. Let's head over to the VM and conjure this
instruction into life:

^code op-not (1 before, 1 after)

Like our previous unary operator, it pops the one operand, performs the
operation, and pushes the result. And, as we did there, we have to worry about
dynamic typing. Taking the logical not of `true` is easy, but there's nothing
preventing an unruly programmer from writing something like:

```lox
print !nil;
```

For unary negation, we made it an error to pass an operand that isn't a <span
name="negate">number</span>. Lox, like most scripting languages, is more
permissive about using other types in a context like here where a Boolean is
expected. The rule for how other types behave there is called "falsiness", and
we implement it here:

<aside name="negate">

Now I can't help but try to figure out what it would mean to negate other types
of values. `nil` is probably its own negation, sort of like a weird pseudo-zero.
Negating a string could, uh, reverse it?

</aside>

^code is-falsey

Lox follows Ruby in that `nil` and `false` are falsey and every other value
behaves like `true`. We've got a new instruction we can generate, so we also
need to be able to *un*-generate it in the disassembler:

^code disassemble-not (2 before, 1 after)

### Equality and comparison operators

That wasn't too bad. Let's keep the momentum going and knock out the equality
and comparison operators too: `==`, `!=`, `<`, `>`, `<=`, and `>=`. That covers
all of the operators that return Boolean results except the logical operators
`&&` and `||`. Since those need to short-circuit -- basically do a little
control flow -- we aren't ready for them yet.

Here's the new instructions for those operators:

^code comparison-ops (1 before, 1 after)

Wait, only three? What about `!=`, `<=`, and `>=`? We could create instructions
for those too. Honestly, the VM would execute faster if we did, so we *should*
do that if the goal was performance.

But my main goal here is to teach you about bytecode compilers. I want you to
start internalizing the idea that the bytecode instructions doesn't need to
closely follow the user's source code. The VM has total freedom to use whatever
instruction set and code sequences it wants as long as they have the right
user-visible semantics.

The expression `a != b` has the same semantics as `!(a == b)`, so the compiler
is free to compile the latter as if it were the former. Instead of a dedicated
`OP_NOT_EQUAL` instruction, it can output an `OP_EQUAL` followed by an `OP_NOT`.
Likewise, `a <= b` is the same as `!(a > b)` and `a >= b` is `!(a < b)`. Thus,
we only need three new instructions.

Over in the parser, though, we do have six new operators to wire up in the parse
table. We use the same `binary()` parser function from before. Here's the row
for `==`:

^code table-equal (1 before, 1 after)

And the remaining five operators are a little farther down in the table:

^code table-comparisons (1 before, 1 after)

Inside `binary()` we have a switch to generate the right bytecode for each
operator. We add cases for the six new operators:

^code comparison-operators (1 before, 1 after)

The `==`, `<`, and `>` operators output a single instruction. The others output
a pair of instructions, one to evalute the inverse operation, and then an
`OP_NOT` to flip the result. Six operators for the price of three instructions!

That means over in the VM, our job is simpler. Equality is the most general
operation:

^code interpret-equal (1 before, 1 after)

You can evaluate `==` on any pair of objects, even objects of different types.
There's enough complexity that it makes sense to shunt that logic over to a
separate function. That function always returns a C `bool`, so we can safely
wrap it in a `BOOL_VAL`. The function relates to values, so we declare it in the
value module:

^code values-equal-h (2 before, 1 after)

It's implementation looks like thus:

^code values-equal

First we check the types. If the values have <span name="equal">different</span>
types, they are definitely not equal. Otherwise, we unwrap the two values and
compare them directly.

<aside name="equal">

Some languages have "implicit conversions" where values different types may be
considered equal if one can be converted to the others' type. For example, the
number 0 is equivalent to the string "0" in JavaScript. This looseness was a
large enough source of pain that JS added a separate "strict equality" operator,
`===`.

PHP considers the strings "1" and "01" to be equivalent because both can be
converted to equivalent numbers, though the ultimate reason is because PHP was
designed by a Lovecraftian Elder God to destroy the mind.

Most dynamically-typed languages that have separate integer and floating point
number types consider value of different number types equal if the numeric
values are the same, though even that seemingly innocuous convenience can bite
the unwary.

</aside>

For each value type, we have a separate case that handles comparing the value
itself. Given how similar the cases are, you might wonder why we can't simply
`memcmp()` the two Value structs and be done with it. The problem is that for
some types, the union field leaves extra unused bits. C gives no guarantee about
what is in those, so it's possible that two equal Values actually do have a
couple of unused bits that differ.

**todo: illustrate**

You wouldn't believe how long it took me to figure out this fact.

Anyway, as we add more types to clox, this function will grow new cases. For
now, these three are sufficient. The other comparison operators are easier since
they only work on numbers:

^code interpret-comparison (1 before, 1 after)

We already extended the `BINARY_OP` macro to handle operators that return
non-numeric types. Now we get to use that. We pass in `BOOL_VAL` since the
result value type is Boolean. Otherwise, it's no different from plus or minus.

As always, the coda to today's aria is disassembling the new instructions:

^code disassemble-comparison (2 before, 1 after)

With that, our numeric calculator has come something closer to a general
expression evaluator. Fire up clox and type in:

```lox
!(5 - 4 > 3 * 2 == !nil)
```

OK, I'll admit that's maybe not the most useful expression, but we're making
progress. We have one missing built-in type with its own literal form: strings.
Those are much more complex because strings can vary in size. That tiny
difference turns out to have implications so large that we need to give strings
[their very own chapter][strings].

[strings]: strings.html

<div class="challenges">

## Challenges

1. We could reduce our binary operators even further than we did here. Which
   other instructions can you eliminate, and how would the compiler cope with
   their absence?

2. Conversely, we can improve the speed of our bytecode VM by adding more
   instructions that correspond to higher-level operations. That way one turn
   through the instruction-decode-and-dispatch loop accomplishes more work.
   What higher-level instructions would you consider to speed up the kind of
   user code we added support for in this chapter?

3. **todo**

</div>
