^title Representing Code
^part A Tree-Walk Interpreter

> To dwellers in a wood, almost every species of tree has its voice as well as
> its feature.
> <cite>Thomas Hardy</cite>

In the last chapter, we took the raw source code as a string and transformed it
into a slightly higher-level representation -- a series of tokens. In the next
chapter, the parser will take those tokens and produce yet another
representation. ince we are now higher up the mountain of understanding, it will
be a richer, more complex representation of the code. Before we can write that
parser, we need to spend some time talking about that data structure itself.

Along the way, we'll learn some theory around formal grammars, a little about
the differences between function and object-oriented programming, go over a
couple of design patterns, and do some metaprogramming.

But first, let's think about how we might want to shape our code's
representation. The goal is something that's simple for the parser to produce,
and easy for the interpreter to consume. Since you probably haven't written a
parser or interpreter yet, it's hard to know what kind of structure accomplish
that.

Maybe our informal intuition can help us out. What is your brain doing when you
are playing the part of a *human* interpreter? Consider how you manually
evaluate an arithmetic expression like:

```lox
1 + 2 * 3 - 4
```

Because you understand the rules of precedence -- the old "Please Excude My Dear
Aunt Sally" stuff -- you know which subexpressions to evaluate first. You fill
in the implicit grouping:

```lox
(1 + (2 * 3)) - 4
```

Another way to visualize that grouping is using a tree:

**TODO: illustrate tree and bottom-up eval**

Leaves are numbers, and interior nodes are arithmetic operators. In order to
evaluate one of the arithmetic nodes, you need to know the numeric value of each
of its subtrees, so you have to evaluate those first. That means working your
way from the leaves up -- a "post-order" traversal. In this example, you end up
with 3.

It seemed pretty straightforward for us to draw one of these trees given the
original text of the expression. Once we had that, it wasn't too hard to
manually evaluate it. So it intuitively seems like a workable representation of
our code is a tree that matches the grammatical structure of the language. We
need to get more precise about what that grammar is then. Like with the lexical
grammar of the previous chapter, there is a metric ton of theory around
syntactic grammars as well.

We're going to go into that theory a little more than we did when scanning
because it turns out to be a really useful tool through much of the interpreter.
It starts by moving one level up the [Chomsky hierarchy][]...

[chomsky hierarchy]: https://en.wikipedia.org/wiki/Chomsky_hierarchy
**TODO: asides?**

## Context-Free Grammars

In the last chapter, the formalism we used for defining the lexical grammar --
the rules for how characters get grouped into tokens -- was called a *regular
language*. Our scanner emits a flat sequence of tokens, and, indeed, regular
languages aren't powerful enough to handle the nested structure we need for
expressions and their subexpressions.

We need a bigger hammer, and that hammer is a **context-free grammar**. It's the
next heavier tool in the toolbox of [formal grammars][]. A **formal grammar** is
a tool for understanding a language, where "language" is defined pretty
abstractly.

[formal grammars]: https://en.wikipedia.org/wiki/Formal_grammar

Each formal grammar takes a set of atomic pieces it calls its "alphabet". Then
it specifies a (usually infinite) set of "strings" that are in the grammar. Each
string is a sequence of "letters" in the alphabet.

I'm using all those quotes because the terms get a little confusing as you move
from lexical to syntactic grammars. In our scanner's grammar, the alphabet
consists of individual characters and the strings are the valid lexemes. In the
syntactic grammar we're talking about now, we're at a different level of
granularity. Now each "letter" in the alphabet is an entire token and a "string"
is a sequence of *tokens*.

A formal grammar's job is to define which strings are valid and which aren't. If
we were defining a grammar for English sentences, "Eggs are tasty for
breakfast." is in the grammar but, "Tasty breakfast for are eggs." not so much.

### Rules for grammars

How do we define a grammar that contains an infinite number of valid strings? We
obviously can't list them all out. Instead, we define finite a set of *rules*.
You can think of them as a game that you can "play" in one of two directions.

If you start with the rules, you can use them to *generate* strings that are in
the grammar. Each step of the game involves picking a rule and seeing what it
tells you to do. Most of the lingo around formal grammars comes from thinking of
them in that way. Each rule is called a **production** because it is used to
*produce* a string in the grammar.

