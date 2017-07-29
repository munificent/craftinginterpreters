^title Representing Code
^part A Tree-Walk Interpreter

> To dwellers in a wood, almost every species of tree has its voice as well as
> its feature.
> <cite>Thomas Hardy</cite>

In the [last chapter][scanning], we took the raw source code as a string and
transformed it into a slightly higher-level representation: a series of tokens.
The parser we'll write in [the next chapter][parsing] takes those tokens and
transforms them yet again to an even richer, more complex representation.

[scanning]: scanning.html
[parsing]: parsing-expressions.html

In this chapter, we'll learn about that data structure. Along the way, we'll
<span name="boring">cover</span> some theory around formal grammars, feel the
difference between functional and object-oriented programming, go over a couple
of design patterns, and do some metaprogramming.

<aside name="boring">

I was so worried about this being one of the most boring chapters in the book
that I kept stuffing more fun ideas into until I ran out of room.

</aside>

Before we do all that, let's focus on the main goal -- a representation for
code. It should be simple for the parser to produce and easy for the
interpreter to consume. If you haven't written a parser or interpreter yet,
those requirements aren't exactly illuminating. Maybe your intuition can help.
What is your brain doing when you play the part of a *human* interpreter? How do
you manually evaluate an arithmetic expression like this:

```lox
1 + 2 * 3 - 4
```

Because you understand the rules of precedence -- the old "Please Excuse My Dear
Aunt Sally" stuff -- you know that the `*` is evaluated before the `+` or `-`.
One way to visualize that precedence is using a tree. Leaf nodes are numbers, and interior nodes are operators with branches for each of their operands.

In order to evaluate an arithmetic node, you need to know the numeric values of
its subtrees, so you have to evaluate those first. That means working your way
from the leaves up to the root -- a *post-order* traversal:

<span name="tree-steps"></span>

<img src="image/representing-code/tree-evaluate.png" alt="Evaluating the tree from the bottom up." />

<aside name="tree-steps">

A. Starting with the full tree, evaluate the bottom-most operation, `2 * 3`.

B. Now we can evaluate the `+`.

C. Finally the `-`.

D. The final answer.

</aside>

If I gave you an arithmetic expression, you could draw one of these trees pretty
easily. Given a tree, you can evaluate it without breaking a sweat. So it
intuitively seems like a workable representation of our code is a <span
name="only">tree</span> that matches the grammatical structure of the language.

<aside name="only">

That's not to say a tree is the *only* possible representation of our code. In
[Part III][], we'll generate bytecode, another representation that isn't as
human friendly but is closer to the machine.

[part iii]: a-bytecode-virtual-machine.html

</aside>

We need to get more precise about what that grammar is then. Like lexical
grammars in the last chapter, there is a long ton of theory around syntactic
grammars. We're going into that theory a little more than we did when scanning
because it turns out to be a useful tool throughout much of the interpreter.
We start by moving one level up the [Chomsky hierarchy][]...

[chomsky hierarchy]: https://en.wikipedia.org/wiki/Chomsky_hierarchy

## Context-Free Grammars

In the last chapter, the formalism we used for defining the lexical grammar --
the rules for how characters get grouped into tokens -- was called a *regular
language*. That was fine for our scanner, which emits a flat sequence of tokens.
But regular languages aren't powerful enough to handle expressions which can
nest arbitrarily deeply.

We need a bigger hammer, and that hammer is a **context-free grammar**
(**CFG**). It's the next heaviest tool in the toolbox of **[formal
grammars][]**. A formal grammar takes a set of atomic pieces it calls its
"alphabet". Then it defines a (usually infinite) set of "strings" that are "in"
the grammar. Each string is a sequence of "letters" in the alphabet.

[formal grammars]: https://en.wikipedia.org/wiki/Formal_grammar

I'm using all those quotes because the terms get a little confusing as you move
from lexical to syntactic grammars. In our scanner's grammar, the alphabet
consists of individual characters and the strings are the valid lexemes, roughly
"words". In the syntactic grammar we're talking about now, we're at a different
level of granularity. Now each "letter" in the alphabet is an entire token and a
"string" is a sequence of *tokens* -- an entire expression.

Oof. Maybe a table will help:

