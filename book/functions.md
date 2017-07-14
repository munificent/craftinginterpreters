^title Functions
^part A Tree-Walk Interpreter

> And that is also the way the human mind works -- by the compounding of old ideas into new structures that become new ideas that can themselves be used in compounds, and round and round endlessly, growing ever more remote from the basic earthbound imagery that is each language's soil.
>
> <cite>Douglas R. Hofstadter</cite>

**todo: be more funnier**

We aren't done yet, but this chapter is a culmination of a lot of hard work. The
previous chapters each add useful functionality in their own right, but each
also supplies a piece of the puzzle that we need to assemble in order to support
functions in Lox. We finally have enough of those pieces ready -- expressions,
statements, variables, control flow, and lexical scope -- that we can make the
jump and add real functions to our little language.

## Function Calls

We've got a chicken and egg problem. If we add function call expressions first,
we don't have any functions to call using them. If we add function declarations,
we can't call them. As you'll see, we can take a shortcut for defining
functions, so we'll start with calls.

You're already familiar with C-style function call syntax, but the grammar is a
little more subtle than you may realize. Calls are typically to named functions
like:

```lox
average(1, 2);
```

But the name of the function being called isn't actually part of the call
syntax. The thing being called -- the **callee** -- can be any expression that
evaluates to a function. (Well, it does have to be a pretty high precedence
expression, but some judicious parentheses take care of that.) For example:

```
getCallback()();
```

Here, we call `getCallback()`, which returns a function. Then we call *that*
using the second pair of parentheses. So it is the parentheses *after* the
callee that indicate a function call. You can think of it sort of like a postfix
operator that starts with `(`.

This "operator" has higher precedence than any other operator, even the unary
ones. So we slot it into the grammar by having the `unary` rule bubble up to a
new `call` rule:

```lox
unary     = ( "!" | "-" ) unary | call ;
call      = primary ( "(" arguments? ")" )* ;
```

This rule matches a primary expression followed by zero or <span
name="curry">more</span> function calls. If there are no parentheses, this
parses a bare primary expression. Otherwise, each call is a pair of parentheses
with an optional list of arguments inside.

<aside name="curry">

This rule can match a series of function calls like `fn(1)(2)(3)`. Code like
this isn't common in C-style languages, but it is in the family of languages
derived from ML. There, the normal way of defining a function that takes
multiple arguments is as a series of nested functions. Each function takes one
argument and returns a new function. That function consumes the next argument,
returns yet another function, and so on. Eventually, once all of the arguments
are consumed, the last function completes the operation.

This style, called **currying**, after Haskell Curry (the same guy whose first
name graces a well-known functional language), is baked directly into the
language syntax so it's not as cumbersome as it sounds.

</aside>

The argument list grammar is:

```lox
arguments = expression ( "," expression )* ;
```

This rule requires at least one expression, followed by zero or more other
expressions, each preceded by a comma. The `call` rule itself treats the
`
` production as optional, which is how zero-argument calls are
allowed.

I admit, this seems a little more metasyntactically awkward than you'd expect
for "zero or more comma-separated things". There are some refined versions of
our BNF notation that handle this better, but in EBNF and many language specs
I've seen, it is kind of cumbersome.

Over in our syntax tree generator, this will become a new node:

^code call-expr

It has an expression for the callee, then a list of expressions for the
arguments. It also stores the token for the closing parenthesis. We'll use that
for its location if we need to report a runtime error caused by a function call.

Crack open the parser. Where `unary()` used to go straight to `primary()`,
change it to call, uh `call()`:

^code unary-call (3 before, 1 after)

That looks like:

^code call

It doesn't quite line up with the grammar rules. I moved a few things around to
keep the code cleaner -- one of the luxuries we have with a hand-written parser.
But it's roughly similar to how we parse infix operators. First, it parses a
primary expression, the "left operand" to the call. Then, each time it sees a
`(`, it calls `finishCall()` to parse the call expression. The result of that
becomes the new `expr` and it loops to see if the result of that is itself
called.

