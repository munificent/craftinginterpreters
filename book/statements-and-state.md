^title Statements and State
^part A Tree-Walk Interpreter

> All my life, my heart has yearned for a thing I cannot name.
> <cite>Andre Breton</cite>

The interpreter we have so far feels less like a real programming language than
it does a calculator. Programming to me means building up a system out of
smaller pieces. We can't do that yet because we have no way to create a
**binding** that associates a name with some data or function. Without names,
there is no way to create abstractions, to define pieces that can be combined.

In order to support bindings, our interpreter needs an internal state. When you
define a variable at the beginning of the program and use it at the end, the
interpreter has to hold on to the value of that variable in the meantime. So in
this chapter, we will give our interpreter a brain that can not just think, but
*remember*.

**todo: illustrate brain**

State and <span name="expr">statements</span> go hand in hand. Since statements,
by definition, don't evaluate to a value, they need to do something else to be
useful. That something is called a **"side effect"**. It could mean producing
user-visible output or modifying some state in the interpreter that can be
detected later. The latter makes them a great fit for declaring variables or
other named entities.

<aside name="expr">

You could make a language that treats variable declarations as expressions that
both create a binding and produce a value, but I'm not aware of any widely-used
languages that do. Scheme seems like a contender, but keep in mind that a `let`
expression evaluates to the result of the *body where the variable is used*, not
the declaration itself. Definitions in Scheme produced using `define` are not
expressions.

</aside>

In this chapter, we'll do all of that. We'll add statements to the language, and
define statements that produce output (`print`) and create state (`var`). We'll
add expressions to access and assign to variables. Finally, we'll add blocks and
local variables. That's a lot to stuff into one chapter, but we'll chew through
it all one bite at a time.

## Statements

The first step is extending Lox's grammar with statements. They aren't very
different from expressions. We start with the two simplest kinds:

1.  An **expression statement** lets you evaluate an expression where a
    statement is expected. They exist mainly so that you can call functions that
    have side effects. You may not notice them, but you use them all the time in
    <span name="expr-stmt">C</span>, Java, and other languages. Any time you see
    a function or method call followed by a `;`, you're looking at one.

    <aside name="expr-stmt">

    Pascal is an outlier. It distinguishes between *procedures* and *functions*.
    Functions return values, but procedures cannot. There is a statement form
    for calling a procedure, but functions can only be called where an expression is expected. There are no statement expressions.

    C sort of goes halfway -- you usually can't call a void function in the
    middle of an expression -- but it's happy to let you call a non-void
    function in statement position.

    </aside>

2.  A **print statement** evaluates a single expression and then displays the
    result to the user. I admit it's weird to bake print right into the language
    instead of making it a library function. Doing so is a concession to the
    fact that we're building this interpreter one chapter at a time and
    want to be able to play with it before it's all done.

    To make print a library function, we'd have to wait until we had all of the
    machinery for defining and calling functions in place <span
    name="print">before</span> we could get any user-visible side effects.

    <aside name="print">

    I will note with only a modicum of defensiveness that BASIC and Python
    have dedicated print statements and they are real languages.

    </aside>

New syntax means new grammar rules. Finally, we get to a point where we can
parse an entire Lox script. Since Lox is an imperative, dynamically-typed
language, the "top level" of a script is simply a list of statements. The new
rules are:

```lox
program     = statement* EOF ;

statement   = exprStmt
            | printStmt ;

exprStmt    = expression ";" ;
printStmt   = "print" expression ";" ;
```

The first rule is now `program`, which is the entry point for the grammar and
represents an entire Lox script or REPL entry. A program is a list of statements
followed by the special "end of file" token. The mandatory end token ensures the
parser consumes the entire input and doesn't silently ignore garbage tokens at
the end of a script.

Right now, `statement` only has two cases for the two kinds of statements we've
described. We'll fill in more later in this chapter and in the following ones.
The next step is turning this grammar into something we can store in memory --
syntax trees.

### Statement syntax trees

There are no places in the grammar whether either an expression or a statement
could appear. The operands of, say, `+` are always expressions, never
statements. The body of a while loop is always a statement.

Since the two syntaxes are disjoint, we don't need a single base class that they
all inherit from. Splitting them into separate class hierarchies encodes that in
Java's type system so the compiler can help us find dumb mistakes like trying to
pass a statement to a Java method that expects an expression.

That means a new base class for statements. As our elders did before us, we will
use the cryptic name "Stmt". With great <span name="foresight">foresight</span>,
I have designed our little AST metaprogramming script in anticipation of this.
That's why we passed in "Expr" as a parameter to `defineAst()`. We add another
call to that for defining Stmt and its subclasses:

<aside name="foresight">

Not really foresight: I wrote all the code for the whole book before I started
slicing things into chapters and scrawling prose.

</aside>

^code stmt-ast (2 before, 1 after)

Run that script and behold your new "Stmt.java" file with the syntax tree
classes we need for expression and print statements. Don't forgot to add it to
your IDE project or makefile or whatever.

### Parsing statements

The parser's `parse()` method that parses and returns a single expression was a
temporary hack to get the last chapter up and running. Now that our grammar has
the correct starting rule, `program`, we can replace `parse()` with the real
deal:

^code parse

It parses a series of statements, as many as it can find until it hits the end
of the input. This is a pretty direct translation of the `program` rule into
recursive descent style. We must also chant a minor prayer to the Java Verbosity
Gods since we are using ArrayList now:

^code parser-imports (2 before, 1 after)

