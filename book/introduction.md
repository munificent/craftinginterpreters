^title Introduction
^part Welcome

- really excited going on adventure together
- book about implementing programming language interpreters
- also about designing languages
- wanted to write book for years

## goal

- walk step by step through implementating complete language
- assumes you know how to code, but have never touched lang stuff before
- from there, teach to get real working, usable lang impl
- lot to cover, so approach is not broad survey, but guided tour
- will point to interesting things off path for you to explore, but not go
  there
- book should be good starting point for you to later go off the path

- language is minimal, but not a toy
- to be useful for real code, want to add some stuff like collections and
  core library so you can do things like read and write files
- but language itself is full-featured
- dynamically typed, imperative
- complete syntax similar to see, all operators and control flow you expect
- global and local block scope
- as we'll see, implementing variables correctly and efficiently will take a
  lot of our attention
- has functions
- first class, and closures
- classes, methods, fields, and inheritance

### two impls

- actually walk through two implementations
- first focuses on concepts
- get you familiar with the phases and pieces of interpreter
- give us chance to introduce techniques use in other language related tools
  around static analysis, etc.
- also get you to think more deeply about corners of language semantic
- by end have a good picture of all of the things implementation needs to do
- but lean on implementation language, java, for many of them: gc, object
  representation, etc.
- end result is a simple, minimal, readable impl, but not a fast one

- then implement it again, in c
- now learn how language really works, from bits up
- write compiler to bytecode
- same technique used by lua, python, ruby, others
- learn how dynamically typed langs represent objects in memory
- write our own memory manager and mark sweep gc
- implement strings and hash table from scratch
- see how all of the state interpreter needs, globals, locals, callstack are
  maintained
- do some benchmarking and optimize hot spots
- by end, we'll have robust, accurate, fast interpreter

### blue collar

- focus of book is on how real-world interpreters implement real langs
- less about theory of pl
- will talk about history of pl concepts, and formal stuff
- also introduce terms used in pl academia
- if find self at cocktail party with pl researchers, should be able to keep up
- but mostly focus on understanding how to actually code lang
- name is deliberate
- not to say theory isn't imporant, but covered well by other books

- personally, learn better by doing
- hard for me to read pages of abstract concepts and reall get it
- but if i've written code for something, debugged it, ran it, can watch it
  work, then i get it

- goal is for this to be great first book on langs, hopefully not last
- then when read later more academic texts, hopefully will seem familiar and
  click more easily

## why learn langs

- odds of creating successful general purpose language are slim
- all of them in the world could fit on one bus
- ok, lots of other reasons to learn languages

### little language

- for every successful general language, hundred successful "little
  languages" or "dsls"
- embedding script, template languages, markup formats, configuration
  languages, etc.
- often good to reuse one when you can, creating new one from scratch can
  be lot of work around tooling, error handling, etc.
- but may find yourself needing to whip up a parser or something when there
  isn't one out there that does what you need
- or occasionally may find imperative bytecode is good way to model some
  kind of problem

### good exercise

- langs are complex and often perf critical
- good environment to learn about fundamental data structures and algorithms
- heavy use of trees and recursion
- will be totally comfortable with recursive data structures and algo by end
- when presented with problem, easier to see when recursion will help

- implement dynamic array, hash table, even string from scratch
- rarely implement them in day-to-day programming, but use them all the time
- will understand them in and out, which help you use them well and understand
  how and why they work and limitations later

- also just kind of hard
- not as hard as you might fear, but good workout
- like runner practicing with weight
- will come away a stronger programmer

### change life

- embarrassing to admit
- secret goal of book
- remember being a kid learning to program and not being able to imagine
  how languages worked at all
- always felt like programming languages were "magic"
- dragon and sicp covers don't help
- problem with "magic" is that inherently elitist
- only special other people, wizards, can do it
- that's what makes it magic
- sufficiently well-understood magic indistinguishable from technology
- to a wizard, it's just craft
- makes it unapproachable, bounds you
- people who hack on langs aren't special
- it's just some crafts and techniques and programmer can pick up
- some are little hard, but harder in way that running 500 meters is harder
  than 100
- hope that by learning them yourself, also learn that those divisions you
  see between you and other kinds of people who have special skills are
  imaginary, and you can push through them just by learning
- you can learn anything

- who knows, maybe you will create the world's next great language
- someone has to

## prereqs

