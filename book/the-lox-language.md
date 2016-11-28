^title The Lox Language
^part Welcome

**TODO: Appendix with full grammar.**

- going to spend rest of book implementing lox
- each chapter will drill down and really explain how corner of lang works
- don't want to do that all up front
- but do want you to be familiar with lang overall
- if want to follow along at home, can build lox from my repo
- think of rest of book as specification for lox, this is tutorial

## Hello, Lox

- first lox prog

```lox
print("Hello, world!");
```

- c-ish syntax
  - not to say c's grammar is that great
  - other language's simpler and cleaner
  - very very familiar, you probably already know it
  - similar to languages using to implement it, java and c
  - when implementing lang, don't want to dodge all the tricky parts!

- most similar to js
  - very simple lang
    - 10 days
  - lots of big apps written in
  - try to dodge most of js's worst misfeatures

- other big inspiration, especially in impl, is lua

## Data types

- start on bottom, work our way up
- numbers
  - everything double
  - keeps it simple
  - tolerable for js and lua
- bools
  - true and false
- strings
  - literals
  - escapes
  - encoding
- nil
  - not "null" to be clearer in impl
  - billion dollar mistake
  - comes up in missing return and uninitialized var
- gc
  - don't have to manage memory for objects
  - most modern languages are
  - need memory safety for dynamic typing
  - hard to do memory safety without memory management
  - ref counting slower, can't handle cycles
  - notoriously scary, want to get rid of magic
  - gc is fun to implement!
- these are built-in data types, get to user-defined below

## Expressions

- arithmetic
  - basic arithmetic expressions know from other languages
  - + - * /
  - called "binary" or "infix"
  - also negate
  - called "unary" or "prefix"
  - all work on numbers
  - runtime error to use any other type
  - few implicit conversions like js

- comparison
  - > < >= <=
  - also only work on numbers
  - produce a bool result

- equality
  - == !=
  - work on any type
  - values of different type are never equal
  - no implicit conversions
  - value equality for built-in types, even strings
  - like comparison, produces bool result

- precedence follows c (will be more precise later when get to parsing)
- can use "()" to override precedence

- ditched a few other operators for simplicity: ++, --, ?:, %, etc.

## Statements

- where expressions produce value, statements produce effect
- usually modify some state, produce output, etc.
- top level of program is list of statements

- seen a bunch already
- look like expressions but end in ";"
- that's expression statement
- lets you put expr where statement is required

- semicolon as terminator not separator
- blocks

## Variables

- declare using "var"
- optional initializers (nil if omitted)
- global and local scope
  - collision
  - shadowing
  - go into lot more later

- access using name expr
  - name characters
- assignment
  - just identifier on lhs (for now)

- dynamic typing
  - when declaring vars and params, no types mentioned
  - could technically mean types are inferred, but no
  - variable can hold value of any type
  - can change
  - why?
    - most scripting languages are
    - static types very big topic by themselves
    - get something up and running sooner
      - don't have to implement type checker before can run simple progs
    - generally simpler
    - also widely user for real progs
    - not to say it's overall better: note that implementing lox using two
      static langs

## Control Flow

- hard to write useful programs if you can't choose not to do something or do
- something more than once
- same control flow statements as c, java, et. al. more or less

- if
  - because dynamically typed, have to handle non-bool conditions
  - nil and false are falsey, everything else truthy
  - same as ruby
  - then and else branches take statements
  - usually block but most statements allowed
  - else is optional

- or and
  - here because they short circuit
  - not "&&" and "!!" since no bitwise
- while
- for
  - simple c-style
  - keep things simple

## Functions

- calls look like in c
- parameters and arguments
- parens mandatory unlike ruby

- define using "fun"
- return statements
- recursion
- mutual recursion at top level
  - why globals late bound
- first-class
  - but no lambda syntax (could easily add)
  - **TODO: Add lambdas?**
  - keep it simple
- closures
- lox is "functional" for some definitions

## Classes

- lox is object-oriented
- why?
  - two question: why put in language in general and why put in lang in this
    book?
  - in general
    - oop hugely successful
    - despite backlash from overuse, oop is awesome
    - need some way of defining user-defined aggregate data types
    - being able to hang methods off it and dynamically dispatch avoids
      long function names like hash-remove in scheme
  - for book
    - few language impl books cover it
    - gives us reason to cover hash tables and dynamic dispatch
    - lots of fun
- two flavors of oop, classes and prototypes
  - no classes in prototypal
  - object can inherit directly from other arbitrary object
  - self, then lua
  - got big in js
  - all languages that heavily focused on minimalism
- in particular, class flavor of oop, why?
  - prototypes really fun for lang implementer
  - so simple and clean
  - give user prototypal language, what do they use them for?
  - ... implement class system
  - pushes complexity onto user "waterbed"
  - users seem to think in terms of kinds of things
    - can give better user experience if bake classes right in

- constructing instances
  - call class like fn
  - initializers
- methods
- properties
- this
- inheritance
- super calls
- first-class classes
  - in fact, classes and fns are interchangeable

- not pure oop
  - built-in types not instances of classes
  - because we're implementing language one chapter at a time
  - better language would be pure oop
**TODO: omit this?**

## Standard library

- saddest part of lox
- trying to keep things small for book
- absolutely minimal std lib
- seen print() already
- can't do anything without it: no output
- also time()
- for benchmarks
- all we need for book
- to make useful language, need lot more!
- but adding more not that technically interesting, mostly just shuffling data
  around
- so omitted from book

- challenges
  - using build of lox interpreter from my repo, write some example lox programs
  - list some questions about language semantics that above leaves open
    - (calling fn with too many or two few args, precedence, errors, dangling
      else)
- design note
  - statements or not?
  - implicit return

<!--

```lox
class List {
  List(data, next) {
    this.data = data;
    this.next = next;
  }

  map(function) {
    var data = function(this.data);
    var next;
    if (this.next != null) next = next.map(function);
    return List(data, next);
  }

  display() {
    var list = this;
    while (list != null) {
      print(this.data);
      list = list.next;
    }
  }
}

var list = List(1, List(2, List(3, List(4, nil))));
list.display();

fun double(n) { return n * 2; }
list = list.map(double);
list.display();
```

-->