<table>
<thead>
<tr>
  <td>Context-free grammar</td>
  <td></td>
  <td>Lexical grammar</td>
  <td>Syntactic grammar</td>
</tr>
</thead>
<tbody>
<tr>
  <td>The &ldquo;alphabet&rdquo; is&hellip;</td>
  <td>&rarr;&ensp;</td>
  <td>Characters</td>
  <td>Tokens</td>
</tr>
<tr>
  <td>A &ldquo;string&rdquo; is&hellip;</td>
  <td>&rarr;&ensp;</td>
  <td>Lexeme or token</td>
  <td>Expression</td>
</tr>
<tr>
  <td>It&apos;s implemented by the&hellip;</td>
  <td>&rarr;&ensp;</td>
  <td>Scanner</td>
  <td>Parser</td>
</tr>
</tbody>
</table>

A formal grammar's job is to specify which strings are valid and which aren't.
If we were defining a grammar for English sentences, "eggs are tasty for
breakfast" would be in the grammar, but "tasty breakfast for are eggs" would
probably not be.

### Rules for grammars

How do we write down a grammar that contains an infinite number of valid
strings? We obviously can't list them all out. Instead, we create a finite set
of *rules*. You can think of them as a game that you can "play" in one of two
directions.

If you start with the rules, you can use them to *generate* strings that are in
the grammar. Strings created this way are called **derivations** because each is
"derived" from the rules of the grammar.

In each step of the game, you pick a rule and follow what it tells you to do.
Most of the lingo around formal grammars comes from playing them in this
direction. Rules are called **productions** because they *produce* strings in
the grammar.

Each production in a context-free grammar has a **head** -- its <span
name="name">name</span> -- and a **body** which describes what it generates. In
its pure form the body is simply a list of symbols. Symbols come in two
delectable flavors:

<aside name="name">

Restricting heads to a single symbol is a defining feature of context-free
grammars. More powerful formalisms like **[unrestricted grammars][]** allow a
sequence of symbols in the head as well as in the body.

[unrestricted grammars]: https://en.wikipedia.org/wiki/Unrestricted_grammar

</aside>

*   A **terminal** is a letter from the grammar's alphabet. You can think of it
    like a literal value. In the syntactic grammar we're defining, the terminals
    are individual lexemes -- tokens coming from the scanner like `if` or
    `1234`.

    These are called "terminals", in the sense of an "end point" because they
    don't lead to any further "moves" in the game. You simply produce that one
    symbol.

*   A **nonterminal** is a named reference to another rule in the grammar. It
    means "play that rule and insert whatever it produces here". In this way,
    the grammar composes.

There is one last refinement: you may have multiple rules with the same name.
When you reach a nonterminal with that name, you are allowed to pick any of the
rules for it, whichever floats your boat.

To make this concrete, we need a <span name="turtles">way</span> to write down
these production rules. Ever since John Backus snuck into Noam Chomsky's
linguistics class and stole some theory to use for specifying ALGOL 58,
programmers have been inventing notations for CFGs. For some reason, virtually
every one of them tweaks the metasyntax in one way or another.

I tried to come up with something clean. Each rule is a name, followed by an
arrow (`→`), followed by its sequence of symbols. Terminals are quoted strings,
and nonterminals are lowercase words.

<aside name="turtles">

Yes, we need to define a syntax to use for the rules that define our syntax.
Should we specify that *metasyntax* too? What notation do we use for *it?* It's
languages all the way down!

</aside>

Using that, here's a grammar for <span name="breakfast">breakfast</span> menus:

<aside name="breakfast">

Yes, I really am going to be using breakfast examples throughout this entire
book. Sorry.

</aside>

```lox
breakfast → protein "with" bread ;
breakfast → protein ;
breakfast → bread ;

protein   → protein "and" protein ;
protein   → "bacon" ;
protein   → "sausage" ;
protein   → cooked "eggs" ;

cooked    → "scrambled" ;
cooked    → "poached" ;
cooked    → "fried" ;

bread     → "toast" ;
bread     → "biscuits" ;
bread     → "English muffin" ;
```