Each production in a context-free grammar has a *name*, then a *body* which
describes what it generates. In it's pure form, the body is simply a list of
symbols. There are two kinds:

*   A **terminal** is a letter from the grammar's alphabet. You can think of it
    like a literal value. In the syntactic grammar we're defining, the terminals
    are individual lexemes -- tokens coming from the scanner like `if` or
    `1234`.

    These are called "terminals" because they don't lead to any further "moves"
    in the game. You simply produce that one symbol.

*   A **nonterminal** is a named reference to another rule in the grammar. It
    means "play that rule and insert whatever it produces here". In this way,
    our grammar composes.

There is one more key refinement: you may have multiple rules with the same
name. When you reach a nonterminal with that name, you are allowed to pick any
of the rules for it, whichever floats your boat.

We need some kind of <span name="turtles">notation</span> to write down the
production rules. Ever since John Backus snuck into Noam Chomsky's class and
stole a bit of linguistics theory to use when specifying ALGOL 58, programmers
have been using some notation for context-free grammars to pin down their
language's syntax.

<aside name="turtles">

Yes, we need to define a syntax to use for the rules that define our syntax.
Should we specify *that* syntax too? What notation do we use for it? It's
languages all the way down!

</aside>

For some reason, virtually every single one of them has tweaked the notation in
one way or another. I tried to come up with something clean. Each rule is a
name, followed by an arrow (`→`), followed by its sequence of symbols. Terminals
are quoted strings, and nonterminals are lowercase words.

Using that, here's a grammar for a <span name="breakfast">breakfast</span> menu:

<aside name="breakfast">

Yes, I really am going to be using breakfast examples throughout this entire
book. Sorry.

</aside>

```lox
breakfast → protein "with" bread
breakfast → protein
breakfast → bread

protein → protein "and" protein
protein → "bacon"
protein → "sausage"
protein → cooked "eggs"

cooked → "scrambled"
cooked → "poached"
cooked → "fried"

bread → "toast"
bread → "biscuits"
bread → "English muffin"
```

We can now play our grammar to generate random breakfasts. Let's play a round
and see how it works. By age-old convention, the first rule is where you start
playing the game. We roll a die and pick the first production. Our resulting
string looks like:

```text
protein "with" bread
```

We need to expand that first nonterminal, `protein`, so we pick a production for
that. Let's again pick the first one:

```lox
protein → protein "and" protein
```

Note that the production refers to its own rule. This is the key difference
between context-free and regular languages. The former are allowed to be
recursive. It is exactly this that lets them nest and compose, and generate an
unbounded number of strings.

We could keep picking the first production for protein over and over again
yielding all manner of breakfasts like "bacon and sausage and sausage and bacon
and...". We won't though. We need to again pick a production for protein for the
inner reference to protein. This time we'll pick "bacon". We finally hit a
nonterminal, so set that as the first word in the resulting string.

Now we pop back out to the first `protein "and" protein`. The next symbol is
"and", a nonterminal, so we add that. Then we pick another protein. This time,
we pick:

```lox
protein → cooked "eggs"
```

We need to pick a production for `cooked` and pick "poached". That's a
nonterminal, so we add that. Now we're back to the protein, so we add "eggs". We
bounce back to breakfast and add "with". Now all that's left is to pick a
production for bread. We'll pick "English muffin". That's again a non-terminal,
so we add that and we're done:

```text
bacon and poached eggs with English muffin
```

**TODO: illustrate**

Any time we hit a rule that had multiple productions, we just picked one
arbitrarily. It is this flexibility that allows a short number of grammar rules
to encode a combinatorially larger set of strings. The fact that a rule can
refer to itself -- directly or indirectly -- kicks it up even more, letting us
pack an infinite number of strings into a finite grammar.

### Enhancing our notation

Stuffing an infinite set of strings in a handful of rules is pretty fantastic,
but let's take it a bit farther. Our notation works, but it does feel a little
tedious. So, like any good language designer, we'll sprinkle a bit of syntactic
sugar on top. Almost everyone who uses a notation for CFGs ends up doing so.

