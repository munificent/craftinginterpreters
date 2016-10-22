^title Introduction
^part Welcome

> No, no! The adventures first, explanations take such a dreadful time.
>
> <cite>Lewis Carroll</cite>

I'm really excited we're going on this journey together. This is a book on
implementing interpreters for programming languages. It's also a book on how to
design a language worth implementing. It's the book I wished I had when I first
started getting into languages, and it's the book I've been writing in my <span
name="head">head</span> for nearly a decade.

<aside name="head">

To my friends and family, sorry I seemed so distracted!

</aside>

In these pages, we will walk step by step through two complete interpreters for
a full-featured language. I assume this is your first foray into languages, so
I'll cover each concept and piece of code you'll need to understand to get to a
complete, usable, fast language implementation.

That's a good bit of distance to cover, so my plan is to take you on a carefully
charted path. To save time, we won't wander far off that path, but there's
plenty of interesting stuff out there. I'll point it out in passing and as you
get more confident, I encourage you to venture out and explore them yourself.

We also won't focus on theory as much as other books do. As we build each piece
of the system, I will introduce the history and concepts behind it. I'll try to
get you familiar with the terminology so that if you ever inadvertantly find
yourself in a cocktail <span name="party">party</span> full of PL (programming
language) researchers, you'll fit right in.

<aside name="party">

Believe it or not, a bizarre situation I have in fact found myself in multiple
times. Some of those people can drink like you wouldn't believe.

</aside>

But we're mostly going to spend our brain juice getting our language up and
running. This is not to say theory isn't important. Being able to reason
precisely and <span name="formal">formally</span> about syntax and semantics is
a vital skill when working on a language. But, personally, I learn best by
doing. It's hard for me to wade through paragraphs full of abstract concepts and
feel I've really absorbed them. But if I've implemented a concept, debugged it,
ran it, and poked at it, then I *get* it.

<aside name="formal">

Static type systems in particular require very strong skill in formal reasoning
and logic. Hacking on a type system has the same feel as proving a theorem in
mathematics. As it turns out, this is no coincidence. In the early half of last
century, Haskell Curry and William Alvin Howard showed that the two are exactly
the same: [the Curry-Howard isomorphism][].

[the curry-howard isomorphism]: https://en.wikipedia.org/wiki/Curry%E2%80%93Howard_correspondence

</aside>

That's my goal for you. I want you to come away with a solid intuition of how a
real language lives and breathes. My hope is this will form the foundation that
you can use to build a more formal understanding on from reading other books.

## Why learn this stuff?

Every introduction to every language book seems to have this section. I don't
know what it is about programming languages that seems to cause this existential
angst. Perhaps we feel presumptious about creating something that may change the
way others express their ideas. I don't think ornithology books worry about
justifying their existence. They tacitly assume the reader loves birds and move
on.

But programming languages are a little different. I suppose it's true that the
odds of any of us creating a broadly successful general purpose programming
language are slim. The designers of all of the world's successful languages
could fit in a Volkswagen bus, even without putting the pop top camper up. So if
joining that elite group was the only reason to learn about languages, it would
be hard to justify. Fortunately, it isn't.

### Little languages are everywhere

For every successful *general purpose* language out there, there are a thousand
successful niche ones. We used each one a "little language", but today the name
on their trumped-up business card reads "domain-specific language". These are
mini-languages, tailor-built to a specific task. Things like
application-specific scripting languages, template engines, markup formats, and
configuration files.

There's a very good chance in your career that you'll find yourself needing one
of these. When you can, it's good to reuse an existing one instead of rolling
your own. Once you take into account the need for documentation, debuggers,
editor support, syntax highlighting, and all of the other tooling, doing it
yourself can be a lot of work.

But there's still a real chance you'll find yourself needing to whip up a parser
or something when there isn't an existing library that fits your needs. Even
when there *is* one you can reuse, you'll inevitably end up needing to debug and
maintain it and poke around in its guts.

### Languages are great exercise

When long distance runners train, they sometimes run with weights strapped to
their ankles. This artificial handicap pushes them and when they take off the
weights later, they can run farther and faster.

An efficient language implementation is a real test of programming skill. They
are complex and performance critical. They heavily rely on recursion. They use
data structures like dynamic arrays, trees, graphs, and hash tables.

You probably use those in your day-to-day programming, especially hash tables,
but how well do you *really* understand them? Well, after we've crafted our own
from scratch, I can promise you will.

While I hope to teach you that a programming language isn't as daunting as you
might believe, it still a challenge. If you are up to the task, I think you'll
come away a stronger programmer, and smarter about how you use data structures
and algorithms in your day job.

### One more reason

This last reason is a little embarrassing to admit. It's sort of the secret goal
of this book. When I first learned to program as a kid, I couldn't conceive of
how a language itself worked. How did they write BASIC before they had BASIC? It
seemed like magic to me.

