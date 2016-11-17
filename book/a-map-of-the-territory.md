^title A Map of the Territory
^part Welcome

> Believable fairy-stories must be intensely practical. You must have a map, no
> matter how rough. Otherwise you wander all over the place. In The Lord of the
> Rings I never made anyone go farther than he could on a given day.
>
> <cite>J.R.R. Tolkien</cite>

We aren't going to wander all over the place, but it's worth looking at the
territory previous language implementers have diligently charted. It will help
us understand where we are going and alternate paths other implementations take.

### "Implementations" versus "Languages"

First up, let's establish a little shorthand so we can talk to each other more
efficiently. The opening paragraph stuffs "implementation" in each of its
sentences, and for good reason. We really are talking about a language
*implementations*, not the language *itself* in sort of of Platonic ideal form.
A *language* has things like syntax, semantics, a grammar, and (if you're lucky)
even a written specification.

That specification doesn't usually contain words like "compiler", "bytecode",
"recursive descent", "stack", "heap", etc. Those are just nuts and bolts one
particular *implementation* of the language might choose to use. But it doesn't
*have* to. From the user's perspective, as long as the resulting contraption
faithfully follows the language's semantics, it's all implementation detail.

Since this book is mostly *about* implementation, if I have to keep writing that
<span name="intl">mouthful</span> of a word, I'll wear my fingers off before I
reach the last chapter. Instead, from here on out, I'll use "language" to refer
to either a language implementation, or a language itself, or both, unless the
distinction really matters and isn't obvious.

<aside name="intl">

Maybe I should do what the accessibility and internationalization folks do and
call it "i12n", like they do with "a11y" and "i18n".

</aside>

## The parts of a language

Engineers have been building programming languages since the dark ages of
computing. As soon as we could talk to computers, we discovered doing so was too
hard, and we enlisted their help. I find it fascinating that even though today's
machines are literally a million times faster and have orders of magnitude more
storage, they have almost the same structure.

Though the territory covered by languages designers is vast, the trails they've
carved through it are <span name="dead">few</span>. Not every language takes the
exact same path -- some take a shortcut or two -- but otherwise they are
reassuringly similar from Rear Admiral Grace Hopper's first COBOL compiler all
the way to some hot new transpile-to-JavaScript language whose "documentation"
consists entirely of a single poorly-edited README in a Git repository
somewhere.

<aside name="dead">

Though there are certainly dead ends. Sad little cul-de-sacs of CS papers with
zero citations and now-forgotten optimizations that only made sense when memory
was measured in individual bytes.

</aside>

I visualize this network of paths an implementation may take as climbing a
mountain. You start off at the bottom with the program as raw source text,
literally just a string of <span name="chars">characters</span>. Each phase
analyzes the program and transforms it to some higher-level representation where
the semantics -- what the author wants the computer to do -- becomes more
apparent.

<aside name="chars">

Already the language designer has to start making some decisions around the
flexibility and the usability of their language. Are source programs ASCII?
Unicode? What encoding? Do you allow non-ASCII characters in comments? Strings?
Variable names?

</aside>

Eventually we reach the peak. From this lofty vantage point, we have the best
view of users's program and we understand what their code *means*. Now we
descend the other side of the mountain. We transform from this highest-level
representation down to successively lower-level forms to get closer and
closer to something we know how to make the CPU actually execute.

There are a few branches along this path, and a couple of different endpoints on
the other side of the mountain, but this structure is remarkably similar across
almost the world's programming languages.

Here's the whole picture:

<img src="image/a-map-of-the-territory/mountain.png" alt="The branching paths a language may take over the mountain." class="wide" />

Place your finger on the map, and we'll trace through each of those trails and
stopping points. Our journey begins on the left with the bare text of the user's
source code:

<img src="image/a-map-of-the-territory/string.png" alt="var average = (min + max) / 2;" />

### Scanning

The first step is **scanning**, also known as **lexing**, or (if you're trying
to impress someone) **lexical analysis**. They all mean the pretty much same
thing. I like "lexing" because it sounds like something an evil supervillain
would do, but I'll use "scanning" here because it seems to be marginally more
common in usage.

A **scanner** (or **"lexer"**) takes in the linear stream of characters and
chunks them together into a series of something more akin to "words". "Lexical"
comes from the Greek root "lex", meaning "word".

In programming languages, each of these words is called a **token**. Some tokens
are single characters, like `(` and `,`. Others may be several characters long,
like numbers (`123`), string literals (`"hi!"`), and identifiers (`min`).

Some characters in a source file don't actually mean anything. Whitespace is
often insignificant and comments, by definition, are ignored by the language.
The scanner usually discards these, leaving a clean sequence of meaningful
tokens.

<img src="image/a-map-of-the-territory/tokens.png" alt="[var] [average] [=] [(] [min] [+] [max] [)] [/] [2] [;]" />

### Parsing

The next step is **parsing**. This is where our syntax gets a **grammar** --
the ability to compose larger phrases, expressions, and statements out of
smaller parts. Did you ever diagram sentences in English class? If so, you've
done what a parser does, except that English has thousands and thousands of
"keywords" and an overflowing cornucopia of ambiguity. Programming languages are
much simpler.

A **parser** takes the sequence of tokens and builds a tree structure that
explicitly encodes the nested nature of the grammar. These trees have a couple
of different names -- **"parse tree"** or **"abstract syntax tree"** --
depending on how close to the grammatical structure to the language they are. In
practice, most language hackers just call them **"syntax trees"**, **"ASTs"**,
**"AST nodes"** or often just **"trees"**.

<img src="image/a-map-of-the-territory/ast.png" alt="An abstract syntax tree." />

The parser handles things like operator <span
name="precedence">precedence</span> so in, say, `a + b * c`, it builds a tree
that reflects that `b * c` is evaluated before adding it to `a`. Parsing is also
where we can detect and report most **syntax errors** like in `a
+ * b`.

<aside name="precedence">

That is assuming that your language *has* operator precedence. Some languages
like Lisp, Smalltalk, and Forth don't. APL is so weird I can't even figure out
whether or not it does.

</aside>

Parsing has a long, rich history in computer science that is closely tied to the
artifical intelligence community. Many of the techniques used today to parse
programming languages were originally conceived to parse *human* languages by AI
researchers who were trying to get computers to talk to us.

It turns out human languages are too messy for the rigid grammars those parsers
could handle, but they were a perfect fit for the simpler artificial grammars of
programming languages.

### Static analysis

The first two stages are pretty similar among almost all implementations and
forms the **front end** of the pipeline. Now, the individual characteristics of
each language start coming into play. At this point, we know the grammatical
structure of the code, operator precedence, when an identifier is declaring a
variable versus accessing, etc. but we don't know much more than that.

For example, in an expression like `a + b`, we don't know what `a` and `b` refer
to. Are they local variables? Global? Where are they defined?

The first bit of analysis that most languages do is called **binding** or
**resolution**. For each **identifier** we find out where that name is defined
and wire the two together. This is where **scope** comes into play -- the region
of source code where a certain name can be used to refer to a certain
declaration.

If the language is <span name="type">statically typed</span>, this is when we
type check. Once we know where `a` and `b` are declared, we can also figure out
their types. Then if those types don't support being added to each other, we
report a **type error**.

<aside name="type">

The language we'll build in this book is dynamically typed, so it will do its
type checking later, at runtime.

</aside>

Take a deep breath. We have reached the summit of the mountain and attained a
bird's-eye view of the user's program. We can see all you could hope to know
about it without actually running it. All this knowledge we gained through
analysis needs to be stored somewhere. There are a variety of places we can
squirrel it away:

* Often, it gets stored right back as **attributes** on the syntax tree
  itself -- extra fields in the nodes that aren't initialized during parsing
  but get filled in later.

* Some languages consume the original syntax tree and produce a new "resolved"
  or "typed" tree that retains the tree structure of the original but with
  things like identifiers pointing directly to what they refer to.

* Other times, we may store data in a look-up table off to the side. Typically,
  the keys to this table are identifiers -- names of variables and declarations.
  In that case, we call it a **symbol table** and the values it associates with
  each key tell us what that identifier refers to.

In practice, many languages use a combination of these for storing various kinds
of data.

### Tree-walk interpreters and compilation

Some programming languages begin executing code about now, after parsing with
maybe a bit of static analysis applied to the trees. To run the program, they
traverse the syntax tree one branch and leaf at a time, interpreting the nodes
as they go.

This style of interpretation is common for student projects and really simple
languages, but not widely used for general-purpose languages since it tends to
be slow. A notable exception is the original implementation of <span
name="ruby">Ruby</span> which worked this way before version 1.9.

<aside name="ruby">

At 1.9, the canonical implementation of Ruby switched from the original MRI
("Matz' Ruby Interpreter") to Koichi Sasada's YARV ("Yet Another Ruby VM"). YARV
is a bytecode virtual machine, which we'll get to in a bit.

</aside>

Some people use "interpreter" to mean only these kinds implementations, but
others define that word more generally, so I'll use the inarguably explicit
**"tree-walk interpreter"** to refer to these. Our first interpreter rolls this
way.

Languages that don't execute now instead continue along the path. We'll get more
precise on the distinction later, but these steps down the back side of the
mountain encompass **compilation** and an implementation that contains any of
them is a **compiler**.

### Optimization

We've done all the static analysis we can and reported errors for things that
didn't add up. We know the code is <span name="correct">correct</span>, and we
know what it means. What else can we do with this knowledge?

<aside name="correct">

"Correct" at least as far as the language is concerned. It's a *valid program*,
it might just be a valid program that doesn't do *what the programmer wants it
to do*.

</aside>

You've already read the subheader so you know the answer: we can optimize it.
Since we know the semantics of the user's program, we are free to transform it
into a *different* program that has the *same semantics* but implements them
more efficiently.

A simple example of optimization is **constant folding**: if some expression
always evaluates to the exact same value, we can do the evaluation at compile
time and replace the code for the expression with its result. If the user typed
in:

```java
pennyArea = 3.15159 * (0.75 / 2) * (0.75 / 2);
```

We can do all of that arithmetic in the compiler and change the code to:

```java
pennyArea = 0.44319234375;
```

Optimization is a huge part of language academia as well as industry. Many
language hackers spend their entire careers here, squeezing every drop of
performance they can out of their compilers to get their benchmarks a fraction
of a percent faster. It can become a sort of obsession.

We're going to hop over that rathole in this book. Many successful languages
have surprisingly few compile-time optimizations. Lua and CPython glide down the
back side of the mountain with few stops along the way, and focus most of their
optimization effort on the runtime.

### Intermediate representations

To make it easier to discover optimizations that can be applied, languages often
transform the syntax tree into some other **intermediate representation** (or
**"IR"**). It's called "intermediate" because it doesn't reflect the source code
the user typed in, or the final output code the compiler generates.

You can think of the compiler as a pipeline where each stage's job is to
organize the code in a way that makes the next stage simpler to implement. The
intermediate representations are the ones in the middle that ultimately get
consumed by later stages.

A common first step is eliminating <span name="sugar">**syntactic
sugar**</span>. Syntactic sugar is a dusting of grammatical niceties that make
your code more pleasant to read and write, but that don't let you express
anything you couldn't write down using other existing, more tedious language
features.

<aside name="sugar">

This delighful turn of phrase was coined by Peter J. Landin in 1964 to describe
how some of the nice expression syntaxes supported by languages like ALGOL were
a sugaring over the more fundamental, yet presumably less palatable lambda
calculus underneath.

![Slightly more than a spoonful of sugar.](image/a-map-of-the-territory/sugar.png)

</aside>

For example, in C, loops are mere syntactic sugar for labels and jumps. Take
this code:

```c
for (a = 0; a < 10; a++) {
  printf("%d\n", a);
}
```

The compiler can **desugar** it to:

```c
  a = 0;
for_start:
  if (!(a < 10)) goto for_end;
  a++;
  printf("%d\n", a);
  goto for_start;
for_end:
```

(Even that little `++` can be desugared to an explicit addition and assignment.)
Expanding these shorthands to more verbose but less numerous constructs makes
later passes easier, since they have fewer different language features to worry
about. If, for example, you desugar all `if`, `for`, and `while` statements to
`goto`, then a later optimization that applies to `goto` will speed up all three
of those.

Simplifying the code by removing syntactic sugar is only the beginning when it
comes to optimization. There are a <span name="established">pack</span> of
well-established intermediate representations and a veritable bestiary of
carefully named and studied optimizations that can be applied to code once
translated to one of them.

<aside name="established">

If you want some keywords to Google, try "control flow graph", "static
single-assignment", "continuation-passing style", and "three-address code" for
IRs.

For optimizations, try "constant propagation", "common subexpression
elimintation", "loop invariant code motion", "global value numbering", "strength
reduction", "scalar replacement of aggregates", "dead code elimination", and
"loop unrolling", to name a few.

</aside>

### Code generation

We have applied all of the optimizations we can think of to the user's program.
The last step is converting it to a form the machine can actually run. In other
words **generating code**, where "code" refers to the kind of primitive
assembly-like instructions a CPU runs and not the kind of "source code" a human
might want to read.

This side of the mountain we're descending is the <span name="back">**back
end**</span>. From here on out, our representation of the code becomes more and
more primitive, like evolution run in reverse, as we get closer to a form the
simple-minded CPU can understand.

<aside name="back">

Historically, compilers did little optimization and interleaved analysis with
parsing, leading to the "front" and "back" ends. As compilers got more
sophisticated, more and more work happened between those two halves.

Instead of replacing the old terms, they gave static analysis and optimization
the charming but spatially illogical name **"middle end"**.

</aside>

We have a decision to make. Do we generate instructions for a real CPU or a
virtual one? In a traditional "compiler", the answer is usually to generate
native machine code. This produces an executable that the OS can load directly
onto the chip.

Native code is lightning fast, but generating it is a lot of work. Today's CPU
architectures have piles of instructions, complex pipelines, and enough <span
name="aad">historical baggage</span> to fill a 747. Speaking the chip's language
also means your compiler is tied to a specific architecture. If your compiler
targets [x86][] machine code, it's not going to run on an [ARM][] device. All
the way back in the 60s, during the Cambrian explosion of computer
architectures, that lack of portability was a real obstacle.

<aside name="aad">

For example, the [AAD][] ("ASCII Adjust AX Before Division") instruction lets
you perform division, which sounds useful. Except that instruction works on two
binary-coded decimal digits that are packed into a single 16-bit register. When
was the last time *you* used BCD on a 16-bit machine?

[aad]: http://www.felixcloutier.com/x86/AAD.html

</aside>

[x86]: https://en.wikipedia.org/wiki/X86
[arm]: https://en.wikipedia.org/wiki/ARM_architecture

To get around that, hackers like Martin Richards and Niklaus Wirth, of BCPL and
Pascal fame, respectively, made their compilers produce *virtual* machine code.
Instead of instructions for some real chip, they'd produce code for a
hypothetical, idealized machine. Wirth called this **"p-code"** for "portable",
but today, we generally call it **bytecode** because each instruction is often a
single byte long.

Each synthetic instruction set was designed to map a little more closely to the
language's semantics, and not be so tied to the peculiarities of any one
computer architecture and its accumulated historical cruft. You can think of it
like a dense, binary encoding of the language's low-level semantics.

### Virtual machine

If your compiler produces bytecode, your work isn't over once that's done. Since
there is no chip that speaks your bytecode, it's your job to translate. Again,
you have two options. You can write a little mini-compiler for each target
architecture that converts the bytecode to native code for that machine. You
still have to do work for <span name="shared">each</span> chip you support, but
this last stage is pretty simple and you get to reuse the rest of the compiler
pipeline across all of the machines you support.

<aside name="shared">

The basic principle here is that the farther down the pipeline you can push the
architecture-specific work, the more of the earlier phases you can share across
architectures. There is a tension, though. Many optimizations, like register
allocation and instruction selection, work best when they know the strengths and
capabilities of a specific chip.

Figuring out which parts of your compiler can be shared and which should be
target-specific is an art.

</aside>

Or you can write a **virtual machine** (**"VM"**), a program that emulates a
hypothetical chip that supports your virtual architecture at runtime. Running
bytecode in a VM is slower than translating it to native code ahead of time
because every instruction must be simulated at runtime as it executes. In return
you, get simplicity and portability. Implement your VM in, say, C, and you can
run your language on any platform that has a C compiler. This is what our second
interpreter does.

### Runtime

We have finally hammered the user's program into a form that we can execute. The
last step is actually running it. If we compiled it to machine code, we can just
tell the operating system to load the executable and off it goes. If we compiled
it to bytecode, we need to start up the VM and load the program into that.

In both cases, for all but the basest of low-level of languages, we usually need
to provide some services that our language supports while the program is
running. For example, if the language automatically manages memory, we need a
garbage collector going in order to reclaim unused bits. If our language
supports "instance of" tests so you can see what type an object has, then we
need some representation to keep track of the type of each object during
execution.

All of this stuff is going at runtime, so it's called, well, the **"runtime"**.
In a fully compiled language, the code implementing the runtime gets compiled
directly into the resulting executable. In, say, [Go][], each compiled
application has its own copy of Go's runtime directly embedded in it.

[go]: https://golang.org/

If the language is run inside an interpreter or VM, then the runtime lives in
there. This is how most implementations of languages like Java, Python, and
JavaScript work.

### Transpilers

<span name="gary">Running</span> through static analysis, possibly a few rounds
of optimization, and then down through code generation is the classic path to
getting the user's code into a runnable form. It's the full hike through the
mountain. However, there is a shortcut that's been around almost as long.

Instead of transforming the parsed, analyzed code to some *lower-level* form
like machine code, we can take a sideways hop and translate it to another
*high-level* language. Then, we use the existing compilation tools for *that*
language as our escape route off the mountain.

An implementation that did this used to be called a **"source-to-source
compiler"** or a **"transcompiler"**. After the rise of languages that compile
to JavaScript in order to run in the browser, they've affected the hipster
sobriquet **"transpiler"**.

<aside name="gary">

The first transcompiler, XLT86, translated 8088 assembly into 8086 assembly.
That might seem straightforward, but keep in mind the 8080 was an 8-bit chip and
the 8086 a 16-bit chip that could use each register as a pair of 8-bit ones.
XLT86 did data flow analysis to track register usage in the source program and
then efficiently map it to the register set of the 8086.

It was written by Gary Kildall, a tragic hero of computer science if there
ever was one. One of the first people to recognize the promise of
microcomputers, he created PL/M and CP/M, the first high level language and OS
for them.

He was a sea captain, business owner, licensed pilot, and motocyclist. A TV host
with the Kris Kristofferson-esque look sported by dashing bearded dudes in the
80s. He took on Bill Gates and, like many, lost, before meeting his end
in a biker bar under mysterious circumstances. He died too young, but sure as
hell lived before he did.

</aside>

While the first transcompiler translated one assembly language to another,
today, almost all transpilers work on higher-level languages. After the viral
spread of UNIX to machines various and sundry, there began a long tradition of
compilers that produced C as their output language. C compilers were available
everywhere UNIX was and produced efficient code, so targetting C was a good way
to get your language running on a lot of architectures.

Web browsers are the "machines" of today, and their "machine code" is
JavaScript, so these days it seems [almost every language out there][js] has a
compiler that targets JS since that's the <span name="js">only</span> way to get
your code running in a browser.

[js]: https://github.com/jashkenas/coffeescript/wiki/list-of-languages-that-compile-to-js

<aside name="js">

JS may not be the only language browsers natively support for much longer. If
[Web Assembly][] takes off, browsers will support another lower-level language
specifically designed to be targeted by compilers.

[web assembly]: https://github.com/webassembly/

</aside>

The front end -- scanner and parser -- of a transpiler looks like other
compilers. Then, if the source language is only a simple syntactic skin over the
target language, it may skip analysis entirely and go straight to outputting the
analogous syntax in the destination language.

If the two languages are more semantically different, then you'll see more of
the typical phases of a full compiler including analysis and possibly even
optimization. Then, when it comes to code generation, instead of outputting some
binary language like machine code, you produce a string of grammatically correct
source (well, destination) code in the target language.

Either way, you then run that resulting code through the target language's
existing compilation pipeline and you're good to go.

## How these parts are organized

Those are all the different pieces and parts you're likely to see in any
language implementation. Some languages omit a piece or two, or combine a
couple, but there are few parts not covered by this list.

Languages that use [parsing expression grammars][peg] as their parsing technique
tend to merge scanning and parsing into a single holistic grammar that goes from
individual characters all the way up to the language syntax.

[peg]: http://bford.info/packrat/

### Single-pass compilers

Some simple compilers <span name="sdt">interleave</span> parsing, analysis, and
code generation so that they can produce output code directly in the parser,
without ever creating any explicit syntax trees or other IRs. These
**single-pass compilers** restrict the design of the language. You have no
intermediate data structures to store global information about the program, and
you don't revisit any previously parsed part of the code. That means as soon as
you parse some expression, you need to know enough to correctly compile it.

<aside name="sdt">

[Syntax-directed translation][pass] is a structured technique to help build
these all-at-once compilers. You associate an *action* with each grammar rule,
usually one that generates output code. Then, whenever the parser matches that
piece of the grammar, it executes that action, building up the target code one
rule at a time.

[pass]: https://en.wikipedia.org/wiki/Syntax-directed_translation

</aside>

Pascal and C were designed around this limitation. At the time, memory was so
precious that a compiler might not even be able to hold an entire source file in
RAM, much less the whole program. This is why Pascal's grammar requires type
declarations appear first in a block. It's why in C you can't call a function
until after the code that defines it unless you have an explicit *forward
declaration* for it—that declaration tells the compiler what it needs to know to
generate code for to call the function.

### Developers and users

While the path is similar across many languages, there is an interesting
decision they all must make: which parts of the path run on the developer's
machine and which run on the end user's?

In languages like C or Rust that compile all the way to machine code, the entire
pipeline runs on the developer's machine. The end user gets a native binary that
they directly execute.

In scripting languages like JavaScript, Python, Ruby, etc. the program is
distributed to the user in its original source code form. All of the stages from
scanning through to code generation happen on the end user's machine every time
they run the program.

Other languages split the difference. On the developer's machine, the code is
compiled to some bytecode, which is what the user receives. Then, on the user's
machine a virtual machine loads that bytecode and interprets it. This is how
Java and C#, and other languages that target the Java Virtual Machine (JVM) or
Common Language Runtime (CLR) are executed.

### Just-in-time compilation

In practice, sophisticated interpreters and bytecode VMs often contain within
them one final translation to machine code. The HotSpot JVM, Microsoft's CLR and
most JavaScript interpreters will take the bytecode or analyzed source code and
then compile it all the way to optimized machine code at runtime on the end
user's machine.

This process of generating native code right at the last second before it's run
is called **"just-in-time compilation"**. Most hackers just say "JIT",
pronounced like it rhymes with "fit". Hardcore VMs JIT the code multiple times
with greater levels of optimization as they discover which corners of the user's
program are performance <span name="hot">hot spots</span>.

<aside name="hot">

This is, of course, exactly where the HotSpot JVM gets its name.

</aside>

## "Compilers" versus "interpreters"

A perennial question in languages is, "What's the difference between a compiler
and an interpreter?" Now we know enough to answer that with some fidelity.

This is sort of like asking what's the difference between a fruit and a
vegetable. That *sounds* like a single binary either-or choice, but actually
"fruit" is a botanical term and "vegetable" is culinary. One does not imply the
negation of the other. There are fruits that aren't vegetables (apples) and
vegetables that are not fruits (carrots), but also plants that are *both*
fruits and vegetables (tomatoes).

<span name="veg"></span></span>

<img src="image/a-map-of-the-territory/plants.png" alt="A Venn diagram of edible plants" />

<aside name="veg">

There are even plant-based foods that are *neither*, like nuts and cereals.

</aside>

* **Compilation** is an *implementation technique* that involves translating a
  source language to some other -- usually lower-level -- form. When you
  generate bytecode or machine code, you are compiling. When you transpile to
  another high-level language you are compiling too. If users run a tool that
  takes a source language and outputs some target language and then stops, we
  call that tool a **compiler**.

* **Interpretation** describes the *user experience of executing a language*. If
  the end user has a single tool that takes in source code and is able to then
  execute it immediately, that tool is an **interpreter**.

First off, we should be clear that these terms apply to language
*implementations*, not languages themselves. There are interpreters for C, and
compilers for Python. There are Scheme implementations that compile to machine
code, and others that interpret directly from source.

Like apples and oranges, some implementations are clearly compilers and *not*
interpreters. GCC and Clang take your C code and compile it to machine code. An
end user runs that executable directly and may never even know which tool was
used to compile it. So those are *compilers* for C.

In older versions of Matz' canonical implementation of Ruby, the user ran Ruby
from source. The implementation parsed it and ran it directly by traversing the
syntax tree. No other translation occurred, either internally or in any
user-visible form. So this was definitely an *interpreter* for Ruby.

But what of CPython? When you run your Python program using it, it is parsed and
converted to an internal bytecode format, which is then executed inside the VM.
From the user's perspective, this is clearly an interpreter -- they run their
program from source. But if you look under CPython's scaly skin, you'll see that
there is clearly some compiling going on.

The answer is that it is <span name="go">both</span>. CPython *is* an
interpreter, and it *has* a compiler. In practice, most scripting languages work
this way. Interpreting straight from the parsed syntax tree is usually too slow
to be practical, so almost all widely-used interpreters contain an internal
compiler.

<aside name="go">

The [Go tool][go] is even more of a horticultural curiosity. If you run `go
build`, it compiles your Go source code to machine code and stops. If you type
`go run`, it does that then immediately executes the generated executable.

So `go` *has* a compiler, *is* an interpreter, and *is* also a compiler.

[go tool]: https://golang.org/cmd/go/

</aside>

<img src="image/a-map-of-the-territory/venn.png" alt="A Venn diagram of compilers and interpreters" />

That overlapping region in the center is where our second interpreter lives too,
since it internally compiles to bytecode. So while this book is nominally about
interpreters, you'll learn compilation too.

## Our own journey

That's a lot to take in all at once. Don't worry. This isn't the chapter where
you're expected to *understand* all of these pieces and parts. I just want you
to know that they are out there and roughly how they fit together.

This map should serve you well as you explore the territory beyond the guided
path we take in this book. I want to leave you yearning to strike out on your
own and wander all over that mountain.

But, for now, it's time for our own journey to begin. Tighten your bootlaces,
cinch up your pack, and come along. From here on out, all you need to focus on
is the path in front of you.

<div class="challenges">

## Challenges

1. Pick an open source implementation of a language you like. Download the
   source code and poke around in it. Try to find the code that implements the
   scanner and parser. Are they hand-written, or generated using tools like
   Lex and Yacc? (`.l` or `.y` files tend to imply the latter.)

1. Just-in-time compilation tends to be the fastest way to implement a
   dynamically-typed language, but not all of them use it. What reasons are
   there to *not* JIT?

1. Most Lisp implementations that compile to C also contain an interpreter that
   lets them execute Lisp code on the fly as well. Why?

</div>