^title Resolving and Binding
^part A Tree-Walk Interpreter

> Once in a while you find yourself in an odd situation. You get into it by
> degrees and in the most natural way but, when you are right in the midst of
> it, you are suddenly astonished and ask yourself how in the world it all came
> about.
>
> <cite>Thor Heyerdahl</cite>

Way back when we [added variables and block scope][statements] to Lox, we got
the scoping rules right then. But when we [later added functions][functions]
and, in particular, closures, we inadvertantly poked a hole in our formerly
watertight lexical scoping implementation. In practice, most programs are
unlikely to stumble into this hole, but as language implementers, we take a
sacred vow to care about correctness even in the most obscure dark corners of
the semantics.

[statements]: statements-and-state.html
[functions]: functions.html

So we will spend this entire chapter plumbing the depths of that hole, and then
carefully, thoroughly, patching it up. In the process, we will gain a more
rigorous understanding of lexical scoping as used by Lox and other languages in
the C tradition. We'll also get a chance to learn about *static analysis* -- a
useful technique for writing tools that extract meaning from the user's source
code.

## Static Scope

When we introduced variables, we talked about how Lox, like most modern
languages, uses *lexical* scoping. This means that when you're sitting there
looking at a variable usage, you (and an implementation) can figure out which
declaration that variable refers to just be reading the text of the program. For
example:

```lox
var a = "outer";
{
  var a = "inner";
  print a;
}
```

Here, we can figure out that the `a` being printed is the variable declared on
the previous line, and not the one declared globally. The dynamic execution of
the program doesn't affect this. In other words, the scope rules are part of the
*static* semantics of the language and Lox features **static scope**.

I've never really spelled out what these static scoping rules are. They are
similar to languages you are familiar with like C and Java, so I assumed you had
the right idea already. Now is a good time to be a <span name="precise">little
more precise</span>.

<aside name="precise">

This is still nowhere near as precise as a real language specification should
be. I'm aiming for just good enough for you and I to have the same idea in mind.
A real language spec intends to be so unambiguous that even a Martian our an
outright malicious implementer would still be forced to implement the correct
semantics as long as they followed the letter of the spec.

That level of clarity is important when a language may be implemented by
multiple competing companies who have an incentive to have their implementations
be incompatible with each other to lock customers into their own
implementations. For this book, we don't need to worry about that level of
malice.

**todo: malice synonym.**

</aside>

**A use of a variable refers to the preceding variable declaration of the same
name in the innermost scope that encloses the expression where the variable is
used.**

There's a lot to unpack in that:

*   I say "use of a variable" instead of "variable expression" to cover both
    variable expressions and assignments. Likewise "expression where the
    variable is used".

*   "Preceding" means appearing before in the program text. Given:

        :::lox
        var a = "outer";
        {
          print a;
          var a = "inner";
        }

    Here, the `a` bring printed is the outer one since it appears <span
    name="hoisting">before</span> the print statement that uses it. In most
    cases, in straight line code, the declaration preceding in *text* will also
    precede the usage in *time*.

    <aside name="hoisting">

    JavaScript does not always follow this rule. In JavaScript, variables
    declared anywhere in a block are implicitly "hoisted" to the beginning of
    the block. That means any of use a variable with the same name in the block
    will refer to that variable, even if the use appears before the declaration.
    When you write this in JavaScript:

        :::js
        {
          console.log(a);
          var a = "value";
        }

    It implicitly treated like:

        :::js
        {
          var a; // Hoist.
          console.log(a);
          a = "value";
        }

    Note that it means that in some cases, you can access a variable before its
    initializer has run, an annoying source of bugs in JavaScript.

    </aside>

    But that's not *always* true. Consider:

        :::lox
        var a = "outer";
        for (var i = 1; i <= 2; i = i + 1) {
          if (i == 2) print a;
          var a = "inner";
        }

    Here, the second inner declaration of `a` will execute before the access of
    `a` in the print does. Even though the second declaration executes first in
    time, it does not *precede* the access in the print statement. This
    distinction is important because it what seperates the static textual
    ordering of the code from the dynamic temporal execution.