The problem with magic is that it inherently excludes people. A magician's
illusion only works when the audience doesn't know the secret. The difference
between a wizard and everyone else is that, to the wizard, *it's not magic*.

If you think of programming languages as a black art known only to certain
arcane practitioners, you are implicitly excluding yourself from that club. My
aim is to show you that there is no magic here.

There are a few techniques you don't often encounter outside of languages, and
some parts are a little difficult. But not more difficult than other engineering
problems you've tackled. It's the divide between normal programmers and
programming language hackers that is the illusion. Code is just code and people
are just people.

If I can get you to punch through that illusory wall, maybe some of the others
that hold you back will look a little ephemeral too. And, who knows, maybe you
*will* make the next great language. Someone has to.

## How the book is organized

This book is broken into three parts. You're reading the first one now. It's a
couple of chapters to get you oriented, teach you some of the lingo language
hackers use, and introduce you to Lox, the language we'll be implementing.

Each of the other two parts builds one complete Lox interpreter. Each
interpreter is built one chapter at a time. It took a good bit of trial and
error on my part, but I managed to carve up each interpreter into chapter-sized
chunks so that each chapter's part builds on the previous ones. At the end of
each chapter, you have an increasingly full-featured interpreter that you can
run an play with.

### Each chapter

Aside from the couple of introductory chapters in this part which are special,
each chapter in the book is structured the same. It takes a single feature
needed in a programming language and teaches you the concepts behind it. At the
same time, it contains every single line of code needed to implement it.

(What this book doesn't contain is the machinery needed to compile and run the
code. I assume you can slap together a makefile or a project in your IDE of
choice in order to get the code to run.)

Many other language books and language implementations use tools like [Lex][]
and <span name="yacc">[Yacc][]</span>, "compiler-compilers" to automatically
generate some of the source files for an implementation from some higher level
description.

<aside name="yacc">

Yacc is a tool that takes in a grammar file and produces a source file for a
compiler, so it's sort of like a "compiler" that outputs a compiler. Hence the
name, "compiler-compiler".

It wasn't the first of its ilk, which is why it's named "Yacc"—Yet Another
Compiler-Compiler. A later similar tool is [Bison][], named as a pun on the
pronunciation of Yacc like "yak".

[bison]: https://en.wikipedia.org/wiki/GNU_bison

If you find all of these little self-references and puns charming and fun,
you'll fit right here. If not, well, maybe the language nerd sense of humor will
be an acquired taste for you.

</aside>

There are pros and cons to tools like that, and strong opinions on both sides.
Here, I've chosen to eschew them. I want to ensure there are no dark corners
where magic and confusion can hide, so we'll be writing everything by hand from
scratch. As you'll see, it's not as bad as it sounds and it means you really
will understand each line of code and how both interpreters work.

[lex]: https://en.wikipedia.org/wiki/Lex_(software)
[yacc]: https://en.wikipedia.org/wiki/Yacc

A book has different constraints from "real world" code and so the style of the
code here might not always reflect the best way to write maintainable code in a
normal codebase. I tend to not worry about about access control modifiers like
`public` and `private` and don't do things like hide fields behind getters and
setters. The pages here aren't as wide as your IDE and every character counts
when I'm trying to make this easy for you to read.

Also, the code here doesn't have many comments. That's because each handful of
lines is surrounded by several paragraphs of honest-to-God prose explaining it.
If you write a book to accompany each of your programs, you can ditch the
comments too! Otherwise, you should probably use `//` a little more than I do.

Each chapter has a few other <span name="aside">accoutrements</span>.

<aside name="aside">

Asides like this one contain historical notes, references to related topics, and
suggestions of other areas to explore. Well, some do, at least. Most of them are
just dumb jokes and goofy illustrations. Sorry.

You can skip them if you want. I won't judge you.

</aside>

### Exercises

The exercises at the end are to help you learn more. Instead of reviewing what
the chapter already told you, they specifically force you to step off the guided
path and explore on your own. They will make you research other languages,
figure out how to implement other language features or otherwise get you to
strike out on your own initiative.

Boldly attack them and you'll come away with a broader understanding and
possibly a few bumps and scrapes. Or skip them if you just want to stay inside
the comfy confines of the tour bus. It's your book.

### Design notes

Most other books assume the language you're implementing is a given and are
focused on how to implement it. This book mainly does that too. I already did
the work to design Lox so you don't have to.

Focusing on implementation is fun because it is so <span
name="benchmark">well-defined</span>. In the ideal, you have a language spec
already and you just need to crank out some code that implements those semantics
and passes the test suite.

<aside name="benchmark">

I know a lot of language hackers who live entirely within this world. They are
like athletes where their language's benchmark suite determines how well they
score and the only criteria by which they are evaluated.