We can use that grammar to generate random breakfasts. Let's play a round and
see how it works. By age-old convention, the game starts with the first rule in
the grammar, here `breakfast`. There are three productions for that, and we
randomly pick the first one. Our resulting string looks like:

```text
protein "with" bread
```

We need to expand that first nonterminal, `protein`, so we pick a production for
that. Let's pick:

```lox
protein → protein "and" protein ;
```

Note that the production refers to its own rule. This is the key difference
between context-free and regular languages. The former are allowed to recurse.
It is exactly this that lets them nest and compose.

We could keep picking the first production for `protein` over and over again
yielding all manner of breakfasts like "bacon and sausage and sausage and bacon
and...". We won't though. We need to again pick a production for `protein` in
the inner reference to `protein "and" protein`. This time we'll pick `"bacon"`.
We finally hit a terminal, so we set that as the first word in the resulting
string.

Now we pop back out to the first `protein "and" protein`. The next symbol is
`"and"`, a terminal, so we add that. Then we hit another `protein`. This
time, we pick:

```lox
protein → cooked "eggs" ;
```

We need a production for `cooked` and pick `"poached"`. That's a terminal, so
we add that. Now we're back to the `protein`, so we add `"eggs"`. We bounce back
to `breakfast` and add `"with"`. Now all that's left is to pick a production for
`bread`. We'll pick `"English muffin"`. That's again a terminal, so we add
that and we're done:

<img src="image/representing-code/breakfast.png" alt='"Playing" the grammar to generate a string.' />

Any time we hit a rule that had multiple productions, we just picked one
arbitrarily. It is this flexibility that allows a short number of grammar rules
to encode a combinatorially larger set of strings. The fact that a rule can
refer to itself -- directly or indirectly -- kicks it up even more, letting us
pack an infinite number of strings into a finite grammar.

### Enhancing our notation

Stuffing an infinite set of strings in a handful of rules is pretty fantastic,
but let's take it farther. Our notation works, but it's a little tedious. So,
like any good language designer, we'll sprinkle some syntactic sugar on top. In
addition to terminals and nonterminals, we'll allow a few other kinds of
expressions in the body of a rule:

*   Instead of repeating the rule name each time we want to add another
    production for it, we'll allow a series of productions separated by `|`:

        :::lox
        bread → "toast" | "biscuits" | "English muffin" ;

*   Further, we'll allow `(` and `)` for grouping and then allow `|` within that
    to select one from a series of options within the middle of a production:

        :::lox
        protein → ( "scrambled" | "poached" | "fried" ) "eggs" ;

*   Using <span name="recursion">recursion</span> to support repeated sequences
    of symbols has a certain appealing <span name="purity">purity</span>, but
    it's kind of a chore to make a separate named sub-rule each time we want to
    loop. Instead, we allow a postfix `*` to mean the previous symbol or group
    may be repeated zero or more times.

        :::lox
        protein → protein ( "and" protein )* ;

<aside name="purity">

This is how the Scheme programming language works. It has no built-in looping
functionality at all. Instead, *all* repetition is expressed in terms of
recursion.

</aside>

*   A postfix `+` is similar, but requires the preceding production to appear
    at least once.

*   A postfix `?` is for an optional production. The thing before it can appear
    zero or one time, but not more.

        :::lox
        breakfast → protein ( "with" bread )? ;

With all of that sugar, our breakfast grammar condenses down to:

```lox
breakfast → protein ( "and" protein )* ( "with" bread )?
          | bread ;

protein   → "bacon"
          | "sausage"
          | ( "scrambled" | "poached" | "fried" ) "eggs" ;

bread     → "toast" | "biscuits" | "English muffin" ;
```

Not too bad, I hope. If you're used to grep or using [regular
expressions][regex] in your text editor, most of the punctuation should be
familiar. The main difference is that symbols here represent entire words, not
single characters.

[regex]: https://en.wikipedia.org/wiki/Regular_expression#Standards

We'll use this notation throughout the rest of the book to precisely describe
Lox's grammar. As you work on programming languages, you'll find context-free
grammars (using this or [EBNF][] or some other notation) help you crystallize
your informal syntax design ideas. They are also a handy medium for
communicating with other language hackers about syntax.

[ebnf]: https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form