- can't teach you everything, book would be too long
- assume you already know how to code
- should know how to create project, organize code and compile and run it
- should know basic imperative programming constructs, oop, and terminology
- if you can write a basic app on your own, probably fine

- use two languages in book, java and c
- java is for first part
- good language for teaching concepts
  - high level enough don't have to worry about memory management, etc.
  - has nice built in collections
  - at same time, explicit about what's going on
  - less magic under the hood so if you write it in java, feel you really do
    understand it
- also, oop
  - paradigm is familiar to millions of working programmers, so lets us
    start from familiar place to you
  - many people working in langs are implementing tools in oo
    languages, so good to know how lang patterns and techniques map to oo.
  - (lots also using functional languages, but plenty of books for that.)
- hugely popular, so good chance you know it or similar lang
  - try not to use too much of it
  - use diamond operator in java 8, not much else
  - if know other oop language, can probably muddle through ok

- c
  - second half is c
  - great for understanding what's really going on in implementation down
    to cpu and memory
  - teaches us stuff java gives us for free: gc, object representation
  - big reason using c is to do things c specifically expresses well
  - require more c expertise
  - use preprocessor
  - don't have to be c wizard, but should be comfortable with pointers
  - will learn a lot of c from book itself
  - many/most real language interpreters implemented in c: lua, python,
    ruby

- hands-on book

## how to read book

### parts

- organized in three parts
- first is one reading now
- get you up and running and give you sense of how things organized and where
  we're going, and lang we're going to focus on
- second implements lang in java, piece at a time
- first starts over again in c

### each impl

- each impl is built one chapter at a time
- took a lot of trial and error on my part, but managed to break interps into
  chapter-sized chunks such that by the end of each chapter, have something
  you can run
- grow interpreter
- side effect of this is order that concepts are introduced might be a little
  odd
- have to break dependency cycles sometimes by splitting concept into two
  chapters
- for example, in java interp, will add variables, then functions, then circle
  back to variables once we see how closures complicate how we handle scope
- likewise, in c version, need to start allocating memory very early on, but
  don't talk about garbage collecting it until much later when we have a complex
  enough web of objects in heap to make gc interesting
- but tried to order things as naturally as i could, and mostly worked out ok

### each chapter

- each chapter is hands on explaining concepts but also each line of code
  needed to write it
- every single line of code needed for both interpreters is in book
- if you type them all in, you'll get the whole thing and it will run and pass
  the tests

- tried to keep code as simple and clean as possible
- gets back to goal of "no magic"
- if you understand every line of code, and there is nothing missing, you have
  totally mastered it
- [aside about not using compiler-compiler too]

- style of code little different from "real" code
- needs of a book are a little different
- in java, ditch some of the modifiers like public and tend to make fields
  public to keep things terse
- page isn't as wide as ide window
- in both langs, code is not super commented
- that's because each line of code here also has a few paragraphs of code
  explaining it!
- if you write a book for each of your programs, you can omit the comments too
- otherwise, should use "//" more than i do here

- chapters have a few other sections
- asides like this one are dumb jokes, historical notes, references to related
  topics, etc. can skip if you want

- exercises are to help you learn more
- not just review of what's in the chapter, you don't need me to do that
- encourage you to step off the guided tour of this book and try to figure out
  something on your own, or do your own research
- some may be easy, barely more than rhetorical question. others may ask you to
  add major feature to language
- often ask you to learn how other languages solve problem
- when designing language, encyclopedic knowledge of other languages is big
  asset

### design breaks

- many chapters also end with design break
- most other books assume lang is given and just trying to implement it
- this book sort of does that too, i already designed lang we're going to impl
- but will also talk about lots of issues come up when designing language
- how to balance features, grow ecosystem, think about usability
- softer side of pl, but may be more important
- nice-to-use langs with crappy impls successful (php), but converse rarely
  true. world doesn't need great impl of bad lang.
- alas, expertise in this rare
- few people designed successful lang, and most haven't written much down
- may not even know how they do it
- what i know i've learned from studying successful langs, trying out my own
  and seeing how people respond, talking to lang designers
- try to give good advice, but take with grain of salt
- not something many people know much about
- if just want to hack lang, feel free to skip design parts

### how to read

- that's what book is, but not how to *use* it
- up to you
- one of most important skills you can learn is how *you* learn
- if you want to really understand the code, type it all in and run it
- if you're "what if" person, try tweaking the code to change the language or
  how it works. experiment and see what happens