</aside>

But the softer side of languages, how a human actually uses it effectively, is a
vital part of making your *new* language successful. Most books don't talk much
about that, I think in large because it is fuzzier. Many computer scientistics
feel uncomfortable talking about anything that can't be proven like a theorem.

I look at languages as, in large part, a user interface. Each is a tool you use
to communicate how a machine should behave to the computer and to the other
programmers maintaining the code. You can't design a good language if you don't
think about the humans using it, even though we Homo sapiens don't have the
pleasant crispness of discrete mathematics.

To touch on that, many chapters also contain a section of "design notes". These
are little essays on some corner of the human aspect of programming languages.
What makes features easier or harder to learn, how to grow an ecosystem,
crafting a readable syntax, stuff like that.

I don't claim to be an expert on any of this—I don't know if anyone really
can—so take these with a large pinch of salt. That should make them tastier food
for thought, which is my main aim. If you come away disagreeing with me on all
accounts, but still *caring* about the user side of languages, that's enough for
me.

On the other hand, if you just want to pump out some code for an interpreter and
get it running, feel free to skip these sections.

### Part II, The first interpreter

**TODO: Condense.**

We'll write our first interpreter in Java. It's a good language for teaching
concepts. It has a nice set of collection types and frees us from having to
manage memory. At the same time, it's pretty explicit. Unlike scripting
languages, there tends to be less "magic" under the hood, and you've got static
types to see exactly what kinds of objects you're working with.

I also chose it specifically because it is an object-oriented language. That
paradigm swept the programming world in the 90s and is now the dominant way of
thinking for millions of programmers. Odds are good you're already used to
organizing things into classes and methods, so we'll keep you in that comfort
zone.

While the academic language community sometimes shies away from object-oriented
programming, the reality is that it is widely used for language work today as
well. GCC and LLVM are written in C++, as are most JavaScript virtual machines.
Object oriented languages are ubiquitous and the tools and compilers for those
languages are often written in the <span name="host">same language</span>.

<aside name="host">

A compiler is a program that reads in files in one language and translates them
to files in another language. You can implement one in any language, including
the same language it uses for its input. If your compiler is powerful enough, it
can even take in its own source code as input and compile itself. That's called
**"self-hosting".**

Of course, you need to be able to compile your compiler using some other
compiler you have laying around before you can run it and pass it its own source
code. But once you've done that once, you now have a compiled version of your
compiler that was produced by your own compiler. Now you can throw away the
version you compiled with the other compiler.

Henceforth, you can keep using previous versions of your own compiler to compile
the next version of it. This is called **"bootstrapping"** from the image of
pulling yourself up by your own bootstraps. (This is also where we get the term
**"booting"** for starting up a computer.)

**TODO: Illustration.**

A language ecosystem needs lots of different tools beyond just the core
compiler. You'll need editors, debuggers, formatters, etc. Most language
designers prefer to write those tools in their own language so they can get some
first-hand experience with their language, and so they aren't as dependent on
other languages. They call this "eating your own dogfood" or just
**"dogfooding"**.

</aside>

And, finally, Java is hugely popular. That means there's a good chance you
already know it, so there's less for you to learn to get going in the book. If
you aren't that familiar with Java, don't freak out. I try to stick to a fairly
minimal subset of it. I use the diamond operator from Java 8 to makes things a
little more terse, but that's about it as far as advanced features go. If you
know another object-oriented language like C# or C++, you can probably muddle
through fine.

For our first interpreter, we'll focus mostly on *concepts*. We'll write the
simplest, cleanest code we can to correctly implement the semantics of the
language. This will get us comfortable with the basic techniques and also hone
our understanding of exactly how the language is supposed to behave.

The end result is a simple, readable implementation, but not a very *fast* one.
It also leans on Java for managing memory and representing objects. But we want
to learn how the Java virtual machine itself implements those things.

### Part III, The second interpreter

**TODO: Condense.**

So in the next part, we'll start all over again, but this time in C. C is the
perfect language for understanding how an implementation *really* works, all the
way down to the bytes in memory and the code flowing through the CPU. It makes
explicit the few things Java doesn't: how memory is managed, and how objects are
represented.

A big reason that we're using C is so I can show you things C is particularly
good at, but that does mean you'll need to be pretty familiar with it. You don't
have to be the reincarnation of Dennis Ritchie, but you shouldn't be spooked by
pointers either.

If you aren't there yet, pick up an introductory book on C and chew through it,
then come back here when you're done. In return, you'll come away from this book
an even stronger C programmer.

That alone is a useful skill. C is still widely used for a variety of domains,
and many language implementations use it, especially scripting languages. Lua,
CPython, and Ruby's MRI are all written in C.