*   "Innermost" is there because of our good friend shadowing. There may be more
    than one variable with the given name in enclosing scopes, as in:

        :::lox
        var a = "outer";
        {
          var a = "inner";
          print a;
        }

    Our rule disambiguates this case by saying the innermost scope wins.

This dense rule, once you unfurl it, implies that a variable expression always
refers to the same declaration through the entire execution of the program.
That's why it's called *static* scope.

Our interpreter so far mostly implements the above rule correctly. But when we
added closures, we let an error slip in:

```lox
var a = "global";
{
  fun showA() {
    print a;
  }

  showA();
  var a = "block";
  showA();
}
```

Before you type this in and run it, decide what you think it *should* print.
If you're familiar with closures in other languages, you expect it to print
"global" twice. The first call to `showA()` should definitely print "global"
since we haven't even reached the declaration if the inner `a` yet. And by our
implied rule that a variable expression always resolves to the same variable,
that means the second call to `showA()` should print the same thing.

Now try it out. Alas, it prints:

```
global
block
```

I want to stress that this program never reassigns any existing variable and
only contains a single print statement. Yet somehow, that print statement for a
never-assigned variable prints two different values at different points in time.
Oops.

### Scopes and mutable environments

In our interpreter, environments are the dynamic sister the static notion of
scopes. The two mostly stay in sync with each other -- we create a new
environment when we enter a new scope, and discard it when we leave the scope.
There is one other operation we perform on environments: declaring a variable in
one. This is where our bug lies.

Let's walk through that problematic example and see what the environments look
like at each step.

**todo: illustrate and explain**

- first just have env for global scope
- enter block and create child env
- declare showA() in that env
- closure captures block
- call showA
- create new env for activation with parent as closure
- look up a, find in global
- declare a in block scope
- call showA again
- create new env for activation with parent as closure
- look up a, find in block

When we implemented environments, we chose a representation that agrees with our
informal intuition. We tend to think of all code within the same block as being
within the same scope, so our interpreter uses a single environment to represent
that. Each environment is a mutable hash table. When a new variable is declared,
it gets added to the existing environment.

This clashes with how we implement closures. When a function is declared, it
captures a reference to the current environment. The function *should* be
capturing a frozen snapshot of the environment as it exists at the point that
the function is declared.

But, instead, in the Java code, it has a reference to the actual mutable
Environment object. If a variable is later declared in the scope that
environment corresponds to, the closure will see the new variable, even though
the declaration does *not* precede the function.

A closure can end up "seeing" local variables that are declared after the
function.

### Persistent environments

Maybe it's our intuition that's wrong. A block is *not* all actually the same
scope. If you define "scope" to mean "a set of declarations", it's clear that
these two points in the program aren't in the same scope:

```lox
{
  var a;
  // 1.
  var b;
  // 2.
}
```

At the first marked line, only `a` is in scope. At the second line, both `a` and
`b` are. It's as if each variable declaration <span name="split">splits</span>
the block into two separate scopes, the scope before the variable is declared
and the one after, which includes the new variable.

<aside name="split">

Some languages make this split explicit. In Scheme and ML, when you declare a
local variable using `let`, you also delineate the subsequent code where the new
variable is in scope. There is no implicit "rest of the block".

</aside>

There is a style of programming that uses what are called **"persistent data
structures"**. Unlike the normal mutable data structures you're familiar with in
imperative programming, a persistent data structure can never be directly
modified. Instead, any "modification" to an existing structure produces a <span
name="copy">brand</span> new structure that contains all of the original data
and the new modification. The original is left unchanged.

<aside name="copy">

This sounds like it might waste tons of memory and time copying the structure
each time. In practice, persistent data structures share most of their data
between the different "copies".

</aside>

If we were to apply that technique to Environment, then every time you declared a variable it would return a *new* environment that contained all of the previously-declared variables along with the one new variable.

**todo: illustrate**

Since every declaration produces a new Environment object, it means each
environment only ever contains a single declaration. Instead of a HashMap of
variables, all Environment would need is a single one. Something like:

```java
class Environment {
  final Environment previous;
  final String variable;
  final Object value;

  private Environment(
      Environment previous, String variable, Object value) {
    this.previous = previous;
  }

  Environment define(String name, Object value) {
    return new Environment(this, name, value);
  }
}
```

Instead of a chain of environments with maps for each *scope*, you get a linear
chain of environments for each *variable*. Something like:

**todo: illustrate**

We'd actually need two environment classes, and a common interface, since the
global environment does contain a single mutable map of variables. Global
variables *are* dynamically resolved, by design, to play nicer with the REPL.
Buy you get the idea.

This doesn't look like it buys us much. But consider how closures work now. A
closure retains a reference to the environment that was current when the
function was declared. Any variables declared after that point don't modify that
environment. Instead, they produce new ones, which the closure won't see. Our
bug would be fixed.

This is one viable way to solve the problem, and it's the classic way to
implement environments in Scheme interpreters. We could do that for Lox, but it
would mean going back and changing a lot of existing code.

Instead, we'll keep the way we represent environments, and change the way we
access variables. Instead of making the data structure persistent and static,
we'll bake the static resolution into the access operation itself.

## Resolution

Our interpreter resolves a variable -- tracks down which declaration it refers
to -- each time the variable expression is evaluated. Even if the variable is
used inside a loop that runs a thousand times, it gets re-resolved a thousand
times.

Static scope means that that variable should always resolve to the same
declaration, which can be determined just by looking at the text. Given that,
why are we doing it dynamically every time? Doing so doesn't just open the hole
that leads to our annoying bug, it's also needlessly slow.

A better solution would be to resolve each variable access *once*. We'll store
that resolution in some way and then the interpreter can reuse it each time it
executes the expression that uses the variable.

There are a lot of ways we could represent the binding between a variable and
its declaration. We could redo our entire Environment class. When we get to the
C interpreter for Lox, we'll have a *much* more efficient way of storing and
accessing variables.

For now, though, I'd like to minimize the amount of collatoral damage we need to
inflict on our existing codebase. I'd hate to make you throw out a bunch of
mostly-fine code. Instead, we'll represent the resolution in a way that makes
the most out of our existing Environment structure.

Recall how the accesses of `a` are interpreted in the problematic example.

**todo: illustrate**

In the first (correct) evaluation, we walk up three environments in the chain
before finding the global declaration of `a`. Then, when the inner `a` is later
declared in an inner scope, it shadows the global one. The next look up walks
the chain only two hops and stops there.

**todo: are the numbers right here?**

If we could ensure a variable access always walked the *same* number of levels
in the environment chain, that would prevent the resolved variable from changing
over time. Since each environment corresponds to a single lexical scope, we'd
ensure that we are looking up the same variable every time.

To "resolve" a variable usage, we only need to calculate how many "hops" away
the declared variable will be in the environment chain. We can do that once,
since it's a static property of the variable and doesn't rely on dynamic
execution.

We can't calculate the actual Environment *object* where the variable can be
found ahead of time. Remember that we create new environments dynamically each
time a function is called so that recursion works. Instead, we'll determine just
the relative distance from the current environment to the enclosing one where
the variable can be found. Even when the environment objects themselves are
created dynamically, the length of the chain itself always matches the source
text.

**todo: illustrate**

The interesting question is *when* to do this calculation -- or, put
differently, where in our interpreter's implementation do we put the code for
it? Since this is a static property that we can calculate based on the structure
of the source code, the obvious answer is in the parser. This is the traditional
approach, and is what we'll do later in clox.

It would work here too, but I want an excuse to show you another technique.
We'll write our resolver as a separate pass.

### A static analysis pass

After the parser produces the syntax tree, but before the interpreter starts
executing it, we'll do a single traversal of the entire tree to resolve all of
the variable. Doing a separate static pass like this after parsing is a
generally handy technique.

If Lox had static types, that's how we'd implement the type checker. It would
would the trees calculating the static type of each expression and making sure
they lined up with the expected types where the expressions are used. <span
name="constant">Optimizations</span> are often implemented in separate passes
like this too. Basically, any work that doesn't rely on state that's only
available at runtime can be done in this way.