The rules and productions we define for Lox are also our guide to the tree data
structure we're going to implement to represent code in memory. Before we can do
that, we need an actual grammar for Lox, or at least enough of it for us to get
started.

### A Grammar for Lox expressions

In the previous chapter, we did Lox's entire lexical grammar in one fell swoop.
Every keyword and bit of punctuation is there. The syntactic grammar is larger,
and it would be a real bore to grind through the entire thing before we actually
get our interpreter up and running.

Instead, we'll crank through a subset of the language in the next couple of
chapters. Once we have that minilanguage represented, parsed, and interpreted,
later chapters will progressively add new features to it, including the new
syntax. For now, we are only going to worry about a handful of expressions:

*   **Literals.** Numbers, strings, Booleans, and `nil`.

*   **Unary expressions.** A prefix `!` to perform a logical not, and `-`
    to negate a number.

*   **Binary expressions.** The infix arithmetic (`+`, `-`, `*`, `/`) and logic
    (`==`, `!=`, `<`, `<=`, `>`, `>=`) operators we know and love.

*   **Parentheses for grouping.**

That gives us enough syntax for expressions like:

```lox
1 - (2 * 3) < 4 == false
```

Using our handy dandy new notation, here's a grammar for those:

```lox
expression → literal
           | unary
           | binary
           | grouping ;

literal    → NUMBER | STRING | "true" | "false" | "nil" ;
grouping   → "(" expression ")" ;
unary      → ( "-" | "!" ) expression ;
binary     → expression operator expression ;
operator   → "==" | "!=" | "<" | "<=" | ">" | ">="
           | "+"  | "-"  | "*" | "/" ;
```

There's one bit of extra <span name="play">metasyntax</span> here. In addition
to quoted strings for terminals that match exact lexemes, we `CAPITALIZE`
terminals that are a single lexeme whose text representation may vary. `NUMBER`
is any number literal, and `STRING` is any string literal. Later, we'll do the
same for `IDENTIFIER`.

This grammar is actually ambiguous, which we'll see when we get to parsing it.
But it's good enough for now.

<aside name="play">

If you're so inclined, try using this grammar to generate a few expressions like
we did with the breakfast grammar before. Do the resulting expressions look
right to you? Can you make it generate anything wrong like `1 + / 3`?

</aside>

## Implementing Syntax Trees

Finally, we get to write some code. That little expression grammar is our
skeleton. Since the grammar is recursive -- note how `grouping`, `unary`, and
`binary` all refer back to `expression`, our data structure will form a tree.
Since this structure represents the syntax of our language, it's called a <span
name="ast">**"syntax tree"**</span>.

<aside name="ast">

In particular, we're defining an ***abstract* syntax tree** (**AST**). In a
**parse tree**, every single grammar production becomes a node in the tree. An
AST elides productions that aren't needed by later phases.

</aside>

Our scanner used a single Token class to represent all kinds of lexemes. To
distinguish the different kinds -- think the number `123` versus the string
`"123"` -- we included a simple TokenType enum.

Syntax trees are not so <span name="token-data">homogenous</span>. Unary
expressions have a single operand, binary expressions have two, and literals
have none. We *could* mush that all together into a single Expression class with
an arbitrary list of children. Some compilers do.

<aside name="token-data">

Tokens aren't entirely homogenous either. Tokens for literals store the value
but other kinds of lexemes don't need that state. I have seen scanners that use
different classes for literals and other kinds of lexemes, but I figured I'd
keep things simpler.

</aside>

But I like getting the most out of Java's type system. So we'll define a base
class for expressions. Then, for each kind of expression -- each production
under `expression` -- we create a subclass that has fields for the nonterminals
specific to that rule. This way, we get a compile error if we, say, try to
access the second operand of a unary expression.

Something like:

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

I avoid abbreviations in my code because they trip up a reader who doesn't know
what they stand for. But in compilers I've looked at, "Expr" and "Stmt" are so
ubiquitous that I may as well start getting you used to them now.

</aside>

Expr is the base class that all expression classes inherit from. I went ahead
and nested the subclasses inside of it. There's no real need for this, but it
lets us cram all of the classes into a single file.

### Disoriented objects