Our C interpreter forces us to implement ourselves all the things Java gave us
for free. We'll write our own dynamic array and hash table. We'll decide how
objects are represented in memory, and build a garbage collector to manage it.

Our Java implementation was focused on being correct. Now that we have that
down, we'll turn to also being *fast*. Our C interpreter will contain a compiler
that translates the code to an efficient bytecode representation which it then
executes. This is the same technique used by <span name="impl">implementations
of</span> Lua, Python, Ruby, PHP and many other successful languages.

<aside name="impl">

The "implementations of" part is significant in this sentence. There are
implementations of Python that use bytecode, and others that compile to native
code. That means words like "compiled" or "interpreted" don't describe a
*language*, just one particular language *implementation*.

</aside>

We'll even do a little benchmarking and optimization. By the end we'll have a
robust, accurate, fast interpreter for our language, able to keep up with other
professional caliber language implementations out there.

## Map of world of languages

This book isn't a broad survey and we aren't going to wander all over the place,
but it's worth at least looking at a map of the territory covered by languages.
It will help us understand where we are going and alternate paths other language
implementations take.

This is also a good time to get some basic terminology down. Have you ever
wondered what the difference between a "compiler" and an "interpreter" is? Now
you'll find out.

Unfortunately, questions like that are surprisingly fuzzy in computer science, a
field that claims to prize precision. Many of these terms were coined in a time
when computers ran literally a hundred thousand times slower and didn't have
enough storage to hold an entire source file in memory. The world has moved on,
but the terms haven't so we keep stretching their definitions to try to match
today's usage.

This is why even a seemingly simple questions like "Is language X compiled or
interpreted?" or even "What's the difference between a compiler and an
interpreter?" get so nebulous.

Fortunately, though, almost all language implementations over the entire history
of languages follow a couple of well-worn paths. Some may skip a step or two,
but you'll find a surprising amount of similarity in a Rear Admiral Grace
Hopper's first COBOL compiler and some brand-new not-even-beta-yet language
implementation with all of two commits on GitHub today.

Language implementations work in a series of phases. I think of them as climbing
over a mountain. It starts off at the bottom with the program as base source
text, literally just a string of <span name="chars">characters</span>.

<aside name="chars">

Already the language designer has to start making some decisions around the
flexibility and the usability of their language. Are source programs ASCII?
Unicode? What encoding? Do you allow non-ASCII characters in comments? Strings?
Variable names?

</aside>

Each phase analyzes the program and transforms it to some higher-level
representation where the semantics—what the author wants the computer to
do—becomes more obvious. Eventually we reach the peak, the point where our
implementation understands all it needs to about the user's program and knows
what it needs to do to be able to execute it.

Now we start going back down the other side of the mountain. We transform from
this highest-level representation back down to successive lower-level forms to
get closer and closer to something we know how to make the CPU actually execute.

There are a few branches along this path, and a couple of different endpoints on
the other side of the mountain, but this structure is remarkably similar across
almost all language implementations.

Here's the whole picture:

**TODO: Alt text.**

<img src="image/introduction/mountain.png" alt="???" class="wide" />

Now let's go through each of those trails and stopping points. Our journey
begins on the left with the source code the user has written in our language.

### Scanning

The first step is **scanning**, also known as **lexing** or (if you want to
sound fancy) **lexical analysis**. They all mean the pretty much same thing. I
like "lexing" because it sounds like something an evil supervillain would do,
but I'll use "scanning" here because it seems to be marginally more common in
usage.

A **scanner** takes in the linear stream of *characters* and chunk them together
into a series of something more akin to "words". "Lexical" comes from the Greek
root "lex", meaning "word".

In programming languages, each of these words is called a **token**. Some tokens
are single characters, like `(` and `,`. Others may be several characters long,
like numbers (`123`), string literals (`"hi!"`), and identifiers (`name`).

Some characters in a source file don't actually mean anything. Whitespace is
often insignificant and comments, by definition, are ignored by the language.
Scanning is the step where these usually get discarded.

**TODO: pipeline picture**

### Parsing

The next step is **parsing**. This is where our syntax gets a **grammar**—the
ability to compose larger phrases, expressions, and statements out of smaller
parts. Did you ever diagram sentences in English class? If so, you've basically
already done this. Except that English has thousands and thousands of "keywords"
and imperial tonnes of ambiguity. Programming languages are much simpler.

A **parser** takes a series of tokens and builds a tree structure that
explicitly encodes the nested nature of the grammar. These trees have a couple
of different names—**parse tree** or **abstract syntax tree**—depending on how
close to the grammatical structure to the language they are. In practice, most
language hackers just call them **"syntax trees"**, **"ASTs"** or often just
**"trees"**.

