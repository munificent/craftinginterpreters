^title Global Variables
^part A Bytecode Virtual Machine

> If only there could be an invention that bottled up a memory, like scent. And
> it never faded, and it never got stale. And then, when one wanted it, the
> bottle could be uncorked, and it would be like living the moment all over
> again.
>
> <cite>Daphne du Maurier, <em>Rebecca</em></cite>

**todo: more asides**

**todo: illustrations**

That [last chapter][hash] was a giant helping of computer science theory. It was
a lot to digest. This chapter is more dessert or palate cleanser. There's no
large meaty concepts to chew through, just a handful of bite-sized pieces of
engineering. We're going to add support for variables to our virtual machine.

[hash]: hash-tables.html

We don't have functions or scope yet, so all we'll worry about first is global
variables. Variables are brought into being using variable declaration
statements, so we'll be adding state too. There's a deeper connection here.
Mutating -- assigning to a variable -- is a *side effect*. It doesn't produce
any interesting value on its own, but it affects the later observable behavior
of the program when you read that variable.

Statements were made for side effects. Since statements don't produce values,
they wouldn't be useful at all if our language didn't have any side effects. So,
with variables, we are adding both statements and a reason to have statements in
the first place.

## Statements and Declarations

Before we get to variables, let's get our parser and compiler handling
statements. This actually requires two new categories productions in the
grammar. If you recall, Lox distinguishes *statements* and *declarations*.
Declarations are those statements that bind a new name to a value. We don't
allow them directly inside flow control statements, like:

```lox
if (monday) var croissant = "yes"; // Error.
```

Allowing this would raise some confusing questions around the scope of the
variable. So, like other languages, we prohibit it syntactically by having two
separate grammar rules for the subset of statements that are allowed inside a
control flow body:

```lox
statement      → exprStmt
               | forStmt
               | ifStmt
               | printStmt
               | returnStmt
               | whileStmt
               | block ;
```

And the set of statements allowed at the top level of a script and inside a
block:

```lox
declaration    → classDecl
               | funDecl
               | varDecl
               | statement ;
```

Since `block` itself is in `statement`, you can always get declarations inside a
control flow construct by nested them inside a block.

In this chapter, we'll only cover a couple of statements and one
declaration:

```lox
statement      → exprStmt
               | printStmt ;

declaration    → varDecl
               | statement ;
```

We'll save the rest for later chapters, but now we'll at least get the
scaffolding in place for both productions. Up to now, our VM considered a
"program" to be a single expression since that's all we could parse and compile.
In a real Lox program, a script is a sequence of declarations. We're ready to do
that now:

^code compile (1 before, 1 after)

We keep compiling declarations until we hit the end of the source file. That
outer if statement handles the degenerate case where the entire file is empty.
Since statements and declarations can refer to each other in the grammar, the
functions for compiling those are recursive. That means forward declarations in
C:

^code forward-declarations (1 before, 1 after)

The main `compile()` function calls:

^code declaration

We'll get to variable declarations later in the chapter so for now, it just
forwards to `statement()`:

^code statement

**todo: subheader for print stmt somewhere in here?**

We're going to implement print and expression statements. If the first token
is `print`, it's obviously a print statement. We determine that using this
helper function:

^code match

It should be familiar from jlox. If the current token has the given type, it
consumes it and returns `true`. Otherwise it leaves it alone and returns
`false`. We implement this in terms of:

^code check

It simply returns `true` if the current token has the given type. I know these
seem pointless now, but they'll pay their way as we reuse them in later
chapters. If we did match the `print` token, then we compile the rest of the
statement using:

^code print-statement

A print statement evaluates an expression and prints the result, so we first
parse and compile that expression. The grammar expects a semicolon after that,
so we consume it. Finally, we emit a new instruction to print the
result.

^code op-print (1 before, 1 after)

At runtime, we execute this instruction like so:

^code interpret-print (1 before, 1 after)

We compiled the expression first, so the code for that has produced some value
and left it on top of the stack. Now we simply pop it and print it.

Note that we don't push anything else after that. Inside the VM, this is a key
difference between expressions and statements. Each bytecode instruction has a
**stack effect**. This describes how the instruction modifies the size of the
stack. For example, `OP_ADD` pops two values and pushes one, leaving the stack
one element smaller than before.