The most well-known of them is [Enhanced Backus-Naur form][ebnf] (EBNF). It's
groovy, but I want something that looks a little more familiar if you're used to
regular expression syntax. In addition to terminals and nonterminals, we'll
allow a few other kinds of expressions on the production side of a rule:

[ebnf]: https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form

*   Instead of repeating the rule name each time we want to add another
    production for it, we'll allow a series of productions separated by `|`:

        :::lox
        bread → "toast" | "biscuits" | "English muffin"

*   Further, we'll allow `(` and `)` for grouping and then allow `|` within that
    to allow selecting one from a series of options within the middle of a
    production:

        :::lox
        protein → ( "scrambled" | "poached" | "fried" ) "eggs"

*   Using <span name="recursion">recursion</span> to support repeated sequences
    of symbols has a certain appealing <span name="purity">purity</span>, but
    it's kind of a chore to make a separate named sub-rule each time we want
    repetition. Instead, we allow a postfix `*` to mean the previous symbol or
    group can be repeated zero or more times.

        :::lox
        protein → protein ( "and" protein )*

<aside name="purity">

This is how the Scheme programming language works. It has no built-in looping
functionality at all. Instead, all repetition is expressed in terms of
recursion.

</aside>

*   A postfix `+` is similar, but requires the preceding production to appear
    at least once.

*   A postfix `?` is for an optional production. The thing before it can appear
    zero or one time, but not more.

        :::lox
        breakfast → protein ( "with" bread )?

With all of that sugar, our breakfast grammar now looks like:

```lox
breakfast → protein ( "with" bread )?
          | bread

protein   → protein "and" protein
          | "bacon"
          | "sausage"
          | ( "scrambled" | "poached" | "fried" ) "eggs"

bread     → "toast" | "biscuits" | "English muffin"
```

Not too bad, I hope. If you're used to grep or using regular expressions in your
text editor, most of the punctuation should be familiar. The main difference is
that symbols here represent entire lexemes, not raw characters.

We'll use this notation throughout the rest of the book to precisely describe
Lox's grammar. As you spend more time working on programming languages, you'll
find context-free grammars (using this or EBNF or some other notation) will help
you crystallize your informal design ideas for syntax. They are also a handy
medium for communicating with other language hackers about syntax.

The rules and productions we define for Lox will also be our guide to the tree
data structure we're going to implement to represent code in memory. Before we
can do that, we need an actual grammar for Lox, or at least enough of it for us
to get started.

### A Grammar for Lox expressions

In the previous chapter, we did the entire lexical grammar in one fell swoop.
Every keyword and bit of punctuation is there. The syntactic grammar is a good
bit bigger, and it would be a real bore to grind through the entire thing before
we actually get our interpreter up and running.

Instead, we'll start with a subset of the language in the next couple of
chapters. Once we have that minilanguage represented, parsed, and interpreted,
later chapters will progressively add new features to it, starting with the
syntax. For now, we are only going to worry about a handful of expressions:

*   **Literals.** Numbers, strings, Booleans, and `nil`.

*   **Unary expressions.** A prefix `!` to perform a logical not, and `-`
    to negate a number.

*   **Binary expressions.** The infix arithmetic (`+`, `-`, `*`, `/`) and logic
    (`==`, `!=`, `<`, `<=`, `>`, `>=`) operators we know and love.

*   **Parentheses for grouping.**

That gives us enough to encode expressions like:

```lox
1 - (2 * 3) < 4 == false
```

Using our handy dandy new notation, the grammar for those looks like this:

```lox
expression → literal
           | unary
           | binary
           | grouping

literal    → NUMBER | STRING | "true" | "false" | "nil"
grouping   → "(" expression ")"
unary      → ( "-" | "!" ) expression
binary     → expression operator expression
operator   → "==" | "!=" | "<" | "<=" | ">" | ">="
           | "+"  | "-"  | "*" | "/"
```

There's one bit of extra <span name="play">notation</span> here. In addition to
quoted strings for terminals that match exact lexemes, we also use `ALL CAPS`
for terminals that are a single lexeme whose text representation may vary.
`NUMBER` is any number literal, and `STRING` is any string literal. Later, we'll
do the same for `IDENTIFIER`.

This grammar is actually ambiguous, which we'll see when we get to parsing it.
But it's good enough to hang our data structure off of for now.