You'll note that, much like the Token class, there aren't any methods here. It's
a dumb structure. Nicely typed, but merely a bag of data. This feels strange in
an object-oriented language like Java. Shouldn't the class *do stuff?*

The problem is that these tree classes aren't owned by any single domain. Should
they have methods for parsing since that's where the trees are created? Or
interpreting since that's where they are consumed? Trees span the border between
those territories, which mean they are really owned by *neither.*

In fact, these types exist to enable the parser and interpreter to
*communicate*. That lends itself to types that are simply data with no
associated behavior. This style is very natural in functional languages like
Lisp and ML where *all* data is separate from behavior, but it feels odd in
Java.

Functional programming aficionados right now are jumping up to exclaim "See!
Object-oriented languages are a bad fit for an interpreter!" I won't go that
far. You'll recall that the scanner itself was admirably suited to
object-orientation. It had all of the mutable state to keep track of where it
was in the source code, a well-defined set of public methods, and a handful of
private helper ones.

My feeling is that each phase or part of the interpreter works fine in an
object-oriented style. It is the data structures that flow between them that are
stripped of behavior.

### Metaprogramming the trees

Java can express behavior-less classes, but I wouldn't say that's particularly
great at it. Eleven lines of code to stuff three fields in an object is pretty
tedious, and when we're all done, we're going to have 21 of these classes.

I don't want to waste your time or my ink writing all that down. Really, what is
the essence of each subclass? A name, and a list of typed fields. That's it.
We're smart language hackers, right? Let's <span
name="automate">automate</span>.

<aside name="automate">

Picture me doing an awkward robot dance when you read that. "AU-TO-MATE."

</aside>

Instead of tediously hand-writing each class definion, field declaration,
constructor, and initializer, we'll hack together a <span
name="python">script</span> that does it for us. It has a description of each
tree type -- its name and fields -- and it prints out the Java code needed to
define a class with that name and state.

This script is a tiny Java command-line app that generates a file named
"Expr.java".

<aside name="python">

I got the idea of scripting the syntax tree classes from Jim Hugunin, creator of
Jython and IronPython.

An actual scripting language would be a better fit for this than Java, but I'm
trying not to throw too many languages at you.

</aside>

^code generate-ast

Note that this file is in a different package, `.tool` instead of `.lox`. This
script isn't part of the interpreter itself. It's a tool *we*, the people
hacking on the interpreter, run ourselves to generate the syntax tree classes.
When it's done, we treat "Expr.java" like any other file in the implementation.
We are merely automating how that file gets authored.

To generate the classes, it needs to have some description of each type and its
fields:

^code call-define-ast (1 before, 1 after)

For brevity's sake, I jammed the description of each type into a string. Each is
the name of the class followed by `:` and the list of fields, separated by
commas. Each field has a type and name.

The first thing `defineAst()` needs to do is output the base Expr class:

^code define-ast

When we call this, `baseName` is "Expr", which is both the name of the class and
the name of the file it outputs. We pass this in instead of hardcoding it
because we'll add a separate family of classes later for statements.

Inside Expr, we define each subclass:

^code nested-classes (2 before, 1 after)

<aside name="robust">

This isn't the world's most elegant string munging code, but that's fine. It
only runs on the exact set of class definitions we give it. Robustness ain't a
priority.

</aside>

That code in turn calls:

^code define-type

There we go. All of that glorious Java boilerplate is done. It declares each
field in the class body. It defines a constructor for the class with parameters
for each field and initializes them in the body.

Run this script now and it blasts out a few dozen lines of code. That's about to
get even longer.

## Working with Trees

Put on your imagination hat for a moment. Even though we aren't there yet,
consider what the interpreter will do with the syntax trees. It needs to select
a different chunk of code to handle each kind of expression. With tokens, we can
simply switch on the TokenType. But we don't have a "type" enum for the syntax
trees, just a separate Java class for each one.

We could write some long chain of type tests:

```java
if (expr instanceof Expr.Binary) {
  // ...
} else if (expr instanceof Expr.Grouping) {
  // ...
} else // ...
```

That's verbose and slow. Also, the Java compiler won't tell us when we forget to
add support for some new expression class. With an enum, we get a compile
error when a switch is missing a case.