<aside name="constant">

A simple example of an optimization like this is **constant folding**. Walk the
syntax tree in a bottom up fashion starting with the leaves of each expression.
When you encounter an arithmetic operator whose operands are both literal
numbers, do the math right then and replace the entire operator tree and its
operands with a new number literal syntax tree node containing the result value.

By running this optimization from the leaves up, you ensure subexpressions are
optimized out first, which lets you evaluate entire complex, nested arithmetic
expressions ahead of time into a single number.

**todo: fix overlap.**

</aside>

This pass works much like a simple <span name="abstract">interpreter</span> of
the syntax tree. It walks the tree, visiting each node. A couple of things make
it a static pass instead of a dynamic execution:

<aside name="abstract">

**Abstract interpretation** is a compilation technique that further blurs the
line between static analysis and runtime execution.

</aside>

*   **There are no side effects.** When the static analysis visits a print
    statement, it doesn't actually print anything. Calls to native functions or
    other operations that reach out to the outside world are stubbed out and
    have no effect.

*   **There is no control flow.** Loops are only visited once. Both branches are
    visited in if statements. Logic operators are not short-circuited. Each
    piece of syntax is touched exactly <span name="fix">once</span> and only
    once.

<aside name="fix">

The simple variable resolution we'll do only touches each node once. It's
performance is `O(n)` where `n` is the number of nodes in the syntax tree. Some
more sophisticated static analyses may have greater algorithmic complexity and
need to do more work to calculate the answers they seek.

Most common analyses are carefully designed to be linear or not too far from it,
though. It's no fun if the compiler takes gets exponentially slower as your
program grows. It's an even more embarrassing faux pas to discover your
algorithm can get stuck in an infinite loop on some inputs.

</aside>

## A Resolver Class

Like everything in Java, our variable resolution pass will live in a class:

^code resolver

Since it needs to "visit" every node in the syntax tree, it will implement the
handy Visitor abstraction we already have in place for exactly that. Only a
couple of nodes are interesting for purposes of resolving variables:

*   A block statement introduces a new scope for the statements it contains, as
    does a function declaration for its body.

*   A variable declaration adds a new variable to the current scope.

*   Variable and assignment expressions need to have their variabl resolved.

The rest of the nodes aren't particularly interesting, but we still need to
implement visit methods for them that traverse into their subtrees. Even though
a `+` expression doesn't *itself* have any variables to resolve, either of its
operands might be variables, or might contain them.

### Resolving blocks

We'll start with blocks since they create the local scopes that everything else
hinges on:

^code visit-block-stmt

It begins a new scope, traverses into the statements inside the block, and then
discards that block scope. The fun stuff lives in those helper methods.

^code resolve-statements

The first one simply walks a list of statements and resolves each one. That in
turn calls:

^code resolve-stmt

It's similar to the `evaluate()` and `execute()` methods in Interpreter -- it
simply bounces back to apply the Visitor pattern to the given syntax tree node.

This is all pretty mundane. The real interesting stuff is around scope. A new
block scope is created using this:

^code begin-scope

Lexical scopes nest and in both the interpreter and the resolver, they are
treated like a stack. The interpreter implements that stack using a linked
list -- the chain of Environment objects. In the resolver, we use an actual
Java Stack:

^code scopes-field (1 before, 2 after)

Each object in the stack is a Map that represents a single block scope. Keys,
as in Environment, are variable names. Values are different here. We'll get to
why they are Booleans soon.

The scope stack is only used for *local* block scopes. Variables declared at the
top level in the global scope are not tracked since those variables are a little
more dynamic in Lox. When resolving a variable, if we can't find it in the stack
of local scopes, we assume it must be global.

Since the scopes are tracked in an explicit stack, exiting a scope is
straightforward:

^code end-scope

This gives us a stack of empty scopes. To make them useful, next we'll handle
declarations.

### Resolving variable declarations