<aside name="play">

If you're so inclined, try using this grammar to generate a few expressions like
we did with the breakfast grammar before. Do the resulting expressions look
right to you? Can you make it generate anything wrong like `1 + / 3`?

</aside>

## Implementing Syntax Trees

Finally, we get to start building some data structures. We'll use that little
expression grammar as our skeleton. Since the grammar is recursive -- note how
`grouping`, `unary`, and `binary` all refer back to `expression`, our data
structure will form a tree. Since this structure represents the syntax of our
language, it's called a <span name="ast">**"syntax tree"**</span>.

<aside name="ast">

In particular, we're defining an ***abstract* syntax tree** (or **AST**). In a
**parse tree**, every single grammar production becomes a node in the tree. An
AST elides productions that aren't needed by later phases.

</aside>

Our scanner used a single Token class to represent all kinds of lexemes. To
distinguish the different kinds, a TokenType enum was sufficient. Since every
token stores the same data regardless of what kind of lexeme its for, there was
no <span name="token-data">need</span> to for separate classes.

<aside name="token-data">

This isn't exactly true. Tokens for literals store the value but other kinds of
lexemes don't need that state. I have seen some scanners that do use different
classes for literals and other kinds of lexemes, but I figured I'd keep things a
little simpler for us.

</aside>

Our syntax tree is not so homogenous. Unary expressions have a single operand,
binary expressions have two, and literals have none. We *could* mush that all
together into a single class with an arbitrary list of children. Some compilers
do.

But I like getting the most out of Java's type system. So we'll define a base
class for expressions. Then, for each kind of expression -- each rule listed
`expression` in the grammar above -- we'll create a subclass that has the state
specific to that kind of expression. Each interesting terminal or nonterminal in
that rule's production will become a field on its class. This way, we can make
it a compile error to, say, try to access the second operand of an unary
expression.

A little something like:

```java
package com.craftinginterpreters.lox;

abstract class Expr { // [expr]
  static class Binary extends Expr {
    Binary(Expr left, Token operator, Expr right) {
      this.left = left;
      this.operator = operator;
      this.right = right;
    }

    final Expr left;
    final Token operator;
    final Expr right;
  }

  // Other expressions...
}
```

<aside name="expr">

I avoid abbreviations in my code because they can trip up a reader who doesn't
know what they stand for. But in compilers I've looked at, "Expr" and "Stmt" are
so ubiquitous that I may as well start getting you used to them now.

</aside>

Expr is the base class that all expression classes inherit from. I went ahead
and nested the subclasses inside of it. There's no real need for this, but it
lets us stuff all of the classes into a single file. If we didn't, we'd probably
end up suffixing each subclass with `Expr` anyway, so this makes that automatic.

### Disoriented objects

You'll note that, much like the Token class, there aren't any methods here. It's
a dumb struct. Nicely typed, but merely a bag of data. This feels strange in an
object-oriented language like Java. Shouldn't the class do stuff?

The problem is that these tree classes aren't owned by any single domain. Should
it have methods related to parsing, where the trees are produced? Or
interpreting, where they are consumed? It has one foot on each side, which means
it's really a part of neither.

In fact, these types exist to enable the parser and interpreter to
*communicate*. That lends itself to types that are simply data with no
associated behavior. This style is very natural in functional languages like
Lisp and ML where *all* data is separate from behavior, but it feels odd in
Java.

Functional programming aficionados right now are jumping up to exclaim "See!
Object-oriented languages are a bad fit for an interpreter!" I won't go that
far. You'll recall that the scanner itself worked great as an object-oriented
class. It had all of the mutable state to keep track of where it was in the
source code, a well-defined set of public methods, and a handful of private
helper ones.

My feeling is that each phase or part of the interpreter works fine in an
object-oriented style. It's just these data structures that flow between and are
owned by none that get stripped of behavior. That's OK. An object-oriented
language can handle that.

### Metaprogramming the trees

While Java can certainly express behavior-less classes, I wouldn't say that's
particularly great at it. Eleven lines of code to define a simple structure that
has three fields is pretty tedious, and when we're all done, we're going to have
21 total syntax tree classes. That's a lot of boilerplate.