We have a family of classes and we need to associate a chunk of behavior with
each one. The natural solution in an object-oriented language like Java is to
put that behavior into methods on the classes themselves. We could add an
abstract <span name="interpreter-pattern">`interpret()`</span> method on Expr
which each subclass then implements to interpret itself.

<aside name="interpreter-pattern">

This exact thing is literally called the ["Interpreter pattern"][interpreter
pattern] in "Design Patterns: Elements of Reusable Object-Oriented Software".

[interpreter pattern]: https://en.wikipedia.org/wiki/Interpreter_pattern

</aside>

This works alright for tiny projects, but it scales poorly. Like I noted before,
these tree classes span a few domains. At the very least, both the parser and
interpreter will mess with them. As [you'll see later][resolution], we need to
do name resolution on them. If our language was statically typed, we'd have a
type checking pass.

[resolution]: resolving-and-binding.html

If we added instance methods to the expression classes for every one of those
operations, that would smush a bunch of different domains together. That
violates [separation of concerns][] and leads to hard to maintain code.

[separation of concerns]: https://en.wikipedia.org/wiki/Separation_of_concerns

### The Expression Problem

This problem is more fundamental than it may at first seem. We have a handful of
types, and a handful of high level operations like "interpret". For each pair of
type and operation, we need a specific implementation. Picture a table:

<img src="image/representing-code/table.png" alt="A table where rows are labeled with expression classes and columns are function names." />

Rows are types, and columns are operations. Each cell represents the
implementation of that operation for that type.

An object-oriented language like Java assumes that all of the code in one row
naturally hangs together. It figures all the things you do with a type are
likely related to each other, and the language makes it easy to define them
together as methods inside the same class.

This makes it easy to extend the table by adding new rows. Simply define a new
class. No existing code has to be touched.

<img src="image/representing-code/rows.png" alt="The table split into rows for each class." />

But imagine if you want to add a new *operation* -- a new column. In Java, that
means cracking open each of those existing classes and adding a method to it.

Functional paradigm languages in the <span name="ml">ML</span> family flip that
around. There, you don't have classes with methods. Types and functions are
totally distinct. To implement an operation for a number of different types, you
define a single function. In the body of that you use *pattern matching* -- sort
of a type-based switch on steroids -- to implement the operation for each type
all in one place.

<aside name="ml">

ML, short for "metalanguage" was created by Robin Milner and friends and forms
one of the main branches in the great programming language family. Its children
include SML, Caml, OCaml, Haskell, and F#. Even Scala, F#, Rust, and Swift
bear a strong resemblance.

Much like Lisp, it is one of those languages that is so full of good ideas that
language designers today are still rediscovering them over forty years later.

</aside>

This makes it trivial to add new operations -- simply define another function
that pattern matches on all of the types.

<img src="image/representing-code/columns.png" alt="The table split into columns for each function." />

But, conversely, adding a new type is hard. You have to go back and add a new
case to all of the pattern matches in all of the existing functions.

Each style has a certain "grain" to it. That's what the paradigm literally means
-- an object-oriented language wants you to *orient* your code along the rows of
types. A functional language instead encourages you to lump each column's worth
of code together into *functions*.

A bunch of smart language nerds noticed that neither style made it easy to add
*both* rows and columns to the <span name="multi">table</span>. They called this
the "expression problem" because -- like we are here -- the example problem they
used was about expression types in an interpreter, but also because it relates
to how "expressive" a language is.

<aside name="multi">

Languages with *multimethods*, like Common Lisp's CLOS, Dylan, and Julia do
support adding both new types and operations easily. What they typically
sacrifice is either static type checking, or separate compilation.

</aside>

People have thrown all sorts of language features, design patterns and
programming tricks to try to knock that problem down but no perfect language has
finished it off yet. In the meantime, the best we can do is try to pick a
language whose orientation matches the natural architectural seams in the
program we're writing.

Object-orientation works fine for many parts of our interpreter, but these tree
classes rub against the grain of Java. Fortunately, there's a design pattern we
can bring to bear on it.

### The Visitor pattern

The **Visitor pattern** is the most widely misunderstood pattern in all of
Design Patterns, which is really saying something when you look at the software
architecture excesses of the past couple of decades.

The trouble starts with terminology. The pattern isn't about "visiting" and the
"accept" method in it doesn't conjure up any helpful imagery either. Many think
the pattern has to do with traversing trees, which isn't the case at all. We
*are* going to use it on a set of classes that are tree-like, but that's a
coincidence. As you'll see, the pattern works as well on a single object.

The Visitor pattern is really about approximating the functional style within an
OOP language. It lets us add new columns to that table easily. We can define all
of the behavior for a new operation on a set of types in one place, without
having to touch the types themselves. It does this the same way we solve almost
every problem in computer science: by adding a layer of indirection.

Before we apply it to our auto-generated Expr classes, we'll walk through a
simpler example. Say we have two kinds of pastries: <span
name="beignet">beignets</span> and crullers.

<aside name="beignet">

A beignet (pronounced "ben-yay", with equal emphasis on both syllables) is a
deep-fried pastry in the same family as doughnuts. When the French colonized
North America in the 1700s, they brought beignets with them. Today, in the US,
they are most strongly associated with the cuisine of New Orleans.

My preferred way to consume them is fresh out of the fryer at Café du Monde,
piled high in powdered sugar, and washed down with a cup of café au lait while I
watch tourists staggering around trying to shake off their hangover from the
previous night's revelry.

</aside>

^code pastries (no location)

We want to be able to define new operations for them -- cooking them, eating
them, decorating them, etc. -- without having to add a new method to each class
every time. Here's how we do it. First, we define a separate interface:

^code pastry-visitor (no location)

<aside name="overload">

In Design Patterns, both of these methods are confusingly named `visit()`, and
they rely on overloading to distinguish them. This leads some readers to think
that the correct visit method is chosen *at runtime* based on its parameter
type. That isn't the case. Unlike over*riding*, over*loading* is statically
dispatched at compile time.

Using distinct names for each method makes that more explicit, and also shows
you how to apply this pattern in languages that don't support overloading.

</aside>

To define a new operation that can be performed on pastries, we create a new
class that implements that interface. It has a concrete method for each type of
pastry. That keeps the code for the operation on both types all nestled snuggly
together in one class.

Given some pastry, how do we route it to the correct method on the visitor based
on its type? Polymorphism to the rescue! We add this method to Pastry:

^code pastry-accept (1 before, 1 after, no location)

Each subclass implements it:

^code beignet-accept (1 before, 1 after, no location)

And:

^code cruller-accept (1 before, 1 after, no location)

To perform an operation on a pastry, we call its `accept()` method and pass in
the visitor for the operation we want to execute. The pastry -- the specific
subclass's implementation of `accept()` -- turns around and calls the
appropriate visit method on the visitor and passes *itself* to it.

That's the heart of the trick right there. It lets us use polymorphic dispatch
on the *pastry* classes to select the appropriate method on the *visitor* class.
In the table, each pastry class is a row, but if you look at all of the methods
for a single visitor, they form a *column*.

<img src="image/representing-code/visitor.png" alt="Now all of the cells for one operation are part of the same class, the visitor." />

We added one `accept()` method to each class, and we can use it for as many
visitors as we want without ever having to touch the pastry classes again. It's
a clever pattern.

### Visitors for expressions

OK, let's weave it into our expression classes. We'll also <span
name="context">refine</span> the pattern a little. In the pastry example, the
visit and `accept()` methods don't return anything. In practice, visitors often
want to define operations that produce values. But what return type should
`accept()` have? We can't assume every visitor wants to produce the same type,
so we'll use generics to let each one pick.

<aside name="context">

Another common refinement is an additional "context" parameter that is passed to
the visit methods and then sent back through as a parameter to `accept()`. That
lets operations take an additional parameter. The visitors we'll define in the
book don't need that, so I omitted it.

</aside>

First, we define the visitor interface. Again, we nest it inside the Expr class
so that we can keep everything in one file:

^code call-define-visitor (2 before, 1 after)

That generates the visitor interface:

^code define-visitor

It iterates through all of the subclasses and declares a visit method for each
one. When we define new expression types later, this will automatically include
them.

Inside the base class, we define the abstract `accept()` method:

^code base-accept-method (2 before, 1 after)

Finally, each subclass implements that and calls the right visit method for its
own type:

^code accept-method (1 before, 2 after)

There we go. Now we can define operations on expressions without having to muck
with the classes or our generator script. Before we end this rambling chapter,
let's try it out...

## A (Not Very) Pretty Printer

When we debug our parser and interpreter, it's often useful to look at a parsed
syntax tree and make sure it has the structure we expect. We could inspect it in
the debugger, but that can be a chore.

Instead, we'd like some code that, given a syntax tree, produces an unambiguous
string representation of it. Converting a tree to a string is sort of the
opposite of a parser, and is often called "pretty printing" when the goal is to
produce a string of text that is valid syntax in the source language.

That's not our goal here. We want the string to very explicitly show the nesting
structure of the tree. A printer that returned `1 + 2 * 3` isn't super helpful
if what we're trying to debug is whether operator precedence is handled
correctly. We want to know if the `+` or `*` is at the top of the tree.

To that end, the string representation we produce isn't going to be Lox syntax.
Instead, it will look a lot like, well, Lisp. Each expression is explicitly
parenthesized, and all of its subexpressions and tokens are contained in that.

Given a syntax tree like:

<img src="image/representing-code/expression.png" alt="An example syntax tree." />

It produces:

```text
(* (- 123) (group 45.67))
```

Not exactly "pretty", but it does show the nesting and grouping explicitly. To
implement this, we define a new class:

^code ast-printer

As you can see, it implements the visitor interface. That means we need visit
methods for each of the expression types we have so far:

^code visit-methods (2 before, 1 after)

Literal expressions are easy -- they convert the value to a string with a little
check to handle Java's `null` standing in for Lox's `nil`. The other expressions
have subexpressions, so they use this `parenthesize()` helper method:

^code print-utilities

It takes a name and a list of subexpressions and wraps them all up in
parentheses, yielding a string like:

```text
(+ 1 2)
```

Note that it calls `accept()` on each subexpression and passes in itself. This
is the <span name="tree">recursive</span> step that lets us print an entire
tree.

<aside name="tree">

This recursion is also why people think the Visitor pattern itself has to do
with trees.

</aside>

We don't have a parser yet, so it's hard to see this in action. For now, we'll
hack together a little `main()` method that manually instantiates a tree and
prints it:

^code printer-main

If we did everything right, it prints:

```text
(* (- 123) (group 45.67))
```

You can go ahead and delete this method. We won't need it. Also, as we add new
syntax tree types, I won't bother showing the necessary visit methods for them
in AstPrinter. If you want to (and you want the Java compiler to not yell at
you), go ahead and add them yourself. It will come in handy in the next chapter
when we start parsing Lox code into syntax trees.

<div class="challenges">

## Challenges

1.  Earlier, I said that the `|`, `*`, and `+` forms we added to our grammar
    metasyntax were just syntactic sugar. Given this grammar:

        expr → expr ( "(" ( expr ( "," expr )* )? ")" | "." IDENTIFIER )*
             | IDENTIFIER
             | NUMBER

    Produce a grammar that matches the same language but does not use any of
    that notational sugar.

    Bonus: What kind of expression does this bit of grammar encode?

1.  The Visitor pattern lets you emulate the functional style in an
    object-oriented language. Devise a corresponding pattern in a functional
    language. It should let you bundle all of the operations on one type
    together and let you define new types easily.

    (SML or Haskell would be ideal for this exercise, but Scheme or another Lisp
    works as well.)

1.  In [Reverse Polish Notation][rpn] (RPN), the operands to an arithmetic
    operator are both placed before the operator, so `1 + 2` becomes `1 2 +`.
    Evaluation proceeds from left to right. Numbers are pushed onto an implicit
    stack. An arithmetic operator pops the top two numbers, performs the
    operation, and pushes the result. Thus, this:

        :::lox
        (1 + 2) * (4 - 3)

    in RPN becomes:

        :::lox
        1 2 + 4 3 - *

    Define a visitor class for our syntax tree classes that takes an expression,
    converts it to RPN, and returns the resulting string.

[rpn]: https://en.wikipedia.org/wiki/Reverse_Polish_notation

</div>