The `while (true)` and the explicit `break` is a little funny looking. It would
be simpler as `while (match(LEFT_PAREN))`. I wrote it this way because later
we'll add some more code before that `else` when we add support for properties
on objects.

The code to handle the argument list is in this helper:

^code finish-call

This is more or less the `arguments` grammar rule translated to code, except
that it also handles the zero-argument case. It does that first, by seeing if
the next token is `)`. (We've already consumed the `(` when we call
`finishCall()`.) If it is, it doesn't try to parse any arguments.

Otherwise, it parses an expression, then looks for a comma indicating that there
is another argument after that. It keeps doing that as long as it finds commas.
When it no longer finds a comma, then the argument list must be done and it
consumes the expected closing parenthesis. Finally, it wraps the callee and its
arguments up into a call expression.

Right now, the loop where we parse arguments has no bound. If you want to call a
function and pass a million arguments to it, the parser would have no problem
with it. Do we want to limit that? Other languages have different approaches.
The C standard says a comforming implementation has to support *at least* 127
arguments to a function, but doesn't say there's any upper limit. The Java
specification says a method can accept no more than <span name="254">255</span>
arguments.

<aside name="254">

The limit is 25*4* arguments if the method is an instance method. That's because
`this` -- the receiver of the method -- works like an argument that is
implicitly passed to the method, so it claims one of the slots.

</aside>

Our Java interpreter for Lox doesn't really need a limit, but having a maximum
number of arguments will simplify our bytecode interpreter in part 3. It ensures
we can encode the number of arguments to a call in a fixed number of bits in the
bytecode. We want our two interpreters to be compatible with each other, even in
weird corner cases like this, so we'll add the same limit to jlox.

^code check-max-arity (1 before, 1 after)

I know, *eight* seems gratuitously low. I picked that mainly to minimize some
boring copy/paste code in the C interpreter. You could go up to probably 32 or
so without any problems if you feel like it. It's your language.

Note that this reports an error if it encounters too many arguments, but it
doesn't *throw* the error. Throwing it is how we kick into panic mode which is
what we want if the parser is in a confused state and doesn't know when the next
tokens are for. But here, the parser is still in a perfectly valid state -- it
just found too many arguments. So it reports the error and keeps on keepin' on.

### Interpreting function calls

We don't have any functions we can call, so it seems weird to start implementing
this, but we'll worry about that when we get there. First, our interpreter needs
a new import:

^code import-array-list (1 after)

As always, interpretation starts with a new visit method for our new call
expression node:

^code visit-call

First, it evaluates the expression for the callee. Typically, this expression is
just an identifier that looks up the function by its name, but it could be any
kind of expression. Then it evaluates each of the argument expressions <span
name="in-order">in order</span> and stores the resulting values in a list.

<aside name="in-order">

The "in order" bit is another one of those subtle semantic choices. Evaluating
the callee and any of the argument expressions may have side effects, so the
order that they are performed in may be user visible. Even so, some languages
like Scheme and C don't officially pin down the order the arguments are
evaluated. This gives compilers freedom to reorder them for efficiency, but
means users may be unpleasantly surprised if arguments aren't evaluated in the
order they expect.

</aside>

Once we've got the callee and the arguments ready, all that remains is to
perform the call. We do that by casting the callee to a Callable and then
invoking a `call()` method on it.

This is not the Java standard library's <span name="callable">Callable</span>
interface. Instead, it's our own interface. The Java representation of any Lox
object that can be called like a function will implement this interface. That's
user-defined functions, naturally, but also includes class objects since classes
can be "called" to construct an instance of the class. We'll also use it for one
more purpose shortly.

<aside name="callable">

Alas, all the good names are already taken, so I had to reuse one.

</aside>

There isn't too much to it:

^code callable

It takes in the interpreter so that the class implementing the call can access
state on it if it needs to. It is also passed in the list of evaluated argument
values. The value returned by `call()` is the result that the call expression
produces.

Before we get to actually implementing this interface, we need to make the visit
method a little more robust. I ignored a couple of failure modes that we can't
pretend won't occur. First, what happens if the callee isn't actually something
you can call? What if you do:

```lox
"totally not a function"()
```

The runtime representation of a Lox string is a Java string, so when we cast
that to Callable, the JVM will throw a ClassCastException. We don't want our
interpreter to vomit out some nasty Java stack trace and die. Because Lox isn't
statically-typed, we can't rely on the type checker to prevent errors like this
from happening. Instead, we need to check the type dynamically ourselves:

^code check-is-callable (2 before, 1 after)

We still throw an exception, but now we're throwing our own runtime exception
type, one that the interpreter knows how to catch and report gracefully.

### Checking arity

The problem relates to the function's **arity**. Arity is the fancy term for the
number of arguments a function or operation expects. Unary operators have arity
one, binary operators two, etc. With functions, the arity is based on the number
of parameters in its declaration:

```lox
fun add(a, b, c) {
  print a + b + c;
}
```

This function defines three parameters, `a`, `b`, and `c`, so it's arity is
three and it expects three arguments. What if you try to call it like:

```lox
add(1, 2, 3, 4); // Too many.
add(1, 2);       // Too few.
```

Different languages take different approaches to this problem. Of course, most
statically-typed languages check this at compile time and refuse to compile the
code if the argument count doesn't match the function's arity. JavaScript
discards any extra arguments you pass. If you don't pass enough, it fills in the
missing parameters with the magic sort-of-like-null-but-not really value
`undefined`. Python raises a runtime error if the argument list is too short or
too long.

I think the latter is a better approach. Passing the wrong number of arguments
is almost always a bug, and I find it to be a common mistake in practice. Given
that, the sooner the implementation draws your attention it, the better. So for
Lox, we'll take Python's approach.

Before calling the callable, we check to see if the argument list's length
matches the callable's arity:

^code check-arity (2 before, 1 after)

That requires a new method on the Callable interface to query its arity:

^code callable-arity (1 before, 1 after)

We could push the arity checking into the concrete implementation of `call()`.
But, since we'll have multiple classes implementing Callable, that would end up
with redundant arity checking spread across a few classes. Hoisting it up into
the visit method lets us do it in one place.

## Native Functions

We can theoretically call functions now, but we have no functions to call. This
is a good time to introduce a vital but often ignored facet of language
implementations -- **native functions**. These are functions that the
interpreter exposes to user code but that are implemented in the host language
(in our case Java), not the language being implemented (Lox).

Sometimes these functions are called **primitives**, **external functions**, or
<span name="native">**foreign functions**</span>. Since these functions can be
called while the user's program is running, they form part of the
implementation's runtime. A lot of programming language books ignore these,
because they aren't very *technically* interesting. They're mostly grunt work.

<aside name="native">

It's curious that two of the names for these functions -- "natives" and
"foreign" -- are antonyms. I think it depends on the perspective of the person
choosing the term. If you think of your "home territory" as within the
language's implementation (in our case, Java) then functions written in Java are
"native" to you. But if you have the mindset of a *user* of your language, then
functions in that language are the neighborly ones, and functions in the
interpreter's host language are "foreign".

Part of it too is that "native" is often used to refer to the machine code
language of the underlying hardware. In Java, "native" methods are ones
implemented in C or C++ and compiled to native machine code.

</aside>

But when it comes to making your language actually useful for doing useful
stuff, the native functions your implementation provides are <span
name="print">vital</span>. They provide access to the fundamental services that
all programs are defined in terms of. If you don't provide native functions to
access the file system, a user's going to have a hell of a time writing a
program that reads and displays a file.

<aside name="print">

A classic native function almost every language provides is one to print text to
stdout. We could have made our print statement a native function instead, but
then the last several chapters would have been a slog with no way to tell if our
code is doing anything useful.

</aside>

Many languages also allow users to provide their own native functions. The
mechanism for doing so is called a **foreign function interface** (FFI),
**native extension**, **native interface**, or something along those lines.
These are nice because they free the language implementer from providing access
to every single capability the underlying platform supports.