I don't want to waste your time or my ink writing all that down. Really, what
the essential information for each subclass? It has a name, and a list of
fields. Each field has a name and a type. That's it.

We're smart language hackers, right? Let's go meta. Instead of tediously
hand-writing each class definion, field declaration, constructor, and
initializer, we'll hack together a little <span name="python">script</span> that
does it for us. It has a description of each tree type -- its name and fields --
and it prints out the Java code needed to define a class with that name and
state.

This script is a one file Java command-line app that writes a file called
Expr.java.

<aside name="python">

An actual scripting language like Python or, heck, even Lox itself would be a
better fit for this than Java, but I'm trying not to throw too many languages at
you.

</aside>

^code generate-ast

Note that this file is in a different package, `.tool` instead of `.lox`. This
script isn't part of the interpreter itself. It's a tool *we*, the people
hacking on the interpreter, run ourselves to generate the syntax tree classes.
When it's done, we treat `Expr.java` like any other file in the implementation.
We are merely automating how that file gets created.

To generate the classes, it needs to have some description of each class and its
fields:

^code call-define-ast (1 before, 1 after)

To try to keep things terse, I jammed the description into a single string that
we'll slice up later. Each string is the name of the class followed by `:` and
the list of fields separated by commas. Each field has a type and name.

The first thing that function needs to do is output the main Expr class that
contains everything:

^code define-ast

When we call this, base name is "Expr", which is both the name of the class and
the name of the file it outputs. We pass this in instead of hardcoding it
because we'll have another family of classes later when we add statements.

Inside, Expr, we create each subclass:

^code nested-classes (2 before, 1 after)

This isn't the world's most beautiful or efficient string munging code, but
that's fine. Remember, we are the only ones who will ever run this code, and it
only runs on the exact set of class definitions we give it. Robustness ain't a
priority.

It in turn calls:

^code define-type

There we go. All of that glorious Java boilerplate is done. It declares each
field in the class body. It also defines a constructor for the class with a
parameter for each field. The body initializes the fields based on the
parameters.

If you run this script now, it blasts out almost a hundred lines of code. That's
only going to get longer as we add new types. Aren't you glad we automated it?

## Working with Trees

Put on your imagination hat for a moment. Even though we aren't there yet, think
about how the interpreter is going to use the syntax trees. It's going to need
to interpret every kind of expression with some different code for each one.

With tokens, you can simply switch on the TokenType. But we don't have a
corresponding "type" enum for the syntax trees, just a different Java class for
each one. You could imagine some long chain like:

```java
if (expr is Expr.Binary) {
  // ...
} else if (expr is Expr.Grouping) {
  // ...
} else // ...
```

That's verbose and slow. Also, the Java compiler won't tell us if we forget to
add support for some new expression class. With an enum, we get a compile
error when a switch is missing a case.

When you have a family of classes and you need to associate a chunk of behavior
with each one, the natural solution in an object-oriented language like Java is
to put that behavior into methods on the classes themselves. You could imagine
us adding an abstract <span name="interpreter-pattern">`interpret()`</span>
method on Expr which each subclass implements to interpret itself.

<aside name="interpreter-pattern">

This exact thing is literally called the ["Interpreter pattern"][interpreter
pattern] in "Design Patterns: Elements of Reusable Object-Oriented Software".

[interpreter pattern]: https://en.wikipedia.org/wiki/Interpreter_pattern

</aside>

This works alright for small projects, but it tends to scale poorly. Like I
noted before, these tree classes span a few domains. At the very least, both the
parser and interpreter will mess with them. In a bigger language, you could also
imagine doing some other static analysis or other work on the trees. If our
language was statically typed, the type check would walk over them. Maybe do
some optimization passes.

If we added instance methods to each expression class for every one of those
operations, you'd have a bunch of different domains all smushed together. That
would violate [separation of concerns][] and lead to some really hard to
maintain classes.

[separation of concerns]: https://en.wikipedia.org/wiki/Separation_of_concerns

### The Expression Problem

This problem is more fundamental that it may at first seem. You have a handful
of types, and a handful of high level operations like "interpret". For each pair
of type and operation, you need a specific implementation. You can think of it
like a table:

**TODO: illustrate**

Rows are types, and columns are operations. Each cell represents the
implementation of that operation for that type.