During parsing, we handle things like operator <span
name="precedence">precedence</span> so in `a + b * c`, the parser's job is to
build a tree that reflects that `b * c` is evaluated before adding it to `a`.
Parsing is also where we can detect and report most **syntax errors** like in `a
+ * b`.

<aside name="precedence">

That is assuming that your language *has* operator precedence. Some languages
like Smalltalk don't.

</aside>

Parsing has a long, rich history in computer science that is closely tied to the
artifical intelligence community. Many of the techniques used today to parse
programming languages were originally conceived to parse *human* languages by AI
researchers who were trying to get computers to talk to us.

It turns out human languages are often too ambiguous for the rigid grammars
those parsers could handle, but they were a perfect fit for the simpler
artificial grammars of programming languages.

**TODO: tokens to tree picture**

Some programming languages begin interpreting code right after they parse it.
Instead of doing any other ahead-of-time analysis or optimization, they simply
take the syntax tree and start interpreting it one branch and leaf at a time.
Any additional work they need to do to understand the user's code will be done
on the fly as each node is processed.

This style of interpretation is common for student projects and really simple
languages, but not widely used for general-purpose languages since it tends to
be slow. A notable exception is the original implementation of <span
name="ruby">Ruby</span> which worked this way before version 1.9.

<aside name="ruby">

At 1.9, the canonical implementation of Ruby switched from the original MRI
("Matz' Ruby Interpreter") to Koichi Sasada's YARV ("Yet Another Ruby VM"). YARV
is a bytecode virtual machine, another style of intepreter that we'll get to in
a bit.

</aside>

Some people use "interpreter" to mean these kinds implementations, but that can
be vague, so I'll use the sometimes-heard and more explicit **"tree-walk
interpreter"** to refer to these. Our first interpreter will roll this way.

### Static analysis

The first two stages are pretty similar among almost all implementations and
forms the **front end** of the pipeline. Now, the individual characteristics of
each language start coming into play. At this point, we know the grammatical
structure of the code, operator precedence, when an identifier is declaring a
variable versus accessing, etc. but we don't know much more than that.

For example, in an expression like `a + b`, we don't know what `a` and `b` refer
to. Are they local variables? Global? Where are they defined?

The first bit of analysis that most languages do is called **binding** or
**resolution**. For each **identifier** (variable name) we find out where that
name is defined and wire the two together. This is where **scope** comes into
play—the region of source code where a certain name can be used to refer to a
certain declaration.

If the language is <span name="type">statically typed</span>, this is when we
type check. Once we know where `a` and `b` are declared, we can also figure out
their types. Then if those types don't support being added to each other, we
report a **type error**.

<aside name="type">

The language we'll build in this book is dynamically typed, so it will do its
type checking later, at runtime.

</aside>

Now we have summitted the peak of the mountain. We have figured out everything
we can about the user's program without actually running it. All this data that
we've figured out through analysis needs to be stored somewhere. There are a
variety of places we can squirrel it away. Often, it gets stored right back as
**attributes** on the syntax tree itself—little extra fields that aren't
populated during parsing but get filled in later.

Other times, we may store it in a look-up table stored off to the side. For
example, we may use one to store the types of expressions. Give it an
expression, and it will return the previously calculated static type of it. More
often, the keys to this table are identifiers—names of variables and
declarations. In that case, we call it a **symbol table** and the values it maps
to names will help us understand what that name refers to.

### Optimization

We've done all the static analysis we can and reported errors for things that
didn't add up. We know the code is <span name="correct">correct</span>, and we
know what it means. What else can we do with this knowledge?

<aside name="correct">

"Correct" at least as far as the language is concerned. It's a valid program, it
might just be a valid program that doesn't do what the programmer wants it to
do.

</aside>

You've already read the subheader so you know the answer: we can optimize the
code. Since we know the semantics of the user's program, we are now free to
transform it into a different program that has the same semantics but implements
them more efficiently.

A simple example is **constant folding**: if some expression always evaluates to
the exact same value, we can do the evaluation now and replace the code for the
expression with its result. If the user typed in:

```java
pennyArea = 3.15159 * (0.75 / 2) * (0.75 / 2);
```

We can do all of that arithmetic in the compiler and change their code to:

```java
pennyArea = 0.44319234375;
```

To enable optimizations like this, we often transform the syntax tree into some
other **intermediate representation** (or **"IR"**). Each is a data structure
designed to make later optimizations and phases of the compiler easier to
implement. You can think of it as a pipeline where each stage's job is to
reorganize the code in a way to make the next stage simpler.

One common first step is eliminating <span name="sugar">**syntactic
sugar**</span>. Syntactic sugar is a dusting of grammatical niceties that make
your code more pleasant to read and write, but that don't let you express
anything you couldn't write down using other existing, more tedious language
features.

<aside name="sugar">