Resolving a variable declaration adds a new entry to the current innermost
scope's map. That seems simple, but there's a little dance we need to do:

^code visit-var-stmt

Binding a new name in the scope takes two steps -- declaring it, and defining
it. That's to handle this funny edge case:

```lox
var a = "outer";
{
  var a = a;
}
```

What happens when the initializer for a local variable refers to a variable with
the same name as what's being declared? We have a couple of options:

*   **Treat the new variable as not in scope until *after* the initializer
    completes.** That means here the new local `a` would be initialized with
    "outer", the value of the *global* one. You can think of an initialized
    variable declaration as syntactic sugar for:

        :::lox
        var temp = a; // Run the initializer.
        var a;        // Declare the variable.
        a = temp;     // Initialize it.

*   **Put the new variable in scope before its initializer is run.** Of course,
    that raises the question of what value it has. After all, the initializer
    hasn't run yet. We'd probably use `nil`. That means the new local `a` would
    be re-initialized to its own implicitly initialzed value, `nil`. It's as
    if every local variable declaration is:

        :::lox
        var a; // Define the variable.
        a = a; // Run the initializer.

*   **Make it an error.** If you look at the previous two options, do either of
    those look like something a user actually *wants*? Shadowing is rare and
    often an error so initializing a shadowing variable based on the value of
    the shadowed one seems unilkely to be deliberate.

    The second option is even less useful. The new variable will *always* have
    the value `nil`, there is never any point in mentioning it by name. You
    could use an explicit `nil` instead.

    Given that, it's reasonable to simply tell the user they probably made a
    mistake by making it a syntax or runtime error to refer to a local variable
    inside its own initializer.

For Lox, we'll take the third option. Further, we'll make it a compile-time
error instead of a runtime one. That way, the user is alerted to the problem
before any code is run.

In order to do that, we need to keep track of which variables are in the
ephemeral state where we are in the middle of resolving their initializers. We
do that by splitting binding into two steps. The first is *declaring* it:

^code declare

This adds it to the innermost scope so that it shadows any outer one and so that
we know the variable exists. We mark it as "not ready yet" by binding its name
to `false` in the scope map. Each value in the scope map means "is finished
being initialized".

Then we resolve the variable's initializer expression itself in the scope where
the new variable is declared but unavailable. Once the initializer expression is
done, the variable is ready for prime time. We do that by *defining* it:

^code define

Now we set the variable's value in the scope map to `true` to mark it as fully
initialized and available for use. It's alive!

### Resolving variable expressions

The other statement that binds a new name is function declarations, but before
we go there, let's see how variables are used. Now that are scopes contain some
stuff, we can resolve variable expressions:

^code visit-variable-expr

First, we check to see if the variable is being accessed inside its own
initializer. This is where the values in the scope map come into play. If the
variable exists in the current scope but it's value is `false`, that means we
have declared it but not yet defined. We report that error.

We only do this check for local variables. We allow global variables to be
re-declared to make the REPL a little more friendly. In that case, allowing a
global to mention its previous incarnation can be handy, though admittedly odd
looking.

After that check, we actually resolve the variable itself using this:

^code resolve-local

This looks, for good reason, a lot like the code in Environment for looking up a
variable. We start at the innermost scope and work outwards, looking in each map
for a matching name. If we find the variable, we tell the interpreter to resolve
it, passing in the number of scopes between the current innermost scope and the
scope enclosing where the variable was found. So, if the variable was found in
the current scope, it passes in 0. If it's in the immediately enclosing scope,
1. You get the idea.

We'll get to the implementation of that method a little later. For now, let's
keep on cranking through other syntax nodes.

### Resolving function declarations

The other statement form that binds names is a function declaration. It binds
the name of the function itself in the scope where it's declared, and also binds
names for the function's parameters inside its body.

^code visit-function-stmt

Similar to `visitVariableStmt()`, it declares and then defines the name of the
function in the current scope. Unlike variables, though, it defines the name
eagerly, before stepping into the function's body. This lets a function refer to
itself inside its own body for recursion, even if the function is declared
inside a block or other function. This is safe because the reference to the
function won't be *executed* until the function is called, which can't happen
until the declaration has finished.