- if you just want concepts, you can read the prose and skip the code
- maybe you like a challenge and want to read the prose but try to implement it
  yourself from scratch without looking at the code
- maybe want to follow what piques your interest and read chapters out of order
  based on what gets you excited next
- all are fine. it's your book, and your job to find out how to make the most
  of it

## where we are going

- before dive into our language, good to talk about basic terminology and how
  most impls are organized
- tricky because, especially in pl, words change in meaning over time, or were
  named in a way that made sense 40 years ago but seems strange today now that
  computers are very different
- even simple question like "what is difference between compiler and
  interpreter" get very fuzzy
- don't even ask about "strong" versus "weak" typing

**TODO: talk about difference between lang and lang impl**

- use metaphor of climbing over a mountain
- every impl starts off with program as bare source text, literally just string
  of characters
- analyzes that syntax, understanding it at a higher and higher level,
- eventually reaches peak where we think of it as understanding semantics of
  language, what programmer intends it to do
- then it goes back down mountain, lowering those semantics into something it
  can make the underlying cpu actually execute
- couple of branches in path, and some impls skip some steps, but remarkably
  similar across langs

**TODO: mountain picture**

### scanning

- first step up mountain is scanning, lexing, or lexical analysis. all mean
  same thing (as far as i can tell)
- take in source code as sequence of individual characters, and chunk into
  "words" called "tokens". some tokens, like "(" or "," are single character.
  others, like identifiers and keywords, are multiple. scanner's job is to get
  those multi-char chunks and make single token out of them.

**TODO: pipeline picture**

### parsing

- [aside about programmers drawing trees upside down]
- next step is parsing. where syntax gets a grammar. if ever diagrammed sentences in english class, have already done this
- (note, some impls combine lexing and parsing)
- take linear series of tokens and organize them hierarchically into tree
- parsing is where we take a + b * c and figure out that b * c is evaluated
  before it is added to a
- also where most "syntax errors" get reported

**TODO: tokens to tree picture**

### static analysis

- first two stages pretty similar for almost all langs, now start to vary
- at this point, know syntax -- expressions, operator precedence, etc.
- but know little about meaning
- now start doing deeper analysis
- most langs do "binding" or "resolution"
- for each identifier, figure out what it refers to
- where scope comes into play -- region of code where identifier can be used
- before, just knew "a + b" was addition, now know adding two variables
  declared here and here
- often involves creating symbol table, that maps names to what name refers to
- can report errors here too, use undefined variable, use variable after out
  of scope, etc.

- if language is statically typed, type checking happens here too
- determine type of each var and expression
- check that operands are valid for arith, function calls, method calls, etc.
- lots of error reporting
- our lang is dynamic, so type checking happens later, while running
- [even static langs push some checking to runtime, covariant arrays, casts, etc.]

- peak of mountain
- now really understand everything we can about code without actually running
  it

- now going down mountain to get closer and closer to something we can execute
- phases up to here called "front end"
- everything past this point generally referred to as "compilation" or "back end"

### intermediate representation

- along the way, may translate code into one or more intermediate representations
- often just take tree object produced by parsing and add bits of info to it
- here is variable referred to, here is type
- but may convert tree entirely to different data structure more useful for
  what later phases want to do with it
- goal of each phase is to produce data organized in way to make next phase
  easier
- especially important for compiler optimizations
- for those, want simple intermediate representation so don't have to optimize
  lots of different code shapes
- so often "desugar" or "lower" to simpler, smaller language
- also often want to encode invariants so it's easier to tell when opt is safe
  to perform
- ssa and cps
- not doing in this book, many interpreters skip this

### optimization

- advanced compilers do many phases of optimizations
- now that understand semantics of program, can freely transform it in ways
  that are more efficient as long as preserve semantics
- efficient hard to define these days: faster, less code, less energy
- many many opts, dead code elimination, constant folding, strength reduction,
  etc.
- many profession lang hackers spend almost entire career here

### code generation

- applied optimizations, have code that does right thing and does it efficient
- last step is converting it to form can execute
- traditional compiler generates native assembly or machine code, language cpu
  itself executes
- gives you an executable os can directly run
- specific to cpu architecture

- other languages generate code for artificial cpu, virtual machine. machine is
  then emulated in software
- this code for fake cpu usually called "bytecode"
- historically "p-code"
- bytecode is simpler to implement—machine code pretty complex, easier to debug
- also portable
- but loses some speed in emulation