You can also consider the combined stack effect of a series of instructions. If
you add up the stack effects of the series of instructions compiled from any
expression, it will also total one. The expression leaves the result value on
the stack.

The bytecode for a statement has a total stack effect of zero. Since a statement
produces no values, it ultimately leaves the stack unchanged, though it of
course uses the stack while it's doing its thing. This is important because when
we get to flow control and looping, a program might execute a long series of
statements. If each of them grew or shrank the stack, the VM might overflow or
underflow the stack.

While we're in the interpreter loop, here's another little change:

^code op-return (1 before, 1 after)

When the VM only compiled and evaluated a single expression, we had some
temporary code in `OP_RETURN` to output the value. Now that we have statements
and `print` we don't need that anymore. We're one step closer to the complete
implementation of clox. Only one step, though. We'll revisit `OP_RETURN` again
when we add functions. Right now, it exits the entire interpreter loop.

As usual, our new instruction needs support in the disassembler.

^code disassemble-print (1 before, 1 after)

That's our `print` statement. If you want, give it a whirl:

```lox
print 1 + 2;
print 3 * 4;
```

I know, mind-blowing, right?

### Expression statements

Wait until you see the next statement:

^code expression-statement

An "expression statement" is simply an expression followed by a semicolon.
They're how you write an expression in a context where a statement is expected.
Usually, it's so that you can call a function for its side effect, like:

```lox
sayHi("friend");
```

Semantically, an expression statement evaluates the expression and discards
the result. The compiler does a literal translation of that. It emits the code
for the expression, and then an `OP_POP` instruction:

^code pop-op (1 before, 1 after)

As the name implies, this pops the top value off the stack and forgets it:

^code interpret-pop (1 before, 2 after)

We can disassemble it too:

^code disassemble-pop (1 before, 1 after)

Expression statements aren't very useful yet since we can't create any
expressions that have side effects, but they'll be essential when we add
functions later. The majority of statements in real-world code in languages like
C are expression statements.

**todo: link**

### Error synchronization

While we're getting this initial structure in place in the compiler, we can tie
off a loose end we left several chapters back. Like jlox, clox uses panic mode
error recovery to minimize the number of cascaded compile errors that it
reports. The compiler exits panic mode when it reaches a synchronization point.
For Lox, we chose statement boundaries as that point. Now that we have
statements, we can implement synchronization:

^code call-synchronize (1 before, 1 after)

If we hit a compile error while parsing the previous declaration, we will enter
panic mode. In that case, after the declaration, we starting synchronizing:

^code synchronize

This skips tokens indiscriminately until it reaches something that looks like a
statement boundary. If the previous token can end a statement -- like a
semicolon -- then we're right at the boundary. Likewise, if the next token can
begin one -- usually one of the control flow or declaration keywords -- then
we're there.

## Variable Declarations

Being able to *print* won't win your language any prizes at the programming
language <span name="fair">fair</span>, so let's move on to something a little
more interesting and get variables going.

<aside name="fair">

I can't help but imagine a "language fair" like some country 4H thing with rows
of straw-filled stalls, full of baby languages moo-ing and baa-ing at each
other.

</aside>

A quick refresher on Lox semantics: Global variables in Lox are "late bound" or
resolved dynamically. This means you can compile a chunk of code that refers to
a global variable before it's defined. As long as the code doesn't *execute*
before the definition happens, everything is fine. In practice, that means you
can refer to later variables inside the body of functions:

```lox
fun showVariable() {
  print global;
}

var global = "after";
showVariable();
```

Code like this might seem odd, but it's handy for defining mutually recursive
functions. It also plays nicer with the REPL. You can write a little function in
one line, then define the variable it uses in the next.

The way we choose to implement global variables in the VM is informed by this
need. If it was a static error to use a global variable before it's defined --
as it is with *local* variables -- then we might choose to compile them
differently.

There are three operations we need to support for variables:

*   Declaring a new variable using a `var` statement.
*   Accessing the value of a variable using an identifier expression.
*   Storing a new value in an existing variable using an assignment expression.

We can't do anything until we *have* some variables, so we'll start with
declarations:

^code match-var (1 before, 2 after)

The placeholder parsing function we sketched out for the declaration grammar
rule has an actual production now. If we match a `var` token, we jump to:

^code var-declaration