Then it handles the function by using this:

^code resolve-function

It's a separate method since we will also use it later for resolving Lox class
methods. Similar to a block, it pushes a new scope for the function's body. Then
it defines variables for each of the function's parameters.

Once that's done, it resolves the function body in that scope. This is different
from how the interpreter handles function declarations. At *runtime* declaring a
function doesn't do anything with the function's body. That doesn't get touched
until later when the function is called. In a *static* analysis, we immediately
traverse into the body right then and there.

### Resolving assignment expressions

Variables are "used" by being read, but also by being written, so we also need
to resolve them in assignments. It looks like this:

^code visit-assign-expr

We resolve the expression that calculates the value being assigned in case it
also contains references to other variables using this helper:

^code resolve-expr

Then we use our existing `resolveLocal()` to resolve the variable that's being
assigned to. Since an assignment doesn't modify the scope, these could be done
in either order.

### Resolving the other syntax tree nodes

That covers all of the interesting corners of the syntax tree. We handle every
place where a variable is declared, read or written, and every place where a
scope is created or destroyed.

All that remains is to implement visit methods for the other syntax tree nodes
to recurse into their subtrees. Even though there's nothing interesting to do
with variables in a binary operator itself, either operand may contain a
variable, so we need to traverse through it.

**todo: illustrate ast with var in leaf**

We'll go kind of "top down" and start with statements. This is going to be kind
of slog, so let's just grind through it.

^code visit-expression-stmt

An expression statement contains a single expression to traverse.

^code visit-if-stmt

Here, we can see how resolution is different from interpretation. When we
resolve an if statement, there is no control flow. We resolve the condition and
*both* branches. Where a dynamic execution only steps into the branch that *is*
run, a static analysis is conservative -- it analyzes any branch that *could* be
run. Since either one could be reached at runtime, it resolves both.

Moving along...

^code visit-print-stmt

Like expression statements, this resolves the single expression.

^code visit-return-stmt

Same deal.

^code visit-while-stmt

Like if statements, for while, we resolve the condition and alway resolve the
body exactly once. That covers all the statements.

^code visit-binary-expr

Our old friend the binary expression. We traverse into and resolve both
operands.

^code visit-call-expr

Calls are similar -- we walk the argument list and resolve them all. The thing
being called is also an expression (usually a variable expression) so that gets
resolved to.

^code visit-grouping-expr

This one's pretty easy.

^code visit-literal-expr

This is even easier. Since a literal expression doesn't mention any variables
and doesn't contain any subexpressions, there is no work to do at all.

^code visit-logical-expr

Since a static analysis does no control flow or short-circuiting, logical
expressions are exactly the same as other binary operators.

^code visit-unary-expr

And, finally, the last node. We just resolve its one operand. If we did that all
right, now our Java compiler should be satisfied that we've fully implemented
Stmt.Visitor and Expr.Visitor

## Interpreting Resolved Variables

OK, that's our resolver. What does it actually do? It looks at every
mention of a variable in the user's program. For each one, the resolver tracks
down the declaration that it refers to. Then it counts the number of scopes
between the use of the variable and its declaration.

At runtime, this corresponds exactly to the number of *environments* between the
current one and the enclosing one where interpreter can find the variable's
value. The resolver gives that to the interpreter by calling this:

^code resolve

We want to store the resolution information somewhere so we can use it when the
variable use is later executed, but where? One obvious place is on the variable
expression or assignment expression itself -- right in the syntax tree nodes.
That's a fine approach, and that's where many compilers store the results of
their static analysis.

We could do that, but it would require mucking around with our syntax tree
generator. Instead, we'll take another common approach and store it off to the
<span name="side">side</span> in a map that associates each syntax tree node
with its resolved data.

<aside name="side">

I've heard this called a "side table" since the map is sort of a tabular data
structure that tracks some information off to the side. But every time I try to
Google the literature to find more background on this term, I just get pages
about furniture.