We won't define an FFI for jlox, but we will add one native function to give you
an idea of what it looks like. When we get to part three and start working on a
much more efficient implementation of Lox, we're going to care a lot about
performance. Performance work requires measurement and that in turn means
**benchmarks**. These are programs that measure the time it takes to exercise
some corner of the language.

We could measure benchmarks by taking the total time it takes to start up the
interpreter, run the benchmark, and exit, but that adds a lot of overhead -- JVM
startup time, OS shenanigans, etc. That stuff does matter, of course, but if
you're just trying to tell if your optimization to, say, arithmetic made a
difference, that stuff is all noise.

A nicer solution is have the benchmark itself measure the time elapsed between
two points in the program. To do that, a Lox program needs to be able to tell
time. There's no way to do that now -- you can't implement a useful clock "from
scratch" without access to the underlying clock on the computer.

So we'll add `clock()` a native function that returns the number of seconds
(including fractional seconds) that the interpreter has been running. The
difference between two calls to this will tell you how much time elapsed.

When we create a new interpreter, we automatically define this function and
register it in the global scope:

^code interpreter-constructor

It defines a <span name="lisp-1">variable</span> named "clock". The value of the
variable is a Java anonymous class that implements Callable. The `clock()`
function takes no arguments, so its arity is zero. The implementation of
`call()` calls the corresponding Java function and converts the result to a
double value in seconds. (And, naturally, that Java function presumably has an
implementation in native code that calls the OS's `clock()` function.)

<aside name="lisp-1">

In Lox, functions and variables occupy the same namespace. That means:

```lox
fun add(a, b) {
  print a + b;
}

add = "not a fn any more";
add(1, 2); // Error!
```

This isn't true of all languages. In Common Lisp, functions and variables live
in their own worlds and you can have functions and variables with the same name
without collision. The way the name is used makes it clear what you are
referring to, though it requires jumping through some hoops when you do want to
use a function as a first-class value.

A classic paper by Richard P. Gabriel and Kent Pitman exploring this aspect of
languages used the terms "Lisp-1" to refer to languages like Scheme and Lox that
put functions and variables in the same namespace and "Lisp-2" for languages
like Common Lisp that partitions them. Despite being totally opaque, those names
have since stuck.

</aside>

So now you can write a Lox program that calls a function, but only that one
since function. And you can't even pass any arguments to it! That's no fun. It's
time to let users define functions too.

## Function Declarations

We finally get to add a new production to the `declaration` rule we introduced
back when we added variables. <span name="name">Function declarations</span>,
like variables, bind a new name. That means they are only allowed in places
where a declaration is allowed.

<aside name="name">

Functions are first class values in Lox, which means they can be stored in
variables. So, really, a named function declaration isn't a single primitive
operation. It's syntactic sugar for two distinct steps:

1. Creating a new function object.
2. Declaring a variable and initializing it with the function.

If Lox had syntax for anonymous functions, it wouldn't need named function
declaration syntax. You could just do:

```lox
var add = fun (a, b) {
  print a + b;
};
```

But since named functions are the common case, I went ahead and gave Lox nice
syntax for those.

</aside>

```lox
declaration = funDecl
            | varDecl
            | statement ;
```

It references this new rule:

```lox
funDecl     = "fun" function ;
function    = IDENTIFIER "(" parameters? ")" block ;
```

The main `funDecl` rule uses a separate helper rule `function`. A function
*declaration* is the `fun` keyword followed by the function declaration itself.
When we get to classes, we'll reuse that same `function` rule for declaring
methods. Those look similar to function declarations, but aren't preceded by
<span name="fun">`fun`</span>.

<aside name="fun">

Methods are too classy to have fun.

</aside>

The function itself is a name followed by the parenthesized parameter list and
the body. The body is always a braced block, using the same grammar rule as we
do for a block statement. The parameter list uses this rule:

```lox
parameters  = IDENTIFIER ( "," IDENTIFIER )* ;
```

It's similar to the earlier `arguments` rule, except this time each parameter is
a single identifier, not an expression.

That's a lot of new syntax for the parser to chew through, but the resulting
syntax tree node isn't too bad:

^code function-ast (1 before, 1 after)

It has a name, a list of parameters -- their names -- and then the body. We
store the body as the list of statements contained inside the curly braces.

Over in the parser, we kick off recognizing the new statement by adding a line
to `declaration()`:

^code match-fun (1 before, 1 after)

Like other statements, a function is recognized by the leading keyword. When we
encounter `fun`, we call:

^code parse-function

This corresponds to the `function` grammar rule since we already matched and
consumed the `fun` keyword. This is a decent-sized blob of code, but we'll work
through it.

1.  First, we consume the expected identifier token for the function's name.

2.  Then we parse the parameter list, surrounded by parentheses. The outer if
    statement handles the zero parameter case, and the inner while loop handles
    parsing parameters as long we find commas to separate them. The result is
    the list of tokens for each parameter's name.

3.  After that, we consume the required `{` indicating the beginning of the
    body. We have to do that <span name="curly">here</span> because `block()`
    assumes that token has already been matched.

<aside name="curly">

Also, it lets us report a more precise error message if the `{` isn't found
since we know it's in the context of a function declaration.

</aside>

You might be wondering about that funny little `kind` parameter. Just like we
reuse the grammar rule, we'll reuse the `function()` method later to parse
methods inside classes. When we do that, we'll pass in "method" for `kind` so
that the error messages are specific to the kind of declaration being parsed.

## Function Objects

We've got some syntax parsed so usually we're ready to interpret, but first we
need to think about how we're going to represent a Lox function in Java. It
needs to keep track of the parameters so that we can bind them to argument
values when the function is called. And, of course, we need to keep around the
code for the body of the function so that we can execute it.

That's basically what the Stmt.Function class itself is. Can we just use that?
Almost, but not quite. We also need a class that implements Callable so that we
can call it. That's a runtime concern, so it would violate my delicate
sensibilities if we slapped that on the syntax node class itself.

Instead, we'll wrap the syntax in a new class:

^code lox-function

Here's how it implements `call()`:

^code function-call

It's only a handful of lines of code, but this is one of the most fundamental,
powerful pieces of our interpreter. As we saw in the chapter on statements and
<span name="env">state</span>, managing name environments is a core part of a
language implementation. Functions are deeply tied to that.

<aside name="env">

We'll dig even deeper into environments in the [next chapter][].

[next chapter]: resolving-and-binding.html

</aside>

When we execute a function, we need to perform a bit of magic with environments.
A key property of function is that it has parameter, and that it *encapsulates*
its parameters -- no other code outside of the function can see its parameters.
Parameters are a key reason why we *have* local scope.

This means each function gets its own environment where it defines its
parameters. Further, this environment must be created dynamically. Each function
*call* gets its own environment. Otherwise, <span
name="fortran">recursion</span> would break. If there are multiple calls to the
same function in play at the same time, each call needs its *own* environment,
even though they are all calls to the same function.

<aside name="fortran">

Early versions of Fortran did not support recursion. This let compilers
statically allocate memory for each function's parameters, since only one set of
them would ever be needed for any given function declaration.

Imagine using `static` for all of your local variables in C, and you have the
idea.

</aside>

This little method implements that magic. At the beginning of the call, it
creates a new environment. Then it walks the parameter and argument lists in
lockstep. For each pair, it creates a new variable with the parameter's name and
binds it to the argument's value.

**todo: illustrate**

Then it tells the interpreter to execute the body of the function in that new
environment. Before that, the current environment is the environment where the
function is being called. We teleport from there inside the new parameter space
we've created for the function.

This is all that's required to pass data into the function. By controlling the
ambient environment that surrounds the body of the function, calls to the same
function with the same code can produce different results.

Once the body of the function has finished executing, `executeBlock()` discards
the environment and restores the previous one. In this case, that's the
environment that was active at the point of our call.

Mechanically, the code is pretty simple. Walk a couple of lists. Bind some new
variables. Call a method. But this is where the crystalline *code* of the
function declaration becomes a leaving, breathing invocation. This is one of my
favorite pieces of code in this entire book. Feel free to take a moment to
meditate on it if you're so inclined.

Done? OK. Note when we bind the parameters, we assume the parameter and argument
lists have the same length. We can do that safely because `visitCallExpr()`
validates that before calling `call()`.  In order to do that, we need to tell it
how many arguments we expect:

^code function-arity

That's most of our object representation. While we're in here, we may as well
implement `toString()`:

^code function-to-string

This will give nicer output if you decide to print a function value:

```lox
fun add(a, b) {
  print a + b;
}

print add; // "add".
```

We'll come back and refine LoxFunction soon, but that's enough to get started
interpreting them. Over in the interpreter, we need to keep track of the global
environment:

^code global-environment (1 before, 1 after)

The `environment` field changes as we enter and exit local scopes. It tracks the
*current* environment. The new `globals` field will always hold a reference to
the outermost global environment.

Now we can visit a function declaration:

^code visit-function

The basic idea is simple, and is similar to how we interpreter other literal
expressions. We take a function *syntax node* -- a compile time representation
of the function -- and convert it to its runtime representation. Here, that's a
LoxObject that wraps the syntax node.

Function declarations are a little different from other literal nodes in that
the declaration *also* binds the resulting object to a new variable. So, after
creating the LoxFunction, we create a new binding in the current environment and
store a reference to it there.

With that, we can define and call our own functions all within Lox. Give it a
try:

```lox
fun sayHi(first, last) {
  print "Hi, " + first + " " + last + "!";
}

sayHi("Dear", "Reader");
```

I don't about you, but that looks like a real programming language to me.

## Return Statements

We can get data into functions by passing parameters, but we've got no way to
get results back <span name="hotel">*out*</span>. Since the body of a function
is a list of statements -- which don't produce values -- the implementation of
`call()` in LoxFunction just executes them and then explicitly returns `nil`.
If Lox was an expression-oriented language like Ruby or Scheme, the body would
be an expression that implicitly evaluated to a value and that would be the
function's result.

<aside name="hotel">

The Hotel California of data.

</aside>

Because we have statements, we need to add dedicated syntax for expressing the
result value. In other words, return statements. I'm sure you can guess the
grammar already:

```lox
statement   = exprStmt
            | forStmt
            | ifStmt
            | printStmt
            | returnStmt
            | whileStmt
            | block ;

returnStmt  = "return" expression? ";" ;
```

We've got one more -- the final, in fact -- production under the venerable
`statement` rule. A return statement itself is the `return` keyword followed by
a return value and terminated with a semicolon. Like most languages, the
return value is optional.

In most statically typed languages, "void" functions that don't return a value
*must* use return without a value expression and non-void functions that do
return a value must provide one in the return statement. Since Lox is
dynamically typed, any function may be called in a position where it is expected
to return a value and it has to return *something*. There are no "void"
functions. Instead, like we already implemented, a function that reaches the end
of its body without hitting a return statement implicitly returns `nil`.
Correspondingly, we'll treat a return statement without a value as equivalent
to:

```lox
return nil;
```

Over in our AST generator, the new node looks like:

^code return-ast (1 before, 1 after)

It keeps the token for the `return` keyword so we can use its location for
error reporting. Then it stores the value being returned, if any.

We parse this like other statements, by recognizing the initial keyword:

^code match-return (1 before, 1 after)

That branches out to:

^code parse-return-statement

First, we hang on to the previously-consumed `return` keyword. Then we see if
there is a value expression or not. Since many different tokens can potentially
start an expression, it's hard to tell if a return value is *present*. Instead,
we check if it's *absent*. Since a semicolon can't occur in an expression, if
the next token is that, we know there must not be a value. In that case, we'll
store `null` in the syntax tree node for the expression.

### Returning from calls

Interpreting a return statement is a little trickier than many other statements.
A return statement can occur anywhere inside the body of a function, even deeply
nested inside other statements. When it's executed, it needs to jump all the way
out of whatever context the interpreter is currently in and cause the function
call to exit, like some kind of jacked up control flow construct.

I don't know about you, but when I hear "jump all the way out" while I'm wearing
my Java hat, I think exceptions. When we execute a return statement, we'll use
an exception to unwind the interpreter past all of the containing statements
back to the code that began executing the body.

The new visit method looks like this:

^code visit-return

If we have a return value, we evaluate it. (Otherwise, we fill in `null` for it,
which is the Java representation of `nil`.) Then we take that value and wrap it
in a custom exception class and throw it. That class is:

^code return-exception

It's a wrapper around the return value, with the accoutrements that Java excepts
for it to be a friendly runtime exception class. The weird super constructor
call with those `null`s and `false`s disables some JVM machinery that we don't
need. Since we're using our exception class for <span name="exception">control
flow</span> and not actual error handling, we don't need things like stach
traces.

<aside name="exception">

For the record, I'm not generally a fan of using exceptions for control flow.
But inside a heavily recursive tree-walk interpreter, it's the way to go. Since
our own syntax tree evaluation is so heavily tied to the Java callstack, we're
pressed to also do some more heavyweight callstack manipulation in some cases,
and exceptions are a powerful tool for that.

</aside>

Back in LoxFunction, where we implement `call()` to begin execution of the
function body, we catch this exception:

^code catch-return (3 before, 1 after)

When we catch a return exception, we pull out the value and make that the return
value from `call()`. If we never catch one of these exceptions, it means the
function reached the end of its body without hitting a return statement. In that
case, we continue to implicitly return `nil`.

Go ahead and give it a try. A fun example that we finally have enough power to
support is a recursive function to calculate Fibonacci numbers:

<span name="slow"></span>

```lox
fun fibonacci(n) {
  if (n <= 1) return n;
  return fibonacci(n - 2) + fibonacci(n - 1);
}

for (var i = 0; i < 20; i = i + 1) {
  print fibonacci(i);
}
```

This tiny program exercises almost every language we support -- expressions,
arithmetic, branching, looping, variables, functions, function calls, parameter
binding, and returns.

<aside name="slow">

You might notice this is pretty slow. Obviously, recursion isn't the most
efficient way to calculate Fibonacci numbers, but as a micro-benchmark, it does
a good job of stress testing how fast our interpreter implements function calls.

As you can see, the answer is "not very fast". That's OK. In our C interpreter,
we'll do better.

</aside>

## Local Functions and Closures

Our functions are pretty full-featured, but there is one loose end to tie up.
In fact, it's a big enough tangle that we'll spend most of the [next chapter][]
unknotting it. But we can get started here.

LoxFunction's implementation of `call()` creates a new environment that it uses
to bind the parameters in. In the code for that, I glossed over one important
point. What is the *parent* of that environment?

In the code we have right now, it always `globals`, the top level global
environment. That way, if an identifier isn't defined inside the function body
itself, the interpreter can look outside the function in the global scope to
find it. In the Fibonacci example, that's how the interpreter is able to look up
the recursive call to `fibonacci` inside the function's own body.

But consider:

```lox
fun outer() {
  var a = "value";

  fun inner() {
    print a;
  }

  inner();
}

outer();
```

In Lox, function declarations are allowed anywhere a name can be bound. That's
at the top level of a Lox script, but also inside a block. That means Lox
supports **local functions** that are defined inside another function, or nested
inside a block. So the above code is *allowed*. The question now is what should
it *do?*

With our current implementation, when the interpreter handles the call to
`inner()`, it creates a new environment for the body of `inner`. The parent of
that environment is the global environment.

**todo illustrate**

The environment chain skips past the environment where `a` is defined, so the
`print a;` is going to fail. Users expect this code to work -- the lexical scope
surrounding the function declaration *should* be available from inside the
function body.

In fact, this means the function may need to hang onto that environment even
after execution of the block where the scope is defined has finished executing.
Here's a classic example:

```lox
fun makeCounter() {
  var i = 0;
  fun count() {
    i = i + 1;
    print i;
  }

  return count;
}

var counter = makeCounter();
counter(); // "1".
counter(); // "2".
```

Here, `count()` uses `i`, which is declared outside of itself in the containing
function `makeCounter()`. `makeCounter()` returns a reference to the `count()`
function and then its own body finishes executing completely.

Meanwhile, the top level code invokes the returned `count()` function. That
executes the body of `count()`, which assigns to and reads `i`, even though the
function where `i` was defined has already exited.

For this to work, `count()` not only needs to access `makeCounter()`'s
environment, it needs to *keep* that reference so that it still has access to it
even the interpreter has discarded it. That means the runtime representation of
a function needs to store the environment in effect at the point where the
function is first declared so that it has access to it later when the function
is called.

This data structure is called a "closure" because it "closes over" and holds
onto the surrounding variables where the function is declared. Closures have
been around since the early Lisp days, and language hackers have come up with
all manner of ways to implement them. For jlox, we'll do the simplest thing that
works. In LoxFunction, we add a field to store the environment:

^code closure-field (1 before)

We need to pass that into the constructor:

^code closure-constructor (1 after)

When we execute a function declaration and create a LoxFunction for it, we pass
in the current environment:

^code visit-closure (1 before, 1 after)

This is the environment that is active when the function is *declared* not when
it's *called*, which is what we want.

Finally, when we call a function, we use that environment:

^code call-closure

The parent environment of the environment we create for the call is now the
environment we stored in the declaration. The LoxFunction grabs the environment
from its declaration and keeps it around as long as needed so we can use it
again when we call the function later.

Try running that `makeCounter()` example now. It works!

We have made Lox dramatically more powerful. Functions let us abstract over,
reuse, and compose code. But, in addition, with closures, we can abstract and
compose <span name="poor">*data*</span>. This may be surprising, but you can use
a closure to represent arbitrary data structures (though the resulting code does
look kind of funny). Since a closure contains an environment -- a Java Map in
our implementation -- it *is* a data structure.

<aside name="poor">

One of my favorite nuggets of programming language lore is [this delightful
koan][koan] from none other than Guy Steele.

[koan]: http://people.csail.mit.edu/gregs/ll1-discuss-archive-html/msg03277.html

</aside>

Of course, we will add more natural support for first-class data structures when
we added classes. But even now, it's possible to define functions that work
something like objects. Cogitate on this:

```lox
fun makePoint(x, y) {
  fun closure(method) {
    if (method == "x") return x;
    if (method == "y") return y;
    print "unknown method " + method;
  }

  return closure;
}

var point = makePoint(2, 3);
print point("x"); // "2".
print point("y"); // "3".
```

We have crossed a real threshold today. Lox is much more powerful than the
rudimentary arithmetic calculator it used to be. Alas, in our rush to cram
closures in, we have let a tiny bit of dynamic scoping leak into the
interpreter. In the next chapter, we will dig more deeply into lexical scope and
plug that hole.

<div class="challenges">

## Challenges

1.  Our interpreter carefully checks that the number of arguments passed to a
    function matches the number of parameters it expects. Since this check is
    done at runtime on every call, it has a real performance cost. Smalltalk
    implementations don't have that problem. Why not?

1.  Lox's function declaration syntax performs two independent operations. It
    creates a function and also binds it to a name. This improves usability for
    the common case where you do want to associate a name with the function.
    But in functional-styled code, you often want to create a function to
    immediately pass it to some other function or return it. In that case, it
    doesn't need a name.

    Languages that encourage a functional style usually support "anonymous
    functions" or "lambdas" -- an expression syntax that creates a function
    without binding it to a name. Add anonymous function syntax to Lox so that
    this works:

        :::lox
        fun thrice(fn) {
          for (var i = 1; i <= 3; i = i + 1) {
            fn(i);
          }
        }

        thrice(fun (a) {
          print a;
        });
        // "1".
        // "2".
        // "3".

    How do you handle the tricky case of an anonymous function expression
    occurring in an expression statement:

        :::lox
        fun () {};

1.  Is this program valid?

        :::lox
        fun scope(a) {
          var a = "local";
        }

    In other words, are a function's parameters in the *same* scope as its local
    variables, or in an outer scope? What does Lox do? What about other
    languages you are familiar with? What do you think a language *should* do?

</div>
