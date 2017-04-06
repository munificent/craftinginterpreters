^title Evaluating Expressions
^part A Tree-Walk Interpreter

> You are my creator, but I am your master; Obey!
>
> <cite>Mary Shelley, Frankenstein</cite>

If you're setting the mood before you embark on this chapter, do your best to
summon a thunderstorm, one of those swirling tempests that likes to yank open
shutters right at the climax of the story. A bolt of lightning would come in
handy. In this chapter, our interpreter will take breath, open its eyes, and
execute some code.

<img src="image/evaluating-expressions/lightning.png" alt="A bolt of lightning strikes a Victorian mansion. Spooky!" />

There are all manner of ways that language implementations make a computer do
what the user's source code commands. They can compile it to machine code,
translate it to another high level language, or translate to some bytecode
format for a virtual machine that runs it. For our first interpreter, though, we
are going to take the simplest, shortest path and execute the syntax tree
itself.

Right now, our parser only supports expressions. So, to "execute" code, we will
evaluate the expression and produce a value. For each kind of expression syntax
we know how to parse -- literal, operator, etc. -- we need a corresponding chunk
of code that knows how to evaluate that tree to produce a result. That raises
two questions:

1. What kinds of values are produced?
2. How is the code to execute expressions organized?

Let's tackle them one at a time.

## Representing Values

Deep in the bowels of our interpreter are objects that represent Lox <span
name="value">values</span>. They are created by literals, produced by
expressions, and stored in variables. To the user, they are *Lox* objects. But
to us, the language implementer, they are defined in terms of the implementation
language -- Java in our case.

<aside name="value">

Here, I'm using "value" and "object" pretty much interchangeably.

Later, in the C interpreter, we'll make a slight distinction between them, but
that's mostly to have unique terms for two different corners of the
implementation -- in-place versus heap-allocated data. From the user's
perspective, the terms are synonymous.

</aside>

So how do we want to implement Lox values? Aside from the basic "pass around and
store in variables", we need to do a few things with one:

*   **Determine its type.** Since Lox is dynamically typed, we need to be able
    to check the type of a value at runtime to make sure you don't do things
    like subtract a string from a number.

*   **Tell if the object is truthy or not.** When the object is used in
    something like an `if` condition, we need to tell if that means the
    condition succeeds or not.

*   **Tell if two objects are equal.** We support `==` and `!=` on all kinds of
    objects, so we need to be able to implement that.

If we can determine an object's type, then we can implement truthiness and
equality as methods that check the type and do the right thing for each kind of
value. So, really, asking an object its type is the only fundamental operation.

Fortunately, the <span name="jvm">JVM</span> gives us that for free. The
`instanceof` operator lets you ask if an object has some type. There are also
boxed types for all of the primitive types we need for Lox. Here's how we map each Lox type to Java:

<aside name="jvm">

Built in instance checks and a really nice garbage collector are the main
reasons we're writing our first interpreter in Java. Those get us up and running
more quickly.

</aside>

<table>
<thead>
<tr>
  <td>Lox type</td>
  <td>Java representation</td>
</tr>
</thead>
<tbody>
<tr>
  <td><code>nil</code></td>
  <td><code>null</code></td>
</tr>
<tr>
  <td>Boolean</td>
  <td>Boolean</td>
</tr>
<tr>
  <td>number</td>
  <td>Double</td>
</tr>
<tr>
  <td>string</td>
  <td>String</td>
</tr>
</tbody>
</table>

We'll have to do a little more work later when we add Lox's notions of
functions, classes, and instances, but these are fine for the basic types. We
don't need any kind of wrapper classes or indirection. We won't be, say,
defining a LoxNumber class. Plain old Double works fine. When we want to pass a
Lox object around, we can type it as Object, since that's a superclass of all of
the above types.

## Evaluating Expressions

Next, we need blobs of code to implement the evaluation logic for each kind of
expression we can parse. We could stuff those into the syntax tree classes
directly in something like an `interpret()` method. In effect, we could tell
each syntax tree node, "Interpret thyself." This is the Gang of Four's
[Interpreter design pattern][]". It's a neat pattern, but like I mentioned
earlier, it gets messy if we jam all sorts of logic into the tree classes.

[interpreter design pattern]: https://en.wikipedia.org/wiki/Interpreter_pattern