This delighful turn of phrase was coined by Peter J. Landin in 1964 to describe
how some of the nice expression syntaxes supported by languages like ALGOL were
a sugaring over the more fundamental, yet presumably less palatable lambda
calculus underneath.

**TODO: picture?**

</aside>

For example, in C, loops are mere syntactic sugar for labels and jumps. A bit of
code like:

```c
for (a = 0; a < 10; a++) {
  printf("%d\n", a);
}
```

Can be **desugared** to:

```c
  a = 0;
for_start:
  if (!(a < 10)) goto for_end;
  a++;
  printf("%d\n", a);
  goto for_start;
for_end:
```

And, of course, even that little `++` can be desugared to a more explicit
assignment. Expanding these shorthands to more verbose but less numerous
constructs makes later passes easier, since they have fewer different language
features that handle.

There are a handful of well-established intermediate representations and a
veritable bestiary of carefully named and studied optimizations that can be
applied to code once translated to one of them.

Optimization is a huge part of language academia as well as industry. Many
language hackers spend their entire careers here, squeezing every drop of
performance they can out of their compilers to get their benchmarks a fraction
of a percent faster. It can become a sort of obsession.

We're going to shy away from that. Many successful languages have surprisingly
few compile-time optimizations. Lua and CPython have glide down the back side of
the mountain with few stops along the way, and focus most of their optimization
effort on the runtime.

### Code generation

OK, we have applied all of the optimizations to the code we can think of. It's
correct, efficient. The last step is converting it to a form the machine can
actually execute. In other words **generating code**, where "code" refers to the
kind of primitive assembly-like instructions a CPU runs and not the kind of
"source code" a human might want to read.

Now we're going down the mountain in the <span name="back">**back end**</span>.
From here on out, our representation of the code will become increasingly more
primitive as we get closer and closer to a form the underlying machine can
actually execute. Where the front end takes code from the characters the
programmer wrote to the semantics they intended, the back end takes it from the
high level semantics to the low level steps a computer can follow.

<aside name="back">

Historically, compilers used to do little optimization and most analysis was
done directly inside the parsing code leading to the "front" and "back" halves.
As compilers got more sophisticated, more and more work was happening between
those two phases that didn't clearly belong to either side.

To be consistent with "front end" and "back end", they gave it the charming but
spatially confusing name **"middle end"**.

</aside>

We have a decision to make here. Do we generate instructions for a real CPU or a
virtual one? In what we usually think of as a "compiler", the answer is to
generate native machine code. This is the language your CPU itself executes, so
with that, the user's program is in a form that can be directly run by just
throwing the generated machine code at the chip. (That's why they call this form
an "executable".)

Targetting native code tends to be the fastest way to implement a language, but
is also a lot of work. Today's CPU architectures have piles of instructions,
complex pipelines, and enough <span name="aad">historical
baggage</span> to fill a 747.

<aside name="aad">

For example, the [AAD][] ("ASCII Adjust AX Before Division") instruction lets
you perform division, which sounds useful. Except that instruction works on two
binary-coded decimal digits that are packed in a single 16-bit register. When
was the last time you used BCD on a 16-bit machine?

[aad]: http://www.felixcloutier.com/x86/AAD.html

</aside>

Speaking the chip's language also means your compiler is tied to a specific
architecture. If your compiler targets [x86][] machine code, it's not going to
run on an [ARM][] device. All the way back in the 60s, during the Cambrian
explosion of computer architectures, that lack of portability was a real
obstacle.

[x86]: https://en.wikipedia.org/wiki/X86
[arm]: https://en.wikipedia.org/wiki/ARM_architecture

To get around that, hackers like Martin Richards and Niklaus Wirth, of BCPL and
Pascal fame, respectively, made their compilers generate *virtual* machine code.
Instead of instructions for some real chip, they'd produce code for a
hypothetical, idealized machine. Wirth called this **"p-code"** for "portable",
but today, we generally call it **bytecode** because each instruction is often a
single byte long.

Since the actual chip you're running on doesn't speak that bytecode language,
you still have some work to do. Again, you have two options. You can write a
little mini-compiler for each target architecture that translates the bytecode
to native code for that machine. You still have to do work for each chip you
support, but this last stage is pretty simple and you get to reuse your front
end and code generator across all of the machines you support.

Or you can write a **virtual machine** (**"VM"**), a program that emulates the
operation of the hypothetical chip that supports your virtual instructions.
Running bytecode on top of a VM is a little slower than running native code
directly because you've got a layer of abstraction and simulation in there, but
you get a lot of simplicity and portability in return. If the VM itself is
implemented in a portable language like C, then *your* language now supports any
architecture you can compile C to.

### Runtime

We have finally turned the user's program into a form that we can execute. The
last step is actually running it. If we compiled it to machine code, we can just
tell the operating system to load the executable and off it goes. If we compiled
it to bytecode, we need to start up the VM and load the program into that.