A program is a list of statements, and one parse one of those using:

^code parse-statement

It's a little bare bones, but we'll fill it in with more statement types later.
It determines which specific statement rule is matched by looking at the next
token. A `print` token means it's obviously a print statement.

If the next token doesn't look like any known kind of statement, we assume it
must be an expression statement. That's the typical final fallthrough case when
parsing a statement, since it's hard to proactively recognize an expression the
next token.

Each statement kind gets its own method. First print:

^code parse-print-statement

Since we <span name="consume">already</span> matched and consumed the `print`
token itself, it doesn't need to do that here. It parses the subsequent
expression, consumes the terminating semicolon, and emits the syntax tree.

<aside name="consume">

Some parsers I've seen have a naming convention of starting methods with
"finish" or something like that if the method is called after a bit of its
corresponding grammar rule has already been matched. In a giant parser with lots
of methods calling each other, this can be handy to help maintainers keep track
of what state the token stream is in when a specific parse method is called. For
Lox's little parser, I didn't bother.

</aside>

If we didn't match a print statement, we must have one of these:

^code parse-expression-statement

Similar to print, it parses an expression followed by a semicolon. It wraps that
Expr in a Stmt and returns it.

### Executing statements

We're running through the previous couple of chapters in microcosm, working our
way through the front end. Our parser can now produce statement syntax trees, so
the next and final step is to interpret them. As in expressions, we use the
Visitor pattern, but we have a new visitor interface, Stmt.Visitor, to
implement since statements have their own base class.

We add that to the list of interfaces Interpreter implements:

^code interpreter (1 after)

Unlike expressions, statements produce no values, so the return type of the
visit methods is Void, not Object. We have two statement types, and we need a
visit method for each. The easiest is expression statements:

<aside name="void">

The capitalized "Void" might be unfamiliar. Java doesn't let you use the real
lowercase "void" type as a type argument in a generic type for obscure reasons
having to do with type erasure and the JVM stack. Instead, there is a separate
"Void" type specifically for this use. Sort of like a "boxed void", as "Integer"
is to "int". It has only one value, `null`.

Now that I think about it, I don't think I've ever used it for anything but the
return type of visit methods in the Visitor pattern.

</aside>

^code visit-expression-stmt

It evaluates the inner expression using our existing `evaluate()` method and
discards the value (appropriately enough by using a Java expression statement to
call `evaluate()`). Then we return `null`. Java requires that to satisfy the
special capitalized Void return type. Weird, but what can you do?

The print statement's visit method isn't much different:

^code visit-print

Before discarding the expression's value, it prints it to standard out. But
first, it converts it to a string using the `stringify()` method we introduced
in the last chapter.

Our interpreter is ready to visit statements now, but we have some work to do to
feed them to it. First, we add a new entrypoint method to the Interpreter class
for accepting a list of statements -- in other words, a program:

^code interpret

This replaces the old temporary `interpret()` method which took a single
expression. It relies on this tiny helper method:

^code execute

That's the statement analogue to the `evaluate()` method we have for
expressions.

Since we're working with lists now, we need to let Java know:

^code import-list (2 before, 2 after)

The main Lox class is still trying to parse a single expression and pass it to
the interpreter. We fix the parsing line like so:

^code parse-statements (1 before, 2 after)

And then replace the call to the interpreter with this:

^code interpret-statements (2 before, 1 after)

Basically just plumbing the new syntax through. OK, fire up the interpreter and
give it a try. At this point, it's worth sketching out a little Lox program in a
text file to run as a script. Something like:

```lox
print "one";
print true;
print 2 + 1;
```

It almost looks like a real program! Granted, not a super useful one, but we're
making progress. Note that the REPL too now requires you to enter a full
statement instead of a simple expression. Don't forget your semicolons.

## Global Variables

Now that we have statements, we can start working on state. Before we get into
all of the complexity of lexical scoping, we'll start off with the easiest kind
of variables, <span name="globals">globals</span>. That requires two new
constructs.

1.  A **variable declaration** statement brings a new variable into the world:

        :::lox
        var beverage = "espresso";


    This creates a new binding that associates a name (here "beverage") with a
    value (here, the string `"espresso"`).

2.  Once that's done, a **variable expression** accesses that binding. When the
    identifier `beverage` is used as an expression, it looks up the value bound
    to that name and returns it.

        :::lox
        print beverage; // "espresso".

Later, we'll add assignment and block scope, but that's enough to get started.

<aside name="globals">

Global state gets a bad rap. Sure, lots of global state -- especially *mutable*
state -- makes it hard to maintain large programs. It's good software
engineering to minimize how much you use.

But when you're slapping together a simple programming language or, heck, even
learning your first language, the flat simplicity of global variables helps. My
first language was BASIC and, though I outgrew it eventually, it was a boon that
I didn't have to wrap my head around scoping rules before I could start making a
computer do fun stuff.

</aside>

### Variable syntax

As before, we'll work through the implementation from front to back, starting
with the syntax. Variable declarations are statements, but they are different
from other statements, and we're going to split the statement grammar in two to
handle them. That's because the grammar restricts where some kinds of statements
are allowed.

The clauses in control flow statements -- think the "then" and "else" parts of
an if statement or the body of a while -- are a single statement. But that
statement is not allowed to be one that declares a name. This is OK:

```lox
if (monday) print "Ugh, already?";
```

But this is not:

```lox
if (monday) var beverage = "espresso";
```

