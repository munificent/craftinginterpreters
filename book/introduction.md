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

To my friends and family, sorry I've been so absent-minded!

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

**TODO: Conclusion.**

<div class="exercises">

## Exercises

1. There are least six domain-specific languages used in the little system I
   cobbled together for myself to write and publish this book. What are they?

1. Get a "Hello, world!" program written and running in Java. Set up whatever
   Makefiles or IDE projects you need to get it working. If you have a debugger,
   get comfortable with it and step your program as it runs.

1. Do the same thing for C. To get some practice with pointers, define a
   [doubly-linked list][] of heap-allocated strings. Write functions to insert,
   find, and delete items from it. Test them.

[doubly-linked list]: https://en.wikipedia.org/wiki/Doubly_linked_list

</div>