In both cases, for all but the most lowest-level of languages, we will usually
need to provide some services that our language declares are available while the
program is running.

For example, if the language is defined to automatically manage memory, we'll
need something like a garbage collector going while the program runs in order to
reclaim unused memory. If our language supports dynamic "instance of" tests so
you can see why type an object has, then we need some representation to keep
track of the type of each object during execution.

All of this stuff is going at runtime, so it's called, well, the **"runtime"**.
In a fully compiled language, the code implementing the runtime services gets
inserted directly into the compiled executable. In, say, [Go][], each compiled
application has its own copy of Go's runtime directly embedded in it.

[go]: https://golang.org/

If the language is run inside an interpreter or VM, that host executable
contains the runtime. This is how most implementations of languages like Java,
Python, and JavaScript work.

### Transpilers

Most languages that "compile"—transform the user's code—translate it into
another language at a lower level of abstraction than the input. Typically,
you're taking some high-level human-friendly language and lowering it to machine
code or bytecode.

But, instead of traipsing *down* the mountain, you could take a sort of sideways
shortcut by translating the input language to another language that's about as
high level. Then you use the existing tools for *that* language to descend the
mountain to something you can run.

A program that did this used to be called a **"source-to-source compiler"** or
<span name="transformer">a</span> **"transcompiler"**. Ever since the rise of
languages that compile to JavaScript in order to run in the browser, they've
gotten the shorter label **"transpiler"**.

<aside name="transformer">

"Transcompiler" is also the name of the nerdiest Transformer to make its way off
Cybertron.

</aside>

The first transcompiler translated 8088 assembly code into 8086 assembly. These
days, almost all transpilers work on higher-level languages. After the rise of
UNIX, there began a long tradition of compilers that produced C as their output
language. C compilers were widely available for a number of architectures and
produced efficient code, so translating to C was a good way to get your language
running on a lot of machines.

Web browsers are the "machines" of today, and their "machine code" is
JavaScript, so these days it seems almost every language out there has a
compiler that targets JS since that's the <span name="js">only</span> way to get
your code running in a browser.

<aside name="js">

JS may not be the only language browsers natively support for much longer. If
[Web Assembly][] takes off, browsers will support another lower-level language
specifically designed to be targeted by compilers.

[web assembly]: https://github.com/webassembly/

</aside>

The front end—scanner and parser—of a transpiler looks like other compilers. If
the source language is just a simple syntactic skin over the target language, it
may skip analysis entirely and go from the syntax tree straight to outputting
the analogous syntax in the destination language.

If the two languages are more semantically different, then you'll see more of
the typical phases of a full compiler including analysis and possibly even
optimization. Then, when it comes to code generation, instead of outputting some
binary language like machine code, you produce a string of grammatically correct
source (well, destination) code in the target language.

## How these parts are organized

Those are all the different phases and parts you're likely to see in any
language implementation. Not every implementation has all of these pieces, but
few implementations have major parts not covered by this list.

Some implementations collapse a few phases together. Languages that use [parsing
expression grammars][peg] as their parsing strategy often combine scanning into
it too into a single holistic grammar that goes from the language syntax all the
way down to individual characters.

[peg]: http://bford.info/packrat/

Many simple compilers <span name="sdt">interleave</span> parsing, analysis, and
code generation so that they can generate output code without creating any
explicit syntax trees or other IRs. These **single-pass compilers** limit the
design of the language. You have no intermediate data structures to store global
information about the program, and you don't revisit any previously parsed part
of the code. That means as soon as you parse some expression, you need to
already know enough to correctly compile it.

<aside name="sdt">

[Syntax-directed translation][pass] is a structured technique to help build
these all-at-once compilers. You associate an *action* with each grammar rule,
usually one that generates output code. Then, whenever the parser matches that
piece of the grammar, it executes that action, building up the target code one
rule at a time.

[pass]: https://en.wikipedia.org/wiki/Syntax-directed_translation

</aside>

Some older languages were designed around these constraints. At the time, memory
was so precious that a compiler might not even be able to hold an entire source
file in RAM, much less the rest of the program. So C and Pascal, among others,
were designed such that it was possible to compile them a tiny piece at a time.
This is why both require explicit *forward declarations* for any function or
type you want to use that hasn't been defined yet.

### Developers and users

Even in implementations that have the exact same phases, there is often an
interesting split point in them. In many languages, the process of going from
the source code the developer authors to a form the user can run happens on the
developer's machine, on the end user's machine, or some mixture of the two.

In a traditional "compiled" language like C or Go that is compiled all the way
to machine code, the entire pipeline runs on the developer's machine. The end
user gets a binary that they can directly execute.

In scripting languages like JavaScript, Python, Ruby, etc. the program is
distributed to the user as its original source code. All of the stages from
scanning through to code generation happen on the end user's machine every time
they run the program.