The keyword is followed by the variable name. That's compiled by
`parseVariable()`, which we'll get to in a second. Then we look for an `=`
followed by an initializer expression. If the user doesn't initialize the
variable, the compiler implicitly initializes it to `nil` by emiting an `OP_NIL`
instruction. Either way, we expect the statement to be terminated with a
semicolon.

There are two new functions here for working with variables and identifiers. The
first one is:

^code parse-variable

It requires the next token to be an identifier token. If found, it calls:

^code identifier-constant

This function takes the given token and adds its lexeme to the chunk's constant
table as a string. It then returns the index of that constant in the constant
table.

Global variables are accessed by name at runtime. That means the VM -- the
bytecode interpreter loop -- needs access to the name. A whole string is too big
to stuff into the bytecode stream as an operand. Instead, we store the string in
the constant table and the instruction can then refer to it by its index in the
table.

So this function returns that index all the way to `varDeclaration()` which
then passes it along to:

^code define-variable

<span name="helper">This</span> outputs the bytecode instruction define the new
variable at runtime. The index of the variable's name in the constant table is
the instruction's operand. As usual in a stack-based VM, we emit this
instruction last. At runtime, we'll execute the code for the variable's
initializer. That will leave the value on the stack. This instruction will take
that and store it away for later.

<aside name="helper">

I know some of these functions seem pretty pointless right now. But we'll get
more mileage out of them as we add more language features for working with
names. Function and class declarations both declare new variables, and variable
and assignment expressions access them.

</aside>

Over in the runtime, we begin with this new instruction:

^code define-global-op (1 before, 1 after)

Since we already built our own hash table, the implementation isn't too hard:

^code interpret-define-global (1 before, 2 after)

We get the name of the variable from the constant table. Then we <span
name="pop">take</span> the value from top of the stack and store it in a hash
table with that name as the key.

<aside name="pop">

Note that we don't *pop* the value until *after* we add it to the hash table.
That ensures the VM can still find the value if a garbage collection is
triggered right in the middle of adding it to the hash table. That's a distinct
possibility since the hash table requires dynamic allocation when it resizes.

</aside>

This code doesn't check to see if the key is already in the table. Lox is pretty
lax with global variables and lets you redefine them without error. That tends
to be handy in a REPL session, so the VM supports that by simply overwriting the
value if the key happens to already be in the hash table.

There's another little helper macro:

^code read-string (1 before, 2 after)

It reads a one-byte operand from the bytecode chunk. It treats that as an index
into the chunk's constant table and returns the string at that index. It doesn't
check that the value *is* a string -- it just indiscriminately casts it. That's
safe because the compiler never emits an instruction that refers to a non-string
constant.

Because we care about lexical hygiene, we also undefine this macro at the end of
the interpret function:

^code undef-read-string (1 before, 1 after)

I keep saying "the hash table", but we don't actually have one yet. We need a
place to store these globals. Since we want them to persist as long as clox is
running, we store them right in the VM:

^code vm-globals (1 before, 1 after)

As we did with the string table, we need to initialize the hash table to a valid
state when the VM boots up:

^code init-globals (1 before, 1 after)

And we need to <span name="tear">tear</span> it down when we exit:

<aside name="tear">

Honestly, we don't. The process is going to exit and free everything anyway, but
it feels undignified to require the operating system to clean up our mess.

</aside>

^code free-globals (1 before, 1 after)

As usual, we want to be able to disassemble the new instruction too:

^code disassemble-define-global (1 before, 1 after)

And with that, we can define global variables. Not that users can *tell* that
they've done so, because they can't actually *use* them. So let's fix that next:

## Reading Variables

Like every programming language ever, a variable's value is accessed by way of
the variable's name. We hook up identifier tokens to the expression parser here:

^code table-identifier (1 before, 1 after)

That calls:

^code variable-without-assign

As with declarations, there are a couple of tiny helper functions that seem
pointless now but will become more useful in later chapters. In particular,
this function will ultimately be used when compiling `this` expressions too:

^code read-named-variable

It uses the same `identifierConstant()` function from before to take the given
identifier token and add its lexeme to the chunk's constant table as a string.
All the remains is to emit an instruction to load the global variable with that
name:

^code get-global-op (1 before, 1 after)

Over in the bytecode interpreter, the implementation mirrors `OP_DEFINE_GLOBAL`:

^code interpret-get-global (1 before, 1 after)