One nice thing about using a side table to store static analysis results, is
that it makes it easy to *discard* the data. In an IDE or interactive editor
where you may often incrementally reparse and resolve parts of the user's
program, it can become difficult to track down which state stored in variable
syntax tree nodes has become invalidated and needs to be recalculated.

If it all lives in a single table, you can clear the whole thing in one fell
swoop.

</aside>

The map is a new field on the Interpreter class:

^code locals-field (1 before, 2 after)

You might think we'd need some sort of nested tree structure to keep track of
each use of a variable and avoid getting confused when there are multiple
expressions that reference the same variable. But each variable *expression* is
its own Java object with its own unique identity. A single monolithic Map for
the entire program or REPL session won't have any trouble keeping them
separated.

As usual, using a collection requires us to import a couple of names:

^code import-hash-map (1 before, 1 after)

And:

^code import-map (1 before, 2 after)

### Accessing a resolved variable

Our interpreter now has access to each variable's resolved location. Finally we
get to make some use of that. We replace the visit method for variable
expressions with this:

^code call-look-up-variable (1 before, 1 after)

That delegates to:

^code look-up-variable

There's a couple of things going on here. First, we look up the resolved
distance in the map. Remember that we only resolved *local* variables. Globals
are treated specially and don't end up in the map (hence the name `locals`). So,
if we don't find the distance in the map, it must be global. In that case, we
look it up, dynamically, in the global environment. That throws a runtime error
if the variable isn't defined.

If we *did* get a distance, we have a local variable, and we get to take
advantage of the results of our static analysis. Instead of calling `get()`, we
call this new method on Environment:

^code get-at

The old `get()` method dynamically walks the chain of enclosing environments,
scouring each one to see if the variable might be hiding in there somewhere.
But, now, we know exactly which environment in the chain will have the variable.
We simply walk that many hops along the environment chain, and return the
variable's value in that map. We don't even have to check to see if the variable
is there -- we know it will be because the resolver already found it there.

<aside name="coupled">

The way the interpreter assumes the variable will be in that map feels like
flying blind to me. The interpreter is trusting that the resolver did its job
and resolved the variable correctly. This means there is a deep coupling between
these two classes. Each line line of code in the resolver that touches a scope
must have its exact match in the interpreter for modifying an environment.

I felt that coupling that first-hand because as I implemented them for the book,
I ran into a number of subtle bugs where the resolver and interpreter code were
slightly out of sync. Tracking those down was difficult. One tool to make that
easier is to have the interpreter explicitly assert -- using Java's assert
statements or some other validation tool -- the contract it expects the resolver
to have already upheld.

Even so, expect to have to pay close attention when you change either the
resolver or interpreter and think carefully about whether that requires a
corresponding change in the other pass.

**todo: overlap**

</aside>

### Assigning to a resolved variable

The other way a variable is used is when it's written. The changes to visiting
an assignment statement are similar:

^code resolved-assign (2 before, 1 after)

Again, we look up the variable's scope distance. If not found, we assume it's
global and handle it the way as before. Otherwise, we call this new method:

^code assign-at

As `getAt()` is to `get()`, this is to `assign()`. It walks a fixed number of
enviroments, and then stuffs the new value in that map.

Those are the only changes to our interpreter. This is why we chose a
representation for our resolved data that was minimally invasive. All of the
rest of the nodes continue working just as they did before. Even the code for
modifying environments is unchanged.

## Running the Resolver

We do need to actually *run* the resolver, though. We insert the new pass after
the the parser does it's magic:

^code create-resolver (3 before, 1 after)

We don't run the resolver if there are any parse errors. If the code has a
syntax error, it's never going to run, so there's little value in resolving it.
Otherwise, we tell the resolver to do it's thing. It has a reference to the
interpreter and pokes the resolved data directly into it as it walks over
variables. When we next run the interpreter, it has everything it needs.

At least, that's true if the resolver *succeeded*. But what about errors during
resolution itself?

### Resolution errors

Since we are doing a static analysis pass, we have an opportunity to make Lox's
semantics more precise, and to help users catch more bugs early before running
their code. Take a look at this bad boy:

```lox
fun bad() {
  var a = "first";
  var a = "second";
}
```

We do allow declaring multiple variables with the same name in the *global*
scope, but doing so in a local scope is probably a mistake. If they knew the
variable already existed, they would assign to it instead of using `var`. And if
they *didn't* know it existed, they probably don't intend to overwrite the
previous one.

We can detect this statically while resolving:

^code duplicate-variable (1 before, 1 after)

When we declare a variable in a local scope, we already know the names of every
variable previously declared in that same scope. If we see a collision, we
report an error.

### Invalid return errors

Here's a another nasty little script:

```lox
return "at top level";
```

This is executing a return statement but it's not even inside a function at all.
It's top level code. I don't know what the user *thinks* is going to happen, but
I don't think we want Lox to allow this.

We can extend the resolver to detect this statically. Much like it tracks scopes
as it walks the tree, it can track whether or not the current code is inside a
function declaration or not:

^code function-type-field (1 before, 2 after)

Instead of a bare Boolean, it uses this funny enum:

^code function-type

It seems kind of dumb now, but we'll add a couple more cases to it later and
then it will make more sense. When we resolve a function declaration, we set
this field before resolving the body.

^code set-current-function (1 before, 1 after)

We stash the previous value of that field in a local variable first. Remember,
Lox has local functions, so you can nest function declarations arbitrarily
deeply. We need to keep track not just that we're in a function, but *how many*
we're in.

We could use an explicit stack of FunctionType values for that, but instead
we'll piggyback on the Java stack. We store the previous value in a local on the
Java stack. When we're done resolving the function body, we restore the field to
that value:

^code restore-current-function (1 before, 1 after)

Now that we can always tell whether or not we're inside a function declaration,
we check that when resolving a return statement:

^code return-from-top (1 before, 1 after)

Neat, right?

There's one more piece. Back in the main Lox class that stitches everything
together, we are careful to not run the interpreter if any parse errors are
encounter. That check runs *before* the resolver so that we don't try to resolve
syntactically invalid code.

But we also need to skip the interpreter if there are resolution errors, so we
add *another* check:

^code resolution-error (1 before, 2 after)

You could imagine doing lots of other analysis in here. For example, if we added
break statements to Lox, we probably want to ensure they are only used inside
for loops.

We could go farther and report warnings for code that isn't necessarily *wrong*
but probably isn't useful. For example, many IDEs will warn if you have
unreachable code after a return statement, or a local variable whose value is
never read. All of that would be pretty easy to add to our static visiting pass,
or as <span name="separate">separate</span> passes.

<aside name="separate">

The choice of how many different analyses to lump into a single pass is
difficult. Many small isolated passes, each with their own responsibility tend
to be simpler to implement and maintain. However, there is a real runtime to
traversing the syntax tree itself, so every time you add another pass, you are
sacrificing some performance.

</aside>

But, for now, we'll stick with that limited amount of analysis. The important
was that we fixed that one weird annoying edge case bug, though it might be
surprising that it took this much work to do it.

<div class="challenges">

## Challenges

1.  How do other languages you know handle local variables that refer to the
    same name in their initializer, like:

        :::lox
        var a = "outer";
        {
          var a = a;
        }

    Is it a runtime error? Compile error? Allowed? Do they treat global
    variables differently? Do you agree with their choices? Justify your answer.

1.  Extend the resolver to report an error if a local variable is never used.

1.  Write a separate static pass that implements simple constant folding of
    arithmetic expressions on numbers. It should walk all expressions bottom
    up. If the expression is a binary or unary operator and the operands are
    both numbers, it should replace the expression with a literal expression
    containing the result of the operation.

    So, if given a program like:

        :::lox
        print 1 + 2 * 3;

    It would produce:

        :::lox
        print 7;

    Hint: Since our syntax trees are immutable, the easiest way to implement
    this is to have it produce an entirely new syntax tree, even for statements
    and expressions that are unchanged. (A faster implementation might allow
    mutating syntax trees, or only replace those whose children are actually
    affected by constant folding.)

</div>