Instead, we're going to break out our groovy [Visitor pattern][]. In the
previous chapter, we created an AstPrinter class. It took in a syntax tree and
recursively traversed it, building up a string which it ultimately returned.
That's almost exactly what a real interpreter does, except instead of
concatenating strings, it produces values.

[visitor pattern]: representing-code.html#the-visitor-pattern

We start with a new class:

^code interpreter-class

It declares that it's a visitor. The return type of the visit methods is Object,
the root class that we use to refer to a Lox value in our Java code. To satisfy
the Visitor interface, we need to define visit methods for each of the four
expression tree classes we've defined. We'll start with the simplest, literals.

### Evaluating literals

The leaves of an expression tree -- the atomic bits of syntax that all other
expressions are composed of -- are <span name="leaf">literals</span>. Literals
are almost values already, but the distinction is important. A literal is a *bit
of syntax* that produces a value. A literal, for example, always has a token
that identifiers where it occurs in the user's source code. Lots of values are
produced by computation and don't appear anywhere in the code itself. Those
aren't literals. A literal comes from the parser's domain. Values are an
interpreter concept, part of the runtime's world.

<aside name="leaf">

In the next chapter, when we implement variables, we'll add identifiers, which
are also leaf nodes.

</aside>

So, much like we converted a literal *token* into a literal *syntax tree node*
in the parser, now we convert the literal tree node into a runtime value. That
turns out to be trivial:

^code visit-literal

We eagerly produced the runtime value way back during scanning and stuffed it in
the token. The parser took that value and stuffed it in the literal tree node,
so to evaluate a literal, we simply pull it back out.

### Evaluating parentheses

The next simplest node to evaluate is a grouping expression -- the node you get
as a result of using explicit parentheses to override precedence in an
expression.

^code visit-grouping

A <span name="grouping">grouping</span> node has a reference to an inner node
for the expression contained inside the parentheses. To evaluate the
parenthesized expression itself, we recursively evaluate that subexpression and
return it.

<aside name="grouping">

Some parsers don't define tree nodes for parentheses. Instead, when parsing a
parenthesized expression, they simply return the node for the inner expression.
We do create a node for parentheses in Lox because we'll need it later when we
get to handling the left-hand sides of assignment expressions.

</aside>

We rely on this helper, which simply sends the expression back into the
interpreter's visitor implementation:

^code evaluate

### Evaluating unary expressions

Like grouping, unary expressions have a single subexpression that we must
evaluate first. The only difference is that the unary expression itself does a
little work afterwards:

^code visit-unary

First, we evaluate the operand expression. Then we apply the unary operator
itself to the result of that. There are two different unary expressions,
identified by the type of the operator token. Here's `-`, which negates the
result of the subexpression.

The subexpression must be a number. Since we don't *statically* know that in
Java, we <span name="cast">cast</span> it before performing the operation. This
type cast is happening at runtime when the `-` is evaluated. That's the core of
dynamic typing right there before your eyes.

<aside name="cast">

You're probably wondering what happens if the cast fails. Fear not, we'll get
into that soon.

</aside>

Now you can start to see how evaluation recursively traverses the tree. Note how
we can't evaluate the unary operator until after we've evaluated its operand
subexpression. That means our interpreter is doing a **post-order traversal** --
each node evaluates its children before doing its own work.

The other operator is `!`:

^code unary-bang (1 before, 1 after)

Unlike `-`, it allows operands of any type. It uses Lox's rules for truthiness
to determine what "true" means for the different types, and then complements
that. We implement the truthiness rules here:

^code is-true

Pretty simple: `nil` and `false` are false, everything else is true.

### Evaluating binary operators

We're onto the last expression tree class, binary operators. Like the name
implies, they work like unary ones but with another operand. Let's start with
the arithmetic ones:

^code visit-binary

I think you can figure out what's going on here. I left out one arithmetic
operator because it's a little special:

^code binary-plus (3 before, 1 after)

The `+` operator can also be used to concatenate two strings. To handle that, we
don't just assume the operands are a certain type and *cast* them, we
dynamically *check* the type and choose the appropriate operation. This is why
we need our object representation to support `instanceof`.

<aside name="plus">

We could have defined an operator specifically for string concatenation. That's
what Perl (`.`), Lua (`..`), Smalltalk (`,`), Haskell (`++`) and others do.

I thought it would make Lox a little more approachable to use the same syntax as
Java, JavaScript, Python, and others. This means that the `+` operator is
**overloaded** to support both adding numbers and concatenating strings. Even in
languages that don't use `+` for strings, it is often overloaded to support
adding both integers and floating point numbers.