We *could* allow the latter, but it's confusing. What is the scope of that
`beverage` variable? If it goes *past* the if, then it means in some cases, the
variable exists and others it doesn't. That makes it really hard for the
compiler or a human to reason about the code. But if the variable *doesn't* live
past the if, then what's the point of declaring it at all?

Code like this is weird, so C, Java, and friends all disallow it. Instead, it's
like there are two levels of <span name="brace">"precedence"</span> for
statements. Some places where a statement is allowed -- like inside a block or
at the top level -- allow any kind of statement, including declarations. Others
only allow the "higher" precedence statements that don't declare names.

<aside name="brace">

In this analogy, a block statement, where you stuff a series of statements
inside a pair of curly braces, works sort of like parentheses for expressions. A
block statement is itself in the "higher" precedence level and can be used
anywhere, like in the clauses of an if statement.

But the statements it *contains* can be lower precedence. You're allowed to
declare variables and other names inside the block. So the curlies let you
escape back into the full statement grammar from a place where only some
statements are allowed.

</aside>

To accommodate the distinction, we add another rule for kinds of statements that
declare names:

```lox
program     = declaration* eof ;

declaration = varDecl
            | statement ;

statement   = exprStmt
            | printStmt ;
```

Statements that declare names go under the new `declaration` rule. Right now,
it's only variables, but later it will include functions and classes. Any place
where a declaration is allowed also allows non-declaring statements, so the
`declaration` rule falls through to `statement`. Obviously, you can declare
stuff at the top level of a script, so `program` routes to the new rule.

The rule for declaring a variable looks like:

```lox
varDecl = "var" IDENTIFIER ( "=" expression )? ";" ;
```

Like most statements it starts with a leading keyword. In this case, `var`. Then
an identifier token for the name of the variable being declared, followed by an
optional initializer expression. Finally, we put a bow on it with the semicolon.

To access a variable, we add a new kind of primary expression:

```lox
primary     = "true" | "false" | "null" | "this"
            | NUMBER | STRING
            | "(" expression ")"
            | IDENTIFIER ;
```

That `IDENTIFIER` clause matches a single identifier token, which is understood
to be the name of the variable being accessed.

These new grammar rules get their corresponding syntax trees. Over in the AST
generator, we add a new statement tree for a variable declaration:

^code var-stmt-ast (1 before, 1 after)

It stores the name token so we know what it's declaring, along with the
initializer expression. (If there isn't an initializer, that's `null`.)

Then we add an expression node for accessing a variable:

^code var-expr (1 before, 1 after)

It's simply a wrapper around the token for the variable name. That's it. As
always, don't forget to run the AstGenerator script so that you get updated
Expr.java and Stmt.java files.

### Parsing variables

Before we parse the new statement syntax, we need to shift around some code to
make room for the new `declaration` rule in the grammar. The grammar for the top
level of a program is now a list of declarations, so the entrypoint method to
the parser becomes:

^code parse-declaration (3 before, 4 after)

That calls this new method:

^code declaration

Hey, do you remember way back in that [earlier chapter][parsing] when we put the
infrastructure in place to do error recovery? We are finally at the point where
we can hook that up.

[parsing]: parsing-expressions.html
[error recovery]: http://localhost:8000/parsing-expressions.html#panic-mode-error-recovery

This `declaration()` method is the method we call repeatedly when parsing a list
of expressions in a block or a script, so it's the right point to synchronize to
when the parser goes into panic mode. The whole body of the method is wrapped in
a try block to catch the exception thrown when the parser begins error recovery.
This gets it back to trying to parse the beginning of the next statement or
declaration.

The real parsing happens inside the try block. First, it looks to see if we're
at a variable declaration by looking for the leading `var` keyword. If not, it
bubbles up to the "higher precedence" statement method.

Recall that `statement()` tries to parse an expression statement if no other
statement matches. And `expression()` reports a syntax error if it can't parse
an expression at the current token. So that chain of calls ensures we report an
error if a valid declaration or statement isn't parsed.

When the parser matches a `var` token, it branches to:

^code parse-var-declaration

As always, the recursive descent code follows the grammar rule. We've already
matched the `var` token, so next it requires and consumes an identifier token
for the variable name.

Then, when it sees a `=` token, it knows there is an initializer expression. If
so, it parses it. Otherwise, it leaves the initializer `null`. Finally, it
consumes the required semicolon at the end of the statement. All this gets
wrapped in a Stmt.Var syntax tree node and we're groovy.

Parsing a variable expression is even easier. In the `primary()` method where
primary expressions are handled, we look for an identifier token:

^code parse-identifier (2 before, 2 after)

That gives us a working front end for declaring and using variables. All that's
left is to feed it into the interpreter. Before we get to that, we need to talk
about where variables live in memory.

## Environments

The bindings that associate variables to values need to be stored somewhere.
Ever since the Lisp folks invented parentheses, this data structure has been
called an <span name="env">**"environment"**</span>. When you execute the code,
the environment is where you go when you have some variable and you need to find
what it represents.

<aside name="env">

I like to imagine the environment literally, as a sylvan wonderland where
variables and values frolic.

</aside>

You can think of it like a <span name="map">map</span> where the keys are
variable names and the values are the variable's, uh, values. In fact, that's
how we'll implement it in Java.

<aside name="map">