Other languages split the difference. On the developer's machine, the code is
compiled to some bytecode, which is what the user receives. Then, on the user's
machine a virtual machine loads that bytecode and interprets it. This is how
Java and C#, and other languages that target the Java Virtual Machine (JVM) or
Common Language Runtime (CLR) are executed.

In practice, advanced interpreters and bytecode VMs often contain within them
one final translation to machine code. The HotSpot JVM, Microsoft's CLR and most
JavaScript interpreters will take the bytecode or analyzed source code and
compile it all the way to optimized machine code at runtime on the end user's
machine.

This process of generating native code right at the last second before it's run
is called **"just-in-time compilation"**. Most hackers just say "JIT",
pronounced like it rhymes with "fit". The most hardcore VMs JIT the code
multiple times with greater levels of optimization as they discover which
corners of the user's program are performance <span name="hot">hot spots</span>.

<aside name="hot">

This is, of course, exactly where the HotSpot JVM gets its name.

</aside>

**TODO: The above is a lot of prose. Illustration, list, or subheader?**

### Compilers and interpreters

Now we can get back to our original question. Why *is* the difference between a
compiler and an interpreter? In a lot of ways, this is like asking the
difference between a fruit and a vegetable. That *sounds* like a single binary
choice, but "fruit" is a botanical term and "vegetable" is culinary. One does
not imply the negation of the other. That means there are fruits that aren't
vegetables (apples), vegetables that are not fruits (lettuce), but also things
that are *both* fruits and vegetables (tomatoes).

**Compilation** is an *implementation technique* where you translate a source
language to some other—usually lower-level—form. When you generate bytecode or
machine code, you are compiling. When you transpile to another high-level
language you are compiling too. If users run a tool that takes a source language
and outputs some target language and then stops, we call that tool a
**compiler**.

**Interpretation** describes the *user experience of executing a language*. If
end users run a program from source without having to first go through a
separate tool to process the code, the thing they use to run their program is an
**interpreter**.

First off, recall that neither compilation nor interpretation is a property of a
*language*. There are Scheme implementations that compile to C or machine code,
and others that interpret directly from source.

In many language implementations, "compile" and "interpret" are indeed disjoint
concepts. GCC and Clang take your C code and compile it to machine code. An end
user runs that executable directly and may never even know which tool was used
to compile it. So those are *compilers* for C.

In older versions of Matz' canonical implementation of Ruby, the user ran Ruby
from source. The implementation parsed it and ran it directly by traversing the
syntax tree. No other translation occurred. So this was definitely an
*interpreter* for Ruby.

But what of CPython? When you run your Python program using it, it is parsed and
converted to an internal bytecode format, which is then executed inside the VM.
From the user's perspective, this is clearly an interpreter—they run their
program from source. But if you look under CPython's scaly skin, you'll see that
there is clearly some compiling going on.

The answer is that it is <span name="go">both</span>. CPython *is* an
interpreter, and it *has* a compiler. In practice, most scripting languages work
this way. Interpreting straight from the parsed syntax tree is usually too slow
to be practical, so almost all widely-used interpreters contain an internal
compiler.

This is how the second interpreter we'll build works. So while this book is
nominally about "interpreters", we'll also learn compilation too.

<aside name="go">

The [Go tool][go] is even more of a chimera. If you run `go build`, it compiles
your Go source code to machine code and stops. If you type `go run`, it does
that then immediately executes the generated executable.

So `go` *has* a compiler, *is* an interpreter and *is* also a compiler.

[go]: https://golang.org/cmd/go/

</aside>

**TODO: illustration showing various lang impls and which phases they have and which run on user's machine**

## Our own journey

That's a lot to take in all at once. Don't worry. This isn't the chapter where
you're expected to *understand* all of these pieces and parts. I just want you
to know that they are out there and roughly how they fit together.

This map should serve you well as you explore the territory well beyond the
guided path we take in this book. I want to leave you yearning to strike out on
your own and wander all over that mountain.

But, for now, it's time for our own journey to begin. Tighten your bootlaces,
cinch up your pack, and come along. From here on out, all you need to focus on
is the path in front of you.

## Exercises (chap 1)

1. There are least six domain-specific languages used in the little system I
   cobbled together for myself to write and publish this book. What are they?

   **TODO: More.**

## Exercises (chap 2)

1. Pick an open source implementation of a language you like. Download the
   source code and poke around in it. Try to find the code that implements the
   scanner and parser. Are they hand-written, or generated using tools like
   Lex and Yacc? (`.l` or `.y` files tend to imply the latter.)

1. Most Lisp implementations that compile to C must also contain an interpreter
   to let them execute Lisp code on the fly as well. Why?

   **TODO: More.**