We pull the constant table index from the instruction's operand and then look
up the variable name. Then we use that as a key to look up the variable's value
in the global's hash table.

If the key isn't present in the hash table, it means that global variable has
never been defined. That's a runtime error in Lox, so we report it and exit the
interpreter loop if that happens. Otherwise, we take the value and push it
onto the stack.

^code disassemble-get-global (1 before, 1 after)

A little bit of disassembly code, and we're done. Now we can start writing code
that looks more like an actual programming language:

```lox
var beverage = "cafe au lait";
var breakfast = "beignets with " + beverage;
print breakfast;
```

There's only one operation left...

## Assignment

Throughout this book, I've tried to keep you on a fairly safe and easy path. I
don't avoid hard problems, but I try to not make the solutions harder than they
need to be. Alas, other design choices in our <span name="jlox">bytecode</span>
compiler make assignment more annoying to implement than it might otherwise be.

<aside name="jlox">

If you recall, assignment was pretty easy in jlox.

</aside>

Our bytecode VM uses a single-pass compiler. It parses and generates bytecode
on the fly without any intermediate AST. As soon as it recognizes a piece of
syntax, it emits code for it. Assignment doesn't naturally fit that. Consider:

```lox
menu.brunch(sunday).beverage = "mimosa";
```

In this code, the parser doesn't realize `menu.brunch(sunday).beverage` is the
target of an assignment and not a normal expression until it reaches `=`, many
tokens after the first `menu`. By then, the compiler has already emitted
bytecode for the whole thing.

It's not as dire as it might seem, though. Even though the `.beverage` part
needs to be compiled a setter and not a getter, the rest of the expression
behaves the same as if it weren't the target of an assignment. The
`menu.brunch(sunday)` part can be compiled and executed as usual.

Fortunately for us, it's only the very right-most bit of syntax whose behavior
changes when appearing before a `=`. Even though the receiver of a setter may be
an arbitrarily long expression, the setter itself is only a `.` followed by an
identifier, which is right before the `=`. We don't need much look ahead to
realize `.beverage` should be compiled as a setter and not a getter.

Variables are even easier since they are just a single bare identifier. So the idea is that right *before* compiling an expression that can also be used as an assignment target, we look for a subsequent `=` token. If we see one, we compile it as an assignment or setter instead of a variable access or getter.

We don't have setters to worry about yet, so all we need to handle is variables:

^code named-variable (1 before, 1 after)

In the parse function for identifier expressions, we look for a subsequent
equals sign. If we find one, instead of emitting code for a variable access,
we compile the assigned value and then emit an assignment instruction.

That's the last instruction we need to add in this chapter:

^code set-global-op (1 before, 1 after)

As you'd expect, its runtime behavior is similar to defining a new variable:

^code interpret-set-global (1 before, 2 after)

The only difference is what happens when the key doesn't already exist in the
globals hash table. If the variable hasn't been defined yet, it's a runtime
error to try to assign to it. Lox [doesn't do implicit variable
declaration][implicit].

[implicit]: statements-and-state.html#design-note

And a little disassembly:

^code disassemble-set-global (1 before, 1 after)

So we're done, right? Not quite. We've made a mistake! Take a gander at:

```lox
a * b = c + d;
```

According to Lox's grammar, `=` has the lowest precedence, so this should be
parsed roughly like:

```lox
(a * b) = (c + d);
```

Obviously, `a * b` isn't a <span name="do">valid</span> assignment target, so
this should be a syntax error. But here's what our parser does:

<aside name="do">

Wouldn't it be wild if it *was*, though? You could imagine some algebra-like
language that would try to divide the assigned value up in some reasonable way
and distribute it to `a` and `b`. That's probably a terrible idea.

</aside>

1.  First, `parsePrecedence()` parses `a` using the `variable()` prefix parser.
1.  After that, it enters the infix parsing loop.
1.  It reaches the `*` and calls `binary()`.
1.  That recursively calls `parsePrecedence()` to parse the right-hand operand.
1.  That calls `variable()` again for parsing `b`.
1.  Inside that call to `variable()`, it looks for a trailing `=`. It sees one
    and thus parses the rest of the program as an assignment.

In other words, the parser sees the above code like:

```lox
a * (b = c + d);
```