Java calls them "maps" or "hashmaps". Other languages call them "hash tables",
"dictionaries" (Python and C#), "hashes" (Ruby and Perl), "tables" (Lua), or
"associative arrays" (PHP). Way back when, they had the evocative name "scatter
tables".

</aside>

We could stuff the environment fields and the code to manage it in the
Interpreter class, but it forms a nicely delineated concept, so we'll pull it
out into its own class.

^code environment-class

There's a Java Map in there to store the bindings. It uses bare strings for the
keys, not tokens. A token represents a unit of code at a specific place in the
source text, but when it comes to looking up variables, all identifier tokens
with the same name should refer to the same variable (ignoring scope for now).
Using the string ensures all of those tokens collapse to the same map key.

There are two operations we need to support. First, a variable definition binds
a new name to a value:

^code environment-define

Not exactly brain surgery, but we have made one interesting semantic choice.
When we add the key to the map, we don't check to see that it isn't already
present. That means that this program works:

```lox
var a = "before";
print a; // "before".
var a = "after";
print a; // "after".
```

A variable statement doesn't just define a *new* variable, it can also be used
to *re*-define an existing variable. We could <span name="scheme">choose</span>
to make this an error instead. The user may not intend to redefine an existing
variable. (If they did mean to, they probably would have used assignment, not
`var`.) Making redefinition an error would help them find that bug.

However, doing so interacts poorly with the REPL. In the middle of a REPL
session, it's nice to not have to mentally track which variables you've already
defined. We could allow redefinition in the REPL but not in scripts, but then
users have to learn two sets of rules, and code copied and pasted from one form
to the other might not work.

<aside name="scheme">

My rule about variables and scoping is, "When in doubt, do what Scheme does."
The Scheme folks have probably spent more time thinking about variable scope
than we ever will -- one of the main goals of Scheme was to introduce lexical
scoping to the world -- so it's hard to go wrong if you follow in their
footsteps.

Scheme allows redefining variables at the top level.

</aside>

So, to keep the two modes consistent, we'll allow it (at least for global
variables). Once a variable exists, we need a way to look it up:

^code environment-get

This is a little more semantically interesting. If the variable is found, it
simply returns the value bound to it. But what if it's not? Again, we have a
choice.

* Make it a syntax error.
* Make it a runtime error.
* Allow it and return some default value like `nil`.

Lox is pretty lax, but the last option is a little *too* permissive to me.
Making it a syntax error -- a compile time error -- seems like a smart choice.
Using an undefined variable is a bug, and the sooner you detect the mistake, the
better.

The problem is that *using* a variable isn't the same as *referring to it* it.
You can refer to a variable in a chunk of code without immediately evaluating it
if that chunk of code is wrapped inside a function. If we make it a static error
to *mention* a variable before it's been declared, it becomes much harder to
define recursive functions.

We could accommodate single recursion -- a function that calls itself -- by
defining the function's own name before we examine its body. But that doesn't
help with mutually recursive procedures that call each other. Consider:

<span name="contrived"></span>

```lox
fun isOdd(n) {
  if (n == 0) return false;
  return isEven(n - 1);
}

fun isEven(n) {
  if (n == 0) return true;
  return isOdd(n - 1);
}
```

<aside name="contrived">

Granted, this is probably not the most efficient way to tell if a number is even
or odd (not to mention the bad things that happen if you pass a non-integer or
negative number to them). Bear with me.

</aside>

The `isEven()` function isn't defined by the <span name="declare">time</span> we
are looking at the body of `isOdd()` where it's called. If we swap the order of
the two functions, then `isOdd()` isn't defined when we're looking at
`isEven()`'s body.

<aside name="declare">

Some statically-typed languages like Java and C# solve this by specifying that
the top level of a program isn't a sequence of imperative statements. Instead, a
program is a set of declarations which all come into being simultaneously. The
implementation declares *all* of the names before looking at the bodies of *any*
of the functions.

Older languages like C and Pascal don't work like this. Instead, they force you
to add explicit *forward declarations* to declare a name before it's fully
defined. That was a concession to the limited computers of the time. They wanted
to be able to fully compile a source file in one single pass through the text,
so those compilers couldn't gather up all of the declarations first before
processing function bodies.

</aside>

Since making it a *static* error makes recursive declarations too difficult,
we'll defer the error to runtime. It's OK to refer to a variable before it's
defined as long as you don't *evaluate* the reference. That make the program for
even and odd numbers work, but you'd get a runtime error in:

```lox
print a;
var a = "too late!";
```

As with type errors in the expression evaluation code, we report a runtime error
by throwing an exception, giving it the token for the variable so it can tell
the user where in their code they messed up.

### Interpreting global variables

The Interpreter class gets an instance of the new Environment class:

^code environment-field (1 before, 1 after)

We store it as a field directly in Interpreter so that the variables stay in
memory as long as the interpreter is still running.

We have two new syntax trees, so that's two new visit methods. The first is for
declaration statements:

^code visit-var

If the variable has an initializer, it evaluates it. If not, we have another
choice to make. We could make it a syntax error by *requiring* an initializer.
Most languages don't, though, so it feels a little harsh to do so in Lox.

We could make it a runtime error. We'd let you define an uninitialized variable,
but if you accessed before assigning to it, a runtime error would occur. It's
not a bad idea. It would make the interpreter a little more complex, and
probably slower too. We'd have to check on every access to see if the variable
has a defined value.

Instead, we'll keep it simple and say that Lox implicitly initializes a variable
with `nil` if it isn't given an explicit initializer:

```lox
var a;
print a; // "nil".
```

Thus, if there isn't an initializer, we set the value to `null`, which is the
Java representation of Lox's `nil` value. Then we tell the environment to bind
the variable to that value.

Next, to evaluate a variable expression:

^code visit-variable