- some langs do both, translate to bytecode, then later translate that to
  machine code

### runtime

- finally have turned user's program into something can run
- last step is running it
- if compiled to machine code, can just tell os to run it and off you go
- if compiled to bytecode, need to load it inside vm and tell vm to run
- in both cases, for all but really low level languages, also need some services
  that language defines are available at runtime
- for example, if language automatically manages mem, need gc
- if can do dynamic dispatch or type tests at runtime, need rep to track types
  of objects at runtime
- extra baggage is called "runtime"
- in fully compiled language, gets compiled into executable. each compile prog
  carries runtime with it. go, haskell
- if lang is run in vm, vm usually contains it. java, python, etc.

### transpilers

- other shortcut path through mountain
- most langs that "compile" or translate translate to some explicitly low level
  lang no one would want to hand code: assembly, machine, bytecode
- some langs compile to high level lang
- c was standard
- was good way to write portable compiled lang since c was already ported to
  most oses
- now thanks to browser hegemony, js is very common
- lets get lang in browser
- used to call "source-to-source" compiler, but "transpiler" hip term
- scan and parse like other langs
- then, depending on how similar source semantics are to dest lang's, may output
  dest source after any step, analysis, opt

## phases and tools

- those are different parts likely to see in any lang
- not every lang has every part
- not every lang runs part at same time
- important distinction is "compile time" versus "runtime"
- maybe more accurate "dev time" versus "user time"
- which phase runs on developer's machine when creating prog versus on end
  user's machine each time prog is run

- in traditional "compiled" language like c or go, every phase runs on dev's
  machine
- end user gets fully compiled app that just executes
- end user may not even have access to source
- called "ahead-of-time"

- in "scripting" language like js, python, ruby, every phase runs on user's
  machine
- they have raw source code sitting on disc (or pulled off web) and lang impls
  runs through whole process above each time program is loaded

- languages in the middle. on dev's machine, compiler compiles to compact
  bytecode rep, which is shipped to user
- user runs that bytecode in vm
- java, c#

- some langs that compile all the way to machine code, still defer some of that
  until point that it runs on user's machine
- may ship user source or bytecode, but then at very end, translate that to
  (usually opt) machine code for end user's cpu
- called "jit"
- often do this multiple times with different opt

## so what is "compiler" and "interpreter"

- so lots of langs have different subsets of phases, and divide them differently across dev's and user's box?
- how to map to two simple terms?
- first thing, realize term refers to lang *impl*, not *lang itself*
- scheme is not a compiled lang or interpreted lang
- some scheme impls compile, some interpret (most do both)

- second, "compile" refers to two concepts

- if lang impl has any of back-end phases, it's doing "compilation"
- if there is dev tool that takes source and translates to separate
  form, and that later form is what end user runs, that first tool is a
  "compiler"
- if translated output is user visible before code runs, "compiler"
- [what about go run?]
- conversely, if there is tool that take source and can execute it directly
  usually called "interpreter"

- in some cases, two are combined and term is obvious
- in go and c, you run separate tool to translate source to machine code
- tool is compiler and does compilation, easy

- likewise, if tool takes source and runs it without any translation, obviously
  interpreter
- no compilation anywhere
- ruby used to work this way
- our first lang impl will
- "tree walk interpreter"
- [old basics didn't even parse before running, they would parse each line on
  the fly every time it executed]

- where it gets fuzzy is many lang impls run from source but have a back end
  that compiles on the fly
- cpython, ruby, lua all compile to bytecode and execute
- most js engines in browsers compile to bytecode then later machine code and
  execute

- user can't see compiler, but it's there
- for these, say *is* interpreter, because that's how user uses it
- but say it *has* a compiler internally, because important implementation
  detail

- our second impl works this way
- so is book on interpreters, but we'll also learn compilation too

**TODO: illustration showing various lang impls and which phases they have and which run on user's machine**

## recap

- same phases appear across almost all lang impls, though some omit some phases
- scanning, parsing, analysis all form front end
- ir, opt, code gen, all back end
- if impl has any back end phases, it is or has a compiler
- if lang impl runs from source, user probably thinks of it as "interpreted"
- most interpreters have compilers internally

- remarkably stable across years and langs
- stumble onto any codebase for lang impl from fortran to swift can probably
  find file called "parser", etc.
- you have your map now, let's explore