An object-oriented language like Java assumes that all of the operations in one
row naturally hang together. It assume all the things you can do with one type
are likely to be related to each other, and the language makes it easy to define
them all together as methods inside the same class.

This makes it easy to extend the table by adding new rows. Simply define a new
class. No existing code has to be touched. But imagine if you want to add a new
operation -- a new column. In Java, that means modifying a lot of separate files
to define a new method on every one of those existing classes.

Languages in the functional paradigm, specifically the statically-typed ones in
the <span name="ml">ML</span> family flip that around. There, you don't have
classes and methods. Types and functions are totally distinct. To implement an
operation for a number of different types, you define a single function. In the
body of that you use *pattern matching* -- sort of a type-based switch on
steroids to implement the operation for each type all in one place.

<aside name="ml">

ML, short for "metalanguage" was created by Robin Milner and friends and forms
one of the main branches in the great programming language family. It directly
led to SML, Caml, OCaml, Haskell, and F#, and its influence is clear in Scala,
F#, Rust, and Swift.

Much like Lisp, it is one of those languages that is so full of good ideas that
language designers today are still rediscovering them over forty years later.

</aside>

This makes it trivial to add new operations -- simply define another function
that pattern matches on all of the types. But adding a new type is harder. You
have to go back and add a new case to all of the pattern matches of all of the
existing functions.

Each style has a certain "grain" to it. That's what the paradigm literally means
-- a object-oriented language wants you to *orient* your code along the rows of
types. A functional language instead encourages you to lump each column's worth
of code together.

A bunch of smart language nerds noticed that neither style made it easy to add
*both* rows and columns to the table. They called this the "expression problem"
because -- like we are here -- the example problem they used was about
expression types in an interpreter, but also because it relates to how
"expressive" a language is.

A number of interesting language features, design patterns and programming
tricks have been conceived to try to tackle that problem but no perfect language
has arisen to knock it dead. In the meantime, the best we can do is try to pick
a language whose orientation matches the natural architectural seams in the
program we're trying to write.

I find OOP works fine for many parts of our interpreter, but these tree classes
are definitely naturally in a functional style. Now we can feel ourselves
rubbing against the grain of Java. Fortunately, there's another design pattern
we can bring to help.

### The Visitor pattern

The Visitor pattern is the most widely misunderstood pattern in all of Design
Patterns, which is really saying something when you look around and realize how
many programmers *think* they know design patterns but have never <span
name="read">cracked</span> open that book once in their life.

<aside name="read">

In their defense, it is a pretty dry read.

</aside>

Part of the problem is that all of the terminology around the pattern is
confusing. The pattern isn't about "visiting" and the "accept" method isn't
exactly a clear metaphor either. Many think it has to do with traversing trees,
which isn't the case at all. We are going to use it on a set of classes that are
tree-like, but that's a total coincidence. You can use the visitor pattern just
fine on a flat set of classes.

The visitor pattern is really about approximating the functional style within an
OOP language. It's a way to let you add new columns to that table easily. It
lets you define all of the behavior for an operation on a set of types all in
one place, without having to touch the types themselves.

We don't want to stuff methods directly into each expression for each operation
we need to support, but the polymorphic dispatch Java gives for methods is a
really powerful tool. It's fast, directly supported by the language, and safely
-typed -- the compiler will yell at you if you forget to implement a method in
an interface.

Can take advantage of that without jamming the whole interpreter into the
expression classes? Yes! How? The same way we solve everything in programming,
friend, by adding a layer of indirection.

In the base Expression class, we define a *single* abstract "do stuff" method.
Each expression subclass implements that to "do stuff" in a way that's specific
to its type. But, we don't actually do the stuff right there.

Instead, the actual work is delegated to some *other* class that knows how to do
stuff. In each expression subclasses, we simply tell the delegate what kind of
expression we are. We do that by calling a different method on the delegate
specific to that type.

The delegate has a different method for each type, but they are all on the same
delegate class. That way, all of the implementations for some conceptual
operation are kept together.

The only remaining question -- aside from what the hell I'm talking about since
that was beyond vague -- is how does the expression get ahold of the delegate?
That's easy: we pass it in.