It simply forwards to the environment which does the heavy lifting around making
sure the variable is defined. With that, we've got rudimentary variables
working. Try this out:

```lox
var a = 1;
var b = 2;
print a + b;
```

We can't reuse *code* yet, but we can start to build up programs that reuse
*data*.

## Assignment

It's possible to create a language that has variables but does not let you
reassign -- or **"mutate"** -- them. Haskell is one example. SML only supports
mutable references and arrays -- variables cannot be re-assigned. Rust steers
you away from mutation by requiring a `mut` modifier to enable assignment.

Mutating a variable is a side effect and, as the name suggests, some language
folks think side effects are <span name="pure">dirty</span> or inelegant. Code
should be pure math that produces values -- crystalline, unchanging ones -- like
an act of divine creation. Not some grubby automaton that bends chunks of data
into shape, one imperative grunt at a time.

<aside name="pure">

I find it delightful that the same group of people who pride themselves on
dispassionate logic are also the ones who couldn't resist appropriating such
emotionally-loaded terms for their work: "pure", "side effect", "lazy",
"persistent", "first-class", "higher-order".

</aside>

Lox is not so austere. Lox is an imperative language, and mutation comes with
the territory. Adding support for assignment doesn't require much work. Global
variables already support redefinition, so most of the machinery is there now.
Mainly, we're missing an explicit assignment notation.

As we'll see, that little `=` syntax is a little more complex than it might
seem. Like most <span name="assign">C-derived</span> languages, assignment is an
expression and not a statement. As in C, it is the lowest precedence expression
form.

<aside name="assign">

Assignment is a statement in Python. It's an expression in Go, but the increment and decrement operators `++` and `--`, which are syntactic sugar for an assignment, are statements.

</aside>

### Assignment syntax

Because our grammar works from the lowest precedence up, that means we'll add it near the top of the rules, slotted between `expression` and `equality`, the next lowest precedence expression:

```lox
expression  = assignment ;
assignment  = identifier ( "=" assignment )?
            | equality ;
```

The new `assignment` rule can be an identifier followed by an `=` and an
expression for the value, or it can be an `equality` expression. Later,
`assignment` will get more complex when we add property setters on objects,
like:

```lox
instance.field = "value";
```

The easy part is adding the new syntax tree node:

^code assign-expr (1 before, 1 after)

After you run the AstGenerator to get the new Expr.Assign class, swap out the
body of the parser's existing `expression()` method to match the updated rule:

^code expression (1 before, 1 after)

Here is where it gets tricky. A single token lookahead recursive descent parser
can't see far enough to tell when it's parsing an assignment until *after* it's
gone through the left-hand side and stumbled onto the `=`. You might wonder why
it even needs to. After all, we don't know we're parsing a `+` expression until
after we've finished parsing the left operand.

The difference is that the left-hand side of an assignment isn't an expression
that evaluates to a value. It's a sort of pseudo-expression that evaluates to a
"thing" you can assign to. In:

```lox
var a = "before";
a = "value";
```

On the second line, we don't *evaluate* `a` (which would return the string
"before"). We figure out what variable `a` refers to to determine where to store
the right-hand side expression's value. The [classic terms][lvalue] for these
two <span name="lvalue">halves</span> of an assignment are **"lvalue"** and
**"rvalue"**. All of the expressions that we've seen so far that produce values
are rvalues. An lvalue "evaluates" to a storage location that you can assign
into.

[lvalue]: https://en.wikipedia.org/wiki/Value_(computer_science)#lrvalue

<aside name="lvalue">

In fact, the names come from assignment expressions. *L*-values appear on the
*left* side of the `=` in an assignment, and *r*-values on the *right*.

</aside>

Right now, the only kind of lvalues we have are simple variable names. When we
add class fields, we'll have compound lvalue expressions like:

```lox
makeList().head.next = node;
```

Because an lvalue isn't evaluated like a normal expression, the syntax tree must
reflect that. The problem is that the parser doesn't know it's parsing an lvalue
until it hits the `=`. In a complex lvalue like the previous example, that may
occur <span name="many">many</span> tokens later.

<aside name="many">

In fact, since the receiver of a field assignment can be any expression, and
expressions can be as long as you want to make them, it may take an unbounded
number of tokens of lookahead to find the `=`.

</aside>

We only have a single token of lookahead, so what do we do? We use a little
trick, and it looks like this:

^code parse-assignment

Most of the code for parsing an assignment expression looks similar to the other
binary operators like `+`. We parse the left-hand side, which can be any
expression of higher precedence. If we find a `=`, we parse the right-hand side
and then wrap it all up in an assignment expression tree node.

One slight difference from binary operators is that we don't loop to build up a
sequence of the same operator. Since assignment is right-associative, we instead
recursively call `assignment()` to parse the right-hand side.

The trick is that right before we create the assignment expression node, we look
at the left-hand side expression and figure out what kind of assignment target
it is. Then we convert its node from an rvalue to an lvalue.

This trick works because it turns out that every valid assignment target happens
to also be <span name="converse">valid syntax</span> as a normal expression.
Consider a complex field assignment like:

<aside name="converse">

You can still use this trick even if there are assignment targets that are not
valid expressions. Define a **cover grammar**, a fake grammar that accepts
the union of all of the valid expression and assignment target syntaxes. Then,
if it turns out that what you parsed isn't a valid expression and also isn't
followed by an `=`, report an error.

</aside>

```lox
newPoint(x + 2, 0).y = 3;
```

