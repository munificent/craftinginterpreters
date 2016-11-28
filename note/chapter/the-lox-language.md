--

Note that we're using keywords for "and" and "or" instead of "||" and "&&"
since we don't define the bitwise forms and it's weird to lex "||" without "|".

--


- to full spec language, need to define
- which strings of characters are valid programs
  - (and if not, maybe what kind of errors should be shown)
- for programs that are valid, what prog means -- what it does when you run it

- very hard to do
- if wanted to spec, say, traffic light, pretty straightforward
- could describe all possible states light can be in and what it should do
- with lang, input is combinatorial
- user can compose pieces together arbitrarily
- even if lang limited source files to 1k, more possible programs than grains
  of sand in every beach on every aquatic planet in universe
- what we love about langs: open ended up
- but makes spec hard
- can't list them all!

- instead, lean on composition
- specify what atomic parts of prog mean
- specify so that meaning is defined when composed
- then all programs are well defined

- lots of different ways and notations for doing this
- roughly work like impl pipeline we saw last chapter
  - lexical grammar
    - how chars become tokens
  - syntax grammar
    - how tokens become trees
  - semantic analysis
    - type checking
  - semantics
    - denotational et. al.
- main difference is that impl is usually imperative but spec is usually
  declarative
- have notation really like for first two phases
- type checking has really nice formalism too, but don't need for lox
- for semantics, lots of formalisms in academia but don't see them used much
  outside
- personally just like crisp prose

- teach you notation for tokens and grammar
- then go through each grammar rule and hang semantics off it
- still not totally precise, later chapters will nail down corners


- start with examples!

- context free language
- chomsky
- https://en.wikipedia.org/wiki/P%C4%81%E1%B9%87ini
- https://en.wikipedia.org/wiki/Syntax_diagram
- terminal and non-terminal
- "An important tenet of denotational semantics is that semantics should be compositional: the denotation of a program phrase should be built out of the denotations of its subphrases."
- https://en.wikipedia.org/wiki/Denotational_semantics

- when defining something -- syntax, semantics, etc -- have few competing goals
  - correctness - definition must mean what you want
  - completeness - can't leave important behavior undefined
  - brevity - don't want it too verbose
  - relevance - should only define things that matter, not implementation
    details or other irrelevant bits
  - clarity - should be easy for human to understand

- key goal is to be compositional
  - inputs to pl are combinatorial
  - space of possible programs larger than number of atoms in universe
  - can't test exhaustively
  - have to be able to define correctness in composable way

- three areas of specification
  - syntax (sub areas lexical and grammatical)
  - static semantics - type system
  - runtime semantics
  - formalisms for each one
  - syntax pretty well defined
    - context free grammars
    - chomsky hierarchy
    - bnf
    - railroad diagrams
  - type system pretty well defined
    - won't go into here
  - lots of formalisms for runtime semantics
    - denotation semantics
    - operational semantics
    - seem to get lot of use in academia
    - haven't seen them used as much outside
      - (though industry tacitly presumes things academics laborious proved)
    - won't do anything formal