OK, if I hand-wave anymore, I'm going to go airborn, so let me try to make this
more concrete. Before we get to rolling it into our expression classes, let's
walk through a simpler example. Say we have two kinds of pastries: <span
name="beignet">beignets</span> and crullers.

<aside name="beignet">

A beignet (pronounced "ben-yay", with equal emphasis on both syllables) is a
deep-fried pastry in the same family as doughnuts. When the French colonized
North America in the 1700s, they brought beignets with them. Today, in the US,
they are most strongly associated with the cuisine of New Orleans.

The ideal way to consume them is fresh out of the fryer at Café du Monde, piled
high in powdered sugar, and washed down with a cup of café au lait while you
watch tourists squint at the sun and try to shake off their hangover from the
previous night's revelry.

</aside>

^code pastries (no location)

We want to be able to define new operations for them -- cooking them, eating
them, decorating them, etc. Here's how we do it. The "delegate" I mentioned
above is called the "visitor":

^code pastry-visitor (no location)

<aside name="overload">

In Design Patterns, both of these methods are named `visit()`, and they rely on
overloading to distinguish them. This leads some readers to think that the
correct visit method is chosen *at runtime* based on its parameter type. That
isn't the case. Unlike over*riding*, over*loading* is statically dispatched at
compile time.

Using distinct names for each method makes that more explicit, and also shows
you how to apply this pattern in languages that don't support overloading.

</aside>

To define a new operation that can be performed on pastries, we create a new
class that implements that interface. It has a concrete method for each type of
pastry. That keeps all the code for the operation across both types all nestled
snuggly together in one class.

Given some pastry, how do we route it to the correct method on the visitor based
on its type? Polymorphism to the rescue! We add this method to Pastry:

^code pastry-accept (1 before, 1 after, no location)

This "accept" method is the "do stuff" I mentioned earlier. Each subclass
overrides this method:

^code beignet-accept (1 before, 1 after, no location)

And:

^code cruller-accept (1 before, 1 after, no location)

These are the "do stuff" methods for each class. There's a little sleight of
hand here that is what trips most people up. When outside code calls `accept()`,
it calls it *on* a pastry and passes *in* the visitor. All each class's
implementation of `accept()` does swap those -- it calls `visit___()` on the
*visitor* and passes in *itself*, the pastry.

That's the heart of the trick right there. It lets us use polymorphic dispatch
on the *pastry* classes to select the appropriate method on the *visitor* class.
In the table, each pastry class is a row, but if you look at all of the methods
for a single visitor, they form a *column*.

**TODO: Illustrate.**

Now we can define as many classes as we want that all implement `PastryVisitor`.
To perform the operation, we just call `accept()` on some pastry and pass a
visitor in.

It's a really clever pattern. But it's also not a very *common* one. Personally,
I never found myself using it years of programming across multiple domains until
I started working on programming languages. Maybe that's why it's misunderstood.

### Visitors for expressions

Now that we understand the pattern, let's stuff it into our expression classes.
If we were hand-maintaining those, we'd have to manually define a visitor class
with visit methods for each type. Then we'd need to go through and add
`accept()` methods to each expression class. Fortunately, we already have our
handy script to automate this drudgery, so we'll add the visitor stuff there.

We'll also <span name="context">refine</span> the pattern a little. In the
pastry example, the visit and `accept()` methods don't return anything. In
practice, visitors often want to define operations that produce values. But what
type should we use? We can't assume every operation every visitor class defines
will return the same type, so we'll use generics to let each one choose.

<aside name="context">

Another common refinement is an additional "context" parameter that is passed to
the visit methods and then sent back through as a parameter to `accept()`. That
lets operations take an additional parameter. The visitors we'll define in the
book don't need that, so I omitted it.

</aside>

First, we'll define the visitor interface. Again we'll nest it inside the Expr
class so that we can keep everything in file:

^code call-define-visitor (2 before, 1 after)

That generates the visitor interface:

^code define-visitor

It iterates through all of the subclasses and declares a visit method for each
one. Every time we add a new expression type later, this will automatically pick
it up and add a new visit method too.

Inside the base class, we define the abstract `accept()` method:

^code base-accept-method (2 before, 1 after)

Finally, each subclass implements that and calls the right visit method for its
type:

^code accept-method (1 before, 2 after)

There we go. Now we'll be able to define operations that apply to every type of
expression without having to much with the expression classes themselves. Before
we close out this rambling chapter, let's try it out...

## A (Not Very) Pretty Printer

When we're debugging our parser and interpreter, it's often useful to look at
some chunk of parsed syntax tree and make sure it has the structure we expect.
We can try to poke around in a debugger, but that can be a chore.

Instead, we'd like some code that, given a syntax tree, produces a terse,
unambiguous string representation of it including its subexpressions. Converting
a syntax tree to a string is sort of the opposite of a parser, and is often
called "pretty printing" when the goal is to produce a string of text that is
valid syntax in the source language.

That's not our goal here. We want the string to very explicitly show the nesting
structure of the tree. A printer that returned `1 + 2 * 3` isn't super helpful
if what we're trying to debug is whether operator precedence is handled
correctly. We want to know if the `+` or `*` is at the top of the tree.

To that end, the string representation we produce isn't going to be Lox syntax.
Instead, it will look a lot like, well, Lisp. Each expression will be explicitly
parenthesized, and all of its subexpressions and tokens are contained in that.

For example, given a syntax tree like:

**TODO: illustration**

It produces:

```text
(* (- 123) (group 45.67))
```

Not exactly "pretty", but it does show the nesting and parenthesis expression
explicitly. To generate this, we define a new class:

^code ast-printer

As you can see, it implements the visitor interface. That means we need a visit
method for each of the five AST types we have so far:

^code visit-methods (2 before, 1 after)

Literal expressions are easy -- they just convert the value to a string. The
other expressions have subexpressions, so they use this `parenthesize()` helper
method:

^code print-utilities

It takes a name and a list of subexpressions and wraps them all up in
parentheses, yielding a string like:

```text
(+ 1 2)
```

Note that it calls <span name="builder">`accept()`</span> on each subexpression
and passes the AstPrinter itself to the expression. This is the recursive step
that lets us print an entire tree.

<aside name="builder">

Each call to `parenthesize()` creates a new StringBuilder including recursive
calls to subexpressions, which then flatten and return their results. A more
efficient implementation would create a single StringBuilder and thread it
through all of the recursive calls.

Since this code is only to help us debug stuff, simple and dumb is fine.

</aside>

We don't have a parser yet, so it's hard to see this in action. For now, we'll
hack together a little `main()` method that manually instantiates a tree and
prints it:

^code printer-main

If we did everything right, it prints:

```text
(* (- 123) (group 45.67))
```

You can go ahead and delete this method. We won't need. Also, as we add new
syntax tree types, I won't bother showing the necessary visit methods for them
in AstPrinter. If you want to (and you want the Java compiler to not yet at
you), go ahead and add them yourself.

This printer will come in handy in the next chapter when we start parsing Lox
code into syntax trees.

<div class="challenges">

## Challenges

1.  Earlier, I said that the `|`, `*`, and `+` we added to our grammar
    notation was just syntactic sugar. Given this grammar:

        expr → expr ( "(" ( expr ( "," expr )* )? ")" | "." IDENTIFIER )*
             | IDENTIFIER
             | NUMBER

    Produce a grammar that matches the same language but does not use any of
    that notational sugar.

1.  The visitor pattern lets you emulate the functional style where operations
    on different types are bundled together so that you can add new operations
    to existing types, but in an object-oriented language. Come up with a
    corresponding pattern that lets you bundle all of the operations on one type
    together and lets you define new types easily, but in a functional language.

    (SML or Haskell would be ideal for this exercise, but Scheme or another Lisp
    works as well.)

1.  In Reverse Polish Notation, the operands to an arithmetic operator are both
    placed before the operator, so `1 + 2` is `1 2 +` in RPN. Evaluation
    proceeds from left to right. Numbers are pushed onto an implicit stack, and
    an arithmetic operator pops the top two numbers, performs the operation,
    and pushes the result. Thus, this:

        :::lox
        (1 + 2) * (4 - 3)

    in RPN becomes:

        :::lox
        1 2 + 4 3 - *

    Define another visitor class for our syntax trees classes that takes an
    expression, converts it to RPN, and returns the resulting string.

</div>