The left-hand side of that assignment could also work as a valid expression:

```lox
newPoint(x + 2, 0).y;
```

(Where the first example sets the field, the second gets it.)

This means we can parse the left-hand side as *if it were* an expression and
then after the fact produce a syntax tree for the corresponding assignment
target. If the left-hand side expression isn't a <span name="paren">valid</span>
assignment target, we fail with a syntax error. That ensures we report an error
on code like:

```lox
a + b = c;
```

<aside name="paren">

Do you remember way back in the parsing chapter when I said we represent
parenthesized expressions in the syntax tree because we'll need them later. This
is why. We need to keep track of them so that we can distinguish between:

```lox
a = 3;   // OK.
(a) = 3; // Error.
```

</aside>

Right now, the only valid target is a simple variable expression, but we'll add
fields later. The end result of this trick is an assignment expression tree node
that knows what it is assigning to and has an expression subtree for the value
being assigned. All with only a single token of lookahead and no backtracking.

### Assignment semantics

We have a new syntax tree node, so our interpreter gets a new visit method:

^code visit-assign

For obvious reasons, it's similar to variable declaration. It evaluates the
right-hand side to get the value, then stores it in the named variable. Instead
of using `define()` on Environment, it calls this new method:

^code environment-assign

The main difference between assignment and definition is that the former is not
<span name="new">allowed</span> to create a *new* variable. In terms of our
implementation, that means it's a runtime error if the key doesn't already exist
in the environment's variable map.

<aside name="new">

Unlike Python and Ruby, Lox doesn't do implicit variable declaration.

</aside>

The last thing the `visit()` method does is return the assigned value. That's
because assignment is an expression that can be nested inside other expressions,
like:

```lox
var a = 1;
print a = 2; // "2".
```

Now our interpreter's management of state is about as sophisticated as early
<span name="basic">BASIC</span> implementations. Making all variables global is
simple, but isn't easy on our users. Writing a large program where any two
chunks of code can accidentally step on each other's state is no fun. We want
*local* variables, which means it's time for *scope*.

<aside name="basic">

Maybe a little better than that. Unlike some old BASICs, Lox can handle variable
names longer than two characters

</aside>

## Scope

**Scope** defines a region where a name is mapped to a specific entity. By
allowing multiple scopes, the same name can refer to different things in
different contexts. In my house, "Bob" usually refers to me. But maybe in your
town you know a different Bob. Same name, but different entities based on where
you use it.

<span name="lexical">**Lexical scope**</span> (or the slightly less commonly
used **static scope**) is a specific style of scope where the text of the
program itself shows you where the scope begins and ends. In Lox, as in most
modern languages, variables are lexically scoped. That means that when you see
an expression that uses some variable, you can figure out which variable
definition it's referred to just by statically analyzing the code.

<aside name="lexical">

"Lexical" comes from the Greek "lexikos" which means "related to words". When we
use it in programming languages, it usually means a thing you can figure out
from source code itself without having to execute anything.

Lexical scope came onto the scene with ALGOL. Earlier languages were often
dynamically scoped. They believed dynamic scope was faster to execute. Today,
thanks to early Scheme hackers, we know that isn't true. If anything, it's the
opposite.

Dynamic scope for variables lives on some corners. Emacs Lisp defaults to
dynamic scope for variables. The [`binding`][binding] macro in Clojure provides
it. The widely-disliked [`with` statement][with] in JavaScript turns properties
on an object into dynamically-scoped variables.

[binding]: http://clojuredocs.org/clojure.core/binding
[with]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/with

</aside>

For example:

```lox
{
  var a = "first";
  print a; // "first".
}

}
  var a = "second";
  print a; // "second".
}
```

Here, we have two blocks and a variable `a` is declared in each of them. You and
I can tell just from looking at the code that the use of `a` in the first print
refers to the first `a`, and the second one refers to the second.

This is in contrast with **dynamic scope** where you don't know what a name
refers to until you execute the code. Lox doesn't have dynamically scoped
*variables*, but methods and fields on objects are dynamically scoped:

```lox
class Saxophone {
  play() {
    print "Doot doot doot";
  }
}

class Golf {
  play() {
    print "Bogey";
  }
}

fn playIt(thing) {
  thing.play();
}
```

When `playIt()` calls to `thing.play()`, we don't know if we're about to hear
"Careless Whisper" or "Fore!". It depends on whether you pass a Saxophone or a
Golf to the function, and we don't know that until runtime.

Scope and environments are close cousins. The former is the theoretical concept,
and the latter machinery implements it. As our interpreter works its way through
code, syntax tree nodes that affect scope will change the environment. In a
C-like language like Lox, scope is controlled by curly-braced <span
name="block">blocks</span>.

<aside name="block">

Naturally, that's why some people call it **block scope**.

</aside>

```lox
{
  var a = "in block";
}
print a; // Error! No more "a".
```

The beginning of a block introduces a new local scope, and that scope ends when
execution passes the closing `}`. Any variables declared inside the block
disappear.

### Nesting and shadowing

You could imagine implementing block scope like this:

1.  As we visit each statement inside the block, keep track of any variables
    declared.

2.  After the last statement is executed, tell the environment to delete all of
    those variables.

That would work for the previous example. But, remember, one motivation for
local scope is encapsulation -- a block of code in one corner of the program
shouldn't interfere with some other one. Check this out:

```lox
var counter = 0;

// Everytime this is called, increments and prints counter.
fn increment() {
  counter = counter + 1;
  print counter;
}

fn countToTen() {
  for (var counter = 1; counter <= 10; counter = counter + 1) {
    print counter;
  }
}
```