We've messed up the precedence handling because `variable()` doesn't take into
account the precedence of the surrounding expression that contains the variable.
If the variable happens to be the right-hand side of an infix operator, or the
operand of a unary operator, than that containing expression is too high
precedence to permit the `=`.

To fix this, `variable()` should only look for and consume the `=` if it's in
the context of a low precedence expression. The code that knows the current
precedence is, logically enough, `parsePrecedence()`. The `variable()` function
doesn't need to know the actual level. It just cares if it's low enough to allow
assignment, so we can pass that fact in as a Boolean:

^code prefix-rule (4 before, 2 after)

Since assignment is itself the lowest precedence expression, the only time we
allow an assignment is if the precedence is that or lower. That flag makes its
way to:

^code variable

Which accepts the parameter:

^code named-variable-signature (1 after)

And then uses it:

^code named-variable-can-assign (2 before, 1 after)

Pretty straight-forward. If the variable is nested inside some expression with
higher precedence, `canAssign` will be `false` and this will ignore the `=`
even if there is one there. Then this returns and eventually makes its way back
to `parsePrecedence()`.

Then what? What does this do with our broken example from before? Right now, `variable()` won't consume the `=` so that will be the current token. It returns
back to `parsePrecedence()`. `variable()` is a prefix parser, so then it tries
to enter the infix parsing loop. There is no parsing function associated with
`=`, so it skips that loop.

Then `parsePrecedence()` silently returns back to the caller. That isn't right.
If the `=` doesn't get consumed as part of the expression, nothing else is going
to consume it. It's an error and we should report it:

^code invalid-assign (5 before, 1 after)

With that, the previous bad program correctly gets an error at compile time. OK,
*now* are we done? Still not quite. See, we're passing an argument to one of the
parse functions. But those functions are stored in a table of function pointers,
so they all need to have the same type. Even though most parse functions don't
support being used as an assignment target -- setters are the only other one --
C requires them *all* to take and ignore the parameter.

So we got a little grunt work to do. First, let's go ahead and pass the flag
to the infix parse functions:

^code infix-rule (1 before, 1 after)

We'll need that for setters eventually. Then we'll fix the function type for
parse functions:

^code parse-fn-type (2 before, 2 after)

And some completely tedious code to accept this parameter in all of our existing
parse functions:

^code binary (1 after)

And:

^code parse-literal (1 after)

And:

^code grouping (1 after)

And:

^code number (1 after)

And:

^code string (1 after)

And, finally:

^code unary (1 after)

That gets us back to a C program we can compile. Fire it up and now you can run:

```lox
var breakfast = "beignets";
var beverage = "cafe au lait";
breakfast = "beignets with " + beverage;

print breakfast;
```

It's starting to look like real code for an actual language!

<div class="challenges">

## Challenges

1.  The compiler adds a global variable's name to the constant table as a string
    every time an identifier is encountered. It creates a new constant each
    time, even if that variable name is already in a previous slot in the
    constant table. That's wasteful in the common case when the same variable is
    referenced multiple times by the same function and increases the odds of
    filling up the constant table and running out slots, since we only allow 256
    constants in a single chunk.

    Optimize this. How does your optimization affect the performance of the
    compiler? Is this the right trade-off?

2.  Looking up a global variable by name in a hash table each time it is used
    is pretty slow, even with a good hash table. Can you come up with a more
    efficient way to store and access global variables without changing the
    semantics?

    [todo: answer. globals hash table maps names to indexes. globals stored in
    separate array. at compile time, add/look up name in array. instr operand
    is array index. have special "undefined" value to tell if not defined yet.

3.  When running in the REPL, a user might write a function that references so
    unknown global variable. Then, in the next line, they declare the variable.
    Lox should handle this gracefully by not reporting an "unknown variable"
    compile error when the function is defined.

    But when a user runs a Lox *script*, the compiler has access to the full
    text of the entire program before any code is run. Consider this program:

        :::lox
        fun useVar() {
          print oops;
        }

        var ooops = "too many o's!";

    Here, we can tell statically that `oops` will not be defined because there
    is *no* declaration of that global anywhere in the program. Note that
    `useVar()` is never called either, so even though the variable isn't
    defined, no runtime error will occur because it's never used either.

    We could report mistakes like this a compile errors, at least when running
    from a script. Do you think we should? Justify your answer. What do other
    scripting languages you know do?

</div>