</aside>

Next up are the comparison operators:

^code binary-comparison (1 before, 1 after)

They are basically the same as arithmetic. The only difference is that where the
arithmetic operators produce a value whose type is the same as the operands
(numbers or strings), the comparison operators always produce a Boolean.

The last pair of operators are equality:

^code binary-equality

Unlike the comparison operators which require numbers, the equality operators
support operands of any type, even mixed ones. You can't ask Lox if 3 is *less*
than `"four"`, but you can ask if it's <span name="equal">*equal*</span> to
it.

<aside name="equal">

Spoiler alert: it's not.

</aside>

Like truthiness, the equality logic is hoisted out into a separate method:

^code is-equal

This is one of those corners where the details of how we represent Lox objects
in terms of Java matters. We need to correctly implement *Lox's* notion of
equality, which may be different from Java's.

Fortunately, the two are pretty similar. We have to handle `nil`/`null`
specially so that we don't throw a NullPointerException if we try to call
`equals()` on `null`. Otherwise, we're fine. `.equals()` on Boolean, Double, and
String have the behavior we want for Lox.

And that's it! That's all the code we need to correctly interpret a valid Lox
expression. But what about an *invalid* one? In particular, what happens when a
subexpression evaluates to an object of the wrong type for the operation being
performed?

## Runtime Errors

I was cavalier about jamming casts into the previous code to take the Object
that a subexpression produces and use it as the number or string some operators
require. Those casts can fail. Even though the code is erroneous, if we want to
make a <span name="fail">usable</span> language, we are responsible to handling
that error gracefully.

<aside name="fail">

We could simply not detect or report a type error at all. This is what C does if
you cast a pointer to some type that doesn't match the data that is actually
being pointed to. C gets gains flexibility and speed by allowing that, but is
also famously dangerous. Once you misinterpret bits in memory, all bets are off.

Few modern languages accept unsafe operations like this. Instead, most are
**memory safe** and ensure -- through a combination of static and runtime checks
-- that a program can never incorrectly interpret the value stored in a piece of
memory.

</aside>

It's time for us to talk about **runtime errors.** I spilled a lot of ink in the
previous chapters talking about error handling, but those were all *syntax* or
*static* errors. Those are detected and reported before *any* code is executed.
Runtime errors are failures that the language semantics demand we detect and
report while the program is running (hence the name).

Right now, the Java cast will fail and the JVM will throw a ClassCastException.
That unwinds the whole stack and exits the application, vomiting a Java stack
trace onto the user. That's probably not what we want. The fact that Lox is
implemented in Java should be a detail hidden from the user. Instead, we want
them to understand that a *Lox* runtime error occurred, and give them an error
message relevant to our language and their program.

The Java behavior does have one thing going for it, though -- it correctly stops
executing any code when the error occurs. Let's say the user enters some
expression like:

    :::lox
    2 * (3 / -"muffin")

You can't negate a <span name="muffin">muffin</span>, so we need to report a
runtime error at that inner `-` expression. That in turn means we can't evaluate
the `/` expression since it has no meaningful right operand. Likewise the `*`.
So when a runtime error occurs deep in some expression, we need to escape all
the way out.

<aside name="muffin">

I don't know, man, *can* you negate a muffin?

![A muffin, negated.](image/evaluating-expressions/muffin.png)

</aside>

We could print a runtime error and then abort the process and exit the
application entirely. That has a certain melodramatic flair. Sort of the
programming language interpreter equivalent of a mic drop.

Tempting as that is, we should probably do something a little less
confrontational. While a runtime error needs to stop evaluating the expression,
it shouldn't kill the *interpreter*. If a user is running the REPL and has a
typo in a line of code, they should still be able to keep going and enter more
code after that.

### Detecting runtime errors

Our tree-walk interpreter evaluates nested expressions using recursive method
calls, and we need to unwind out of all of those. Throwing an exception in Java
is a fine way to accomplish that. However, instead of using Java's own cast
failure, we'll define a Lox-specific one so that we can handle it how we want.

Before we do the cast, we validate the type ourselves. So, for unary `-`, we
add:

^code check-unary-operand (1 before, 1 after)

The code to check the operand is:

^code check-operand

When the operand isn't a number, it throws one of these:

^code runtime-error-class