Now imagine calling `countToTen()`. After the loop finishes, the interpreter
will delete the *global* `counter` variable. That ain't right. When we exit the
block, we should remove any variables declared inside the block, but if there is
a variable with the same name declared outside of the block, *that's a different
variable*. It doesn't get touched.

When a local variable has the same name as a variable in an enclosing scope, it
**shadows** the outer one. Code inside the block can't see it any more (it is
hidden in the "shadow" cast by the inner one), but it's still there.

**todo: illustrate**

When we enter a new block scope, we need to preserve variables defined in outer
scopes so they are still around when we exit the inner block. We do that by
defining a fresh environment for each block containing only the variables
defined in that block. When we exit the block, we discard its environment and
restore the previous one.

**todo: illustrate**

We also need to handle enclosing variables that are *not* shadowed:

```lox
var global = "outside";
{
  var local = "inside";
  print global + local;
}
```

Here, `global` lives in the outer global environment and `local` is defined
inside the block's environment. In that `print` statement, both of those
variables are in scope. In order to find them, the interpreter needs to be able
to search not just the current innermost environment, but also any enclosing
ones.

We implement this by <span name="cactus">chaining</span> the environments
together. Each environment has a reference to the environment of the immediately
enclosing scope. When we look up a variable, we walk that chain from innermost
out until we find the variable. Starting at the innermost scope is how we make
local variables shadow outer ones.

<aside name="cactus">

At any point in time while the interpreter is running, the environments form a
singly-linked list of objects. But if you consider the full set of environments
that are created during the entire execution, you end up with something like a
tree. Any given outer scope may have multiple different blocks nested within it,
and each will point to it.

The boring name for this is a [**"parent-pointer tree"**][parent pointer], but I
much prefer the evocative "cactus stack".

[parent pointer]: https://en.wikipedia.org/wiki/Parent_pointer_tree
</aside>

**todo: illustrate cactus stack**

Before we start adding block syntax to the language, let's beef up our
Environment class with support for this nesting. First, we give each environment
a reference to its enclosing one:

^code enclosing-field (1 before, 1 after)

This field needs to be initialized, so we add a couple of constructors:

^code environment-constructors

The no-argument constructor is for the global scope's environment, which ends
the chain. The other constructor creates a new local scope nested inside the
given outer one.

We don't have to touch the `define()` method -- a new variable is always
declared in the current innermost scope. But variable lookup and assignment work
with an existing variable and they need to walk the chain to find it. First,
lookup:

^code environment-get-enclosing (2 before, 3 after)

If the variable isn't found in this scope, we simply try the enclosing one. That
will in turn do the same thing <span name="recurse">recursively</span>, so this
will ultimately walk the entire chain. Assignment proceeds in likewise fashion:

<aside name="recurse">

It would be faster to iteratively walk the chain, but don't you think the
recursive solution is prettier? We'll do something *much* faster in clox.

</aside>

^code environment-assign-enclosing (2 before, 1 after)

Again, if the variable isn't in this environment, we try the outer one,
recursively.

### Block syntax and semantics

Now that Environments nest, we're ready to add blocks to the language. As in C,
a block in Lox is a (possibly empty) series of statements or declarations
surrounded by curly braces. A block is itself a statement and can appear
anywhere a statement is allowed.

In grammar-ese, that's:

```lox
statement   = exprStmt
            | printStmt
            | block ;

block       = "{" declaration* "}" ;
```

Pretty straightforward. As usual, before we can parse this, we need a syntax
tree to parse it into:

^code block-ast (1 before, 1 after)

<span name="generate">It</span> contains the list of statements that are inside
the block. Parsing is straightforward too. Like other statements, we detect the
beginning of a block by its leading token -- in this case the `{`. In the
`statement()` method, we add:

<aside name="generate">

As always, don't forget to run GenerateAst.java.

</aside>

^code parse-block (1 before, 2 after)

All the real work happens here:

^code block

It creates an empty list of statements and then keeps parsing statements and
adding them to it until we reach the end of the block, marked by the closing
`}`.

Note that the loop also has an explicit check for `isAtEnd()`. In a recursive
descent parser, we have to be careful to avoid infinite loops, even in incorrect
code. If the user forgot a closing `}`, the parser needs to not get stuck.

That's it for syntax. For semantics, we add another visit method to Interpreter:

^code visit-block

Up until now, the `environment` field in Interpreter always pointed to the same
environment -- the global one. Now, it will represent the *current* environment.
It will always be the innermost scope that contains the code currently being
executed. As the interpreter enters and exits scopes, it updates the
`environment` field accordingly.

To execute a block, we create a new environment nested inside the current scope
for the block's local scope. Then the rest of the work happens in this other
method:

^code execute-block

It executes a given list of statements in the context of the given <span
name="param">environment</span>. All that means is that it updates the
interpreter's `environment` field, visits all of the statements, and then
restores the previous value. As is always good practice in Java, we restore the
previous environment using a finally clause. That way it gets restored even if
an exception is thrown.

<aside name="param">

Explicitly changing and restoring a mutable `environment` field that is used by
the interpreter may seem a little inelegant. Another classic approach to this is
to explicitly pass the environment as a parameter to each visit method. To
"change" the enviroment, you pass a different one as you recurse down the tree.
You don't have to restore the old one, since the new one lives on the Java stack
and is implicitly discarded when the interpreter returns from the block's visit
method.

