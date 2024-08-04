**TODO: when can we introduce a print statement/function?**

needs to happen before statements and flow control otherwise those aren't
visible to user.

- Warming Up
    - Introduction
        - who book is for
        - who am i
            - doodling languages in notebook
            - always fascinated
            - seemed like magic
            - iStudio
            - paternity
            - Dart
        - why learn languages?
            - in full programming career, will end up doing something related to
              language
            - good way to learn lots of techniques: recursion, trees, graphs,
              state machines, memory management, optimization
            - hard, training with weights on
            - fun
            - dispel magic
        - structure of book
        - languages used in impl
        - what's in book
        - what's not in book
        - end goal is high quality, efficient interpreter suitable for real use
        - to get there, narrow path through space, not broad survey
        - will point to alternatives to explore on own
        - learn enough to carry conversation with professional lang person
    - The Vox Language
        - intro to full language we'll be implementing
        - ebnf
    - The Pancake Language
        - basic phases and terminology of interpreter
        - simple stack-based language
- Practice (Java)
    - Framework
        - repl
        - interpreters run from source
        - test framework
    - Scanning
        - tokens
        - whitespace
        - regex
        - comments
        - numbers
            - leading zeroes
            - floating point
            - leading and trailing "."
            - range
            - negative
        - token value
        - strings
        - token type
        - escaping
        - errors
        - maximal munch
        - fortran parsing identifiers without whitespace
        - significant indentation
        - state machine for identifiers
        - ex: self-assignment and increment
        - ex: scientific and hex
        - ex: significant indentation and newlines
        - ex: escapes
        - eagerly scan to list of tokens
    - Parsing Expressions
        - ast
        - metaprogramming the ast types
        - recursive descent
        - lookahead
        - ex: "needs more input" for multi-line repl
    - Tree Walk Interpreting
        - evaluating operands
        - recursion
        - arithmetic
        - visitor pattern
        - aside: interpreter pattern is putting interpret methods on nodes
          - makes it possible to add new node types
        - values versus ast nodes for literals
        - dynamic typing and conversions
        - errors
    - Variables
        - statements versus expressions
        - declaration
        - assignment
        - variable references
        - scope
        - undefined names
        - block scope
    - Control Flow
        - if
        - and and or
        - while
        - for
    - Functions
        - parsing calls
        - '(' as infix operator
        - built in fns
        - user-defined fns
        - parameters and arguments
        - call stack
        - closures
        - ffi?
        - tail call optimization
        - arity mismatch
    - Resolution
        - compile errors
        - recursion and mutual recursion
        - decorating an ast
        - symbol tables
        - name binding
        - early versus late binding
    - Classes
        - classes
        - prototypes?
        - this
        - properties
        - methods
        - dynamic dispatch
        - constructors
    - Inheritance
        - inheritance
        - super calls
    - Lists and Loops
        - list type
        - subscript operator
        - subscript setter
        - for syntax
        - iterator protocol
        - desugaring
        - ex: make string implement protocol
        TODO: Cut this?

TODO: Still needs a lot of work:

- Performance (C)
    - Framework
    - A Virtual Machine
        - stack
        - for now, Value is just a double and OP_CONSTANT uses the argument as
          an immediate int value so we don't need a constant table
        - bytecode
        - hand-author and run some bytecode
    - Scanning
        - pull based lazy scanning
        - zero-alloc tokens
        - talk about state machine for keywords?
    - Compiling Expressions
        - top-down operator precedence parsing
        - single-pass compiling
        ^ can now compile and run arithmetic exprs
    - Representing Objects
        - numbers, strings, bools, null
        - dynamic typing
        ^ now can handle "str" + "another"
    - Garbage Collection
        ^ since previous chapter needs to heap alloc stuff, need to manage it
        - roots
        ??? we don't have any objects that store references to other objects
            yet, so there is no traversal happening
    - String Interning and Symbols
        - string interning
        - fast equality
        - hashing?
        - separate symbol types
        - intern all or some strings
        - gcing interned strings
    - Variables
        - statements versus expressions
    - Control Flow
        - branching instructions

    - Functions
        - upvalues

    TODO: other stuff...
    - constant pools
    - functions
    - symbol tables
    - nan tagging
    - copy down inheritance


principles

- each top-level section builds one interpreter starting from scratch
- since the book will be "published" online serially, the chapters should be
  ordered such that they are useful even while the book is incomplete. that
  probably means doing all of parsing for the whole grammar isn't a good idea:
  it's boring until later chapters do something with it.

- kinds of content in a chapter
  - main narrative with prose and code
  - historical context and people
  - further things to learn
  - omitted alternatives
  - review questions: ask things chapter did explain
  - challenges: add new features or compare other languages
  - quotation at beginning of each chapter
  - engineering considerations: error handling, maintainability, etc.
  - design and pyschology: usability, aesthetics, popularity, learnability, etc.

stuff to maybe include:

- error-handling
    - stack traces and line information
    - runtime errors
- variables
    - scopes as dictionaries
    - name binding of locals
    - variables and assignment
    - scope
- object model
    - objects as dictionaries
    - objects
    - classes
    - prototypes
    - nan tagging
    - object representation
    - symbol tables and hash tables
    - strings
    - arrays
    - hash tables (for internal use and as object in language)
    - dynamic dispatch
- syntax
    - aesthetics and usability of syntax design
    - backjumping and infinite lookahead or context-sensitive grammars