Unlike the Java cast exception, our <span name="class">class</span> tracks the
token that identifies where in the user's code the runtime error came from. Like
with static errors, this helps the user know where to fix their code.

<aside name="class">

I admit the name "RuntimeError" is confusing since Java defines a
RuntimeException class. An annoying thing about implementing languages is you
often want names that collide with ones already taken by the implementation
language. Wait until we get to implementing Lox classes.

</aside>

We add similar checking to the binary operators. In the interest of
completeness, I'll run through them all.

Greater than:

^code check-greater-operand (1 before, 1 after)

Greater than or equal to:

^code check-greater-equal-operand (1 before, 1 after)

Less than:

^code check-less-operand (1 before, 1 after)

Less than or equal to:

^code check-less-equal-operand (1 before, 1 after)

Subtraction:

^code check-minus-operand (1 before, 1 after)

Division:

^code check-slash-operand (1 before, 1 after)

Multiplication:

^code check-star-operand (1 before, 1 after)

All of those rely on this validator, which is virtually the same as the unary
one:

^code check-operands

<aside name="operand">

A better error message would tell the user *which* operand wasn't a number, but
I'm trying to keep the code terse.

</aside>

The odd one out, again, is addition. Since it's overloaded for numbers and
strings, it already has code to check the types. All we need to do is fail when
neither of the two success cases are matched:

^code string-wrong-type (3 before, 1 after)

OK, now we detect runtime errors deep in the bowels of the evaluator. The errors
are getting thrown. The next step is to write the code that catches them. For
that, we need to wire up the Interpreter class into the main Lox class that
drives it.

## Hooking Up the Interpreter

The visit methods are sort of the guts of the Interpreter class, where the real
work happens. We need to wrap a skin around it so that it can interface with the
rest of the program. Its public API is simply:

^code interpret

It takes in a syntax tree for an expression and evaluates it. If that succeeds,
`evaluate()` returns an object for the result value. `interpret()` converts that
to a string and shows it to the user. To convert any Lox value to a string, we
rely on:

^code stringify

This is another of those pieces of code like `isTrue()` that crosses the
membrane between the user's view of Lox objects and their internal
representation in Java.

It's pretty straightforward. Since Lox was designed to be familiar to someone
coming from Java, things like Booleans look the same in both languages. The two
edge cases are `nil`, which we represent using Java's `null`, and numbers.

Lox uses double-precision numbers even for integer values. In that case, they
should print without a decimal point. Since Java has both floating point and
integer types, it wants you to know which one you're using. It tells you by
adding an explicit `.0` to integer-valued doubles. We don't care about that, so
we <span name="number">hack</span> it off the end.

<aside name="number">

We take care of this edge case with numbers to ensure that jlox and clox behave
the same. Ensuring compatibility in all of the weird corners of the language
like this is an annoying but vital part of the job. Users rely on these
details -- either deliberately or inadvertently -- and if the implementations aren't consistent, their program will break when they
run it on different interpreters.

</aside>

### Reporting runtime errors

If a runtime error is thrown while evaluating the expression, `interpret()`
catches it. This lets us report the error to the user and then gracefully
continue. All of our existing error reporting code lives in the Lox class, so we
put this method there too:

^code runtime-error-method

It uses the token associated with the RuntimeError to tell the user what line of
code was executing when the error occurred. It would be even better to give the
user an entire callstack to show how they *got* to be executing that code. But
we don't have function calls yet, so I guess we don't have to worry about it.

After showing the error, it sets this field:

^code had-runtime-error-field (1 before, 1 after)

It's similar to the `hasError` field we defined earlier to tracking when a
syntax error was detected. We use that field to ensure we don't execute any code
if there's a syntax error. It's too late for that now, so what is this second
field for?

^code check-runtime-error (4 before, 1 after)

It has one minor use. If the user is running a Lox <span name="repl">script from
a file</span> and a runtime error occurs, we set an exit code when the process
quits to let them know. Not everyone cares about command line program shell
conventions, but we do.

<aside name="repl">

If the user is running the REPL, we don't care about tracking runtime errors.
After they are reported, we simply loop around and let them input new code and
keep going.

</aside>

### Running the interpreter

Now that we have an interpreter, the Lox class can start using it. It creates
and stores an instance of it:

^code interpreter-instance (1 before, 1 after)

We make it static so that successive calls to `run()` inside a REPL session
reuse the same interpreter. That doesn't make a difference now, but it will
later when the interpreter stores state like global variables. That state needs
to persist throughout the entire REPL session.