I considered that for jlox, but it's kind of tedious and verbose adding an
environment parameter to every single visit method.

</aside>

Surprisingly, that's all we need to do in order to fully support local
variables, nesting, and shadowing. Go ahead and try this out:

```lox
var a = "global a";
var b = "global b";
var c = "global c";
{
  var a = "outer a";
  var b = "outer b";
  {
    var a = "inner a";
    print a;
    print b;
    print c;
  }
  print a;
  print b;
  print c;
}
print a;
print b;
print c;
```

With every bit of code, we inch closer to something resembling a full-featured
programming language.

<div class="challenges">

## Challenges

1.  The REPL no longer supports entering a single expression and automatically
    printing its result value. That's a drag. Add support to the REPL to let
    users type in both statements and expressions. If they enter a statement,
    execute it. If they enter an expression, evaluate it and display the result
    value.

2.  Maybe you want Lox to be a little more explicit about variable
    initialization. Make it a runtime error to access a variable that has not
    been initialized or assigned to, as in:

        :::lox
        // No initializers.
        var a;
        var b;

        a = "assigned";
        print a; // OK, was assigned first.

        print b; // Error!

3.  What does the following program do?

        :::lox
        var a = 1;
        {
          var a = a + 2;
          print a;
        }

    What did you *expect* it to do? Is it what you think it should do? What
    does analogous code in other languages you are familiar with do? What do
    you think users will expect this to do?

</div>

<div class="design-note">

## Design Note: Implicit Variable Declaration

Lox has distinct syntax for declaring a new variable and assigning to an
existing one. Some languages collapse those to only assignment syntax. Assigning
to a non-existent variable automatically brings it into existence. This is
called **implicit variable declaration** and exists in Python, Ruby, and
CoffeeScript, among others. JavaScript has an explicit syntax to declare
variables, but can also create new variables on assignment. Visual BASIC has [an
option to enable or disable implicit variables][vb].

[vb]: https://msdn.microsoft.com/en-us/library/xe53dz5w(v=vs.100).aspx

When the same syntax can assign or create a variable, each language has to
decide what happens when the context isn't clear about which behavior the user
intends. In particular, each language must choose how implicit declaration
interacts with shadowing, and which scope an implicitly declared variable goes
into.

*   In Python, assignment always creates a variable in the current function's
    scope, even if there is a variable with the same name declared outside of
    the function.

*   Ruby avoids some ambiguity by having different naming rules for local and
    global variables. However, blocks (which are more like closures than like
    "blocks" in C) in Ruby have their own scope, so it still has to deal with
    ambiguity. Assignment in Ruby assigns to an existing variable outside of the
    current block if there is one with the same name. Otherwise, it creates a
    new variable in the current block's scope.

*   CoffeeScript, which takes after Ruby in many ways, is similar. It explicitly
    disallows shadowing by saying that assignment always assigns to a variable
    in an outer scope if there is one, all the way up to the outermost global
    scope. Otherwise, it creates the variable in the current function scope.

*   In JavaScript, assignment modifies an existing variable in any enclosing
    scope, if found. If not, it implicitly creates a new variable *in the global
    scope*.

The main advantage to implicit declaration is simplicity. There's less syntax
and no "declaration" concept to learn. Users can just start assigning stuff and
the language figures it out.

Older statically typed languages like C benefit from explicit declaration
because they give the user a place to tell the compiler what type each variable
has and how much storage to allocate for it. In a dynamically typed, garbage
collected language, that isn't really necessary, so you can get away with making
declarations implicit. It feels a little more "scripty", more "you know what I
mean".

But is that a good idea? Implicit declaration has some problems.

*   A user may intend to assign to an existing variable, but may have misspelled
    it. The interpreter doesn't know that, so it goes ahead and silently creates
    some new variable and the variable the user wanted to assign to still has
    its old value. This is particularly heinous in JavaScript where a typo will
    create a *global* variable, which may in turn interfere with other code.

*   JS, Ruby, and CoffeeScript use the presence of an existing variable with
    some name -- even in an outer scope -- to determine whether or not an
    assignment creates a new variable or assigns to an existing one. That means
    adding a new variable in a surrounding scope can change the meaning of
    existing code. What was once a local variable may silently turn into an
    assignment to that new outer variable.

*   In Python, you may *want* to assign to some variable outside of the current
    function instead of creating a new variable in the current one, but you
    can't.

Over time, the languages I know with implicit variable declaration ended up
adding more features and complexity to deal with these problems.

*   Implicit declaration of global variables in JavaScript is universally
    considered a mistake today. "Strict mode" disables it and makes it a compile
    error.

*   Python added a `global` statement to let you explicitly assign to a
    global variable from within a function. Later, as functional-styled
    programming and nested functions became more popular, they added a similar
    `nonlocal` statement to assign to variables in enclosing functions.

*   Ruby extended its block syntax to allow declaring certain variables to be
    explicitly local to the block even if the same name exists in an outer
    scope.

Given those, I think the simplicity argument is mostly lost. There is an
argument that implicit declaration is the right *default* but I personally find
that less compelling.

My opinion is that implicit declaration made sense in years past when most
scripting languages were heavily imperative and code was pretty flat. As
programmers got more comfortable with deep nesting, functional programming, and
closures, it's become much more common to want access to variables in outer
scopes. That makes it more likely that users will run into the tricky cases
where it's not clear what whether they intend their assignment to create a new
variable or reuse a surrounding one.

So I prefer explicitly declaring variables, which is why Lox requires it.

</div>