Finally, we remove the temporary code from the [last chapter][] that printed the
syntax tree and replace it with this:

[last chapter]: parsing-expressions.html

^code interpreter-interpret (3 before, 1 after)

We have an entire language pipeline now: scanning, parsing, and
execution. Congratulations, you now have your very own arithmetic calculator.

As you can see, the interpreter is pretty simple. But the Interpreter class and
the visitor pattern we've set up form the skeleton that later chapters will
stuff full of interesting guts -- variables, functions, etc. Right now, the
interpreter doesn't do very much, but it's alive!

<span name="useless"></span>

<img src="image/evaluating-expressions/skeleton.png" alt="A skeleton waving hello." />

<aside name="useless">

Did you notice that none of the illustrations in this chapter are even the
slightest bit useful?

</aside>

<div class="challenges">

## Challenges

1.  Allowing comparison operators on types other than numbers could be useful.
    The syntax is shorter than named function calls and might have a reasonable
    interpretation for some types like strings. Even comparisons among mixed
    types, like `3 < "pancake"` could be handy to enable things like
    heterogenous ordered collections. Or it could lead to bugs and confused
    users.

    Would you extend Lox to support comparing other types? If so, which pairs of
    types do you allow and how do you define their ordering? Justify your
    choices and compare them to other languages.

2.  Many languages define `+` such that if *either* operand is a string, the
    other is converted to a string and the results are then concatenated. For
    example, `"scone" + 4` would yield `scone4`. Extend the code in
    `visitBinary()` to support that.

3.  What happens right now if you divide a number by zero? What do you think
    should happen? Justify your choice. How do other languages you know handle
    division by zero and why do they make the choices they do?

    Change the implementation in `visitBinary()` to detect and report a runtime
    error for this case.

</div>

<div class="design-note">

## Design Note: Static and Dynamic Typing

Some languages, like Java, are statically typed which means type errors are
detected and reported at compile time before any code is run. Others, like Lox,
are dynamically typed and defer checking for type errors until runtime right
before an operation is attempted. We tend to think of these two styles as
entirely disjoint, but there is actually a continuum between them.

It turns out that even most statically typed languages defer *some* type checks
until runtime. The type system checks most type rules statically, but inserts
runtime checks in the generated code for other operations.

For example, in Java, the *static* type system assumes a cast expression will
always safely succeed. After you cast some value, you can statically treat it as
the destination type and not get any compile errors.

But downcasts can fail, obviously. The only reason the static checker can
presume that casts always succeed without violating the language's soundness
guarantees is because the cast is checked *at runtime* and throws an exception
on failure.

A more subtle example is [covariant arrays][] in Java and C#. The static
subtyping rules for arrays allow operations that are not sound. Consider:

[covariant arrays]: https://en.wikipedia.org/wiki/Covariance_and_contravariance_(computer_science)#Covariant_arrays_in_Java_and_C.23

```java
Object[] stuff = new Integer[1];
stuff[0] = "not an int!";
```

This code compiles without any errors. The first line upcasts the Integer array
and stores it in a variable of type Object array. The second line stores a
string in one of its cells. The Object array type statically allows that
-- strings *are* Objects -- but the actual Integer array that `stuff` refers to
at runtime should never have a string in it!

To avoid that catastrophe, when you store a value in an array, the JVM does a
*runtime* check to make sure it's an allowed type. If not, it throws an
ArrayStoreException.

Java could have avoided the need to check this at runtime by disallowing the
cast on the first line. It could make arrays *invariant* such that an array of
Integers is *not* an array of Objects. That's statically sound, but it prohibits
common and safe patterns of code that only read from arrays. Covariants is fine
if you never *write* to the array. Those patterns were particularly important
for usability in Java 1.0 before it supported generics.

Gosling and the other Java designers traded off a little static safety and
performance (those checks in array stores take time) in return for some
flexibility.

There are few modern statically typed languages that don't do *any* of their
type validation at runtime. If you find yourself designing a statically typed
language, keep in mind that you can sometimes give users more flexibility
without sacrificing too many of the benefits of static safety by deferring some
type checks until runtime.

On the other hand, a key reason users choose statically typed languages is
because of the confidence the language gives them that certain kinds of errors
can *never* occur when their program is run. Defer too many type checks until
runtime, and you risk eroding that confidence.

</div>
