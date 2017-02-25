^title Parsing Expressions
^part A Tree-Walk Interpreter

- first major milestone of book
- parsing scary reputation
- being able to write "real" parser, not cobbled together out of regex
  and substrings...
- after this chapter, will have crossed that threshold

- easier than you think
- previous chapter front loaded most of hard work
- already familiar with formal grammars
- have grammar for syntax trees
- have java classes to represent them
- what's left is parsing

- aside: "parse" comes from old french "pars" for "part of speech"
    - about taking text and understand how each part maps to grammar -- what it
      represents

- some textbooks make big deal out of parsing
- back in 60s and 70s, parser tech was hot area of research
- languages like algol and fortran were designed on paper and literally didn't
  know how to make computer correctly recognize them
- took lot of hard research to figure out how
- especially on limited machines of the time
- came up with multiple brilliant techniques
- like any hero, wanted to erect monument to success
- cover of dragon book is literally that
- knight is syntax directed translation, wielding lalr parser generator
- stuff is interesting
- but honestly don't need to know most of it
- they just were so proud that they couldn't help but tell you

- we'll do much simpler: one technique
- recursive descent
- simplest way to write a parser
- does not mean toy!
- rec desc often very fast, and easy to add robust error handling
- most real-world parsers for industry langs i know use this
- gcc, roslyn, v8, dart, etc.

## parsing

- playing game in reverse now
  - starting with string, figure out which rules were used to generate it
  - grammar is ambiguous
  - show arith expression and illustrate two valid parse trees
  - show how you can play grammar two ways to get same string

- create different rule for each precedence level
- hierarchy explicitly encodes precedence
- (some leave it ambiguous in official grammar and rely on extra data on side
  to disambiguate)
- lhs of a "*" expr is an expr, but not any expr
- can't have "+" expr there, at least not without parens
- so really, using "expr" in grammar isn't precise enough
- lets subexprs of lower precedence sneak in
- stratify to fix that

```lox
expression → equality
equality   → comparison ( ( "!=" | "==" ) comparison )*
comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )*
term       → factor ( ( "-" | "+" ) factor )*
factor     → unary ( ( "/" | "*" ) unary )*
unary      → ( "!" | "-" ) unary
           | primary
primary    → NUMBER | STRING | "true" | "false" | "nil"
           | "(" expression ")"

```

- prod for each prec means "this kind of expr or one of higher prec"

- aside: Explain difference between parse tree and AST now that we have intermediate
productions for handling precedence that aren't stored in tree.

## recursive descent

- many parsing techniques
- no lalr, etc.
- dragon book cover
- recursive descent
- simple
- good for real world
- hand writable
- each prod becomes fn
- (don't have to map exactly)
- "descent" because higher level prods call lower level ones
- entrypoint fn is top of grammar
- "top down"
- "recursive" because when grammar rules refer to self, recursive calls
- translate grammar to imperative code
- terminal becomes expect
- nonterminal is call
- repetition is loop
- option is if

## parser class

- shell of class

^code parser

- like scanner, consumes sequence
- this time tokens instead of chars
- emits syntax trees

- let's translate grammar
- entrypoint (until have statements and decls later) parses expr

^code parse-expression

- prod for that grammar rule just falls to equality, so body just calls that


^code equality

- parse lhs as comparison prec
- repeated part uses while loop
- each rep must start with "==" or "!=", so we use that to tell if we have
  equality expr
- if so, expr rhs, so parse that
- becomes new result then loop
- eventually run out of equals ops and exit loop, return

- uses helper fns, much like scanner had

^code match

- checks to see if next token is one of given types
- if so, consumes it and returns true
- otherwise returns false and leaves token there

- in terms of

^code check-and-advance

- check looks at type of next token and returns true if it is type
- does not consume
- advance consumes current token, just like advance() in scanner for chars

^code utils

- bottom out on last batch of helpers
- is at end returns true if reached end of token stream
- peek looks at current unconsumed token
- previous gets token just consumed
- makes it easier to use match and then access the just-matched token

- most of the infra we need
- missing fn is comparison

^code comparison

- grammar rule is similar so code is similar
- exact same structure
- only difference is which fn is called for operands, and which token types
  are operators
- (clever use of java could make those params and have single fn for
  parse binary)

- remaining two binary are same

^code term-and-factor

- see how prec is handled now
- since lhs of term calls factor, can never parse "1 < 2 + 3" where "1 < 2" is
  lhs of plus

- binary is done, on to unary

^code unary

- again, look at next token to see if we have unary expr or primary
- if matched, call unary again
- unlike binary, use recursion for repetition here
- could use loop, but would have to remember stack of operators
- don't know what inner primary expr to wrap until very end
- finally hit last grammar

^code primary

- mostly simple
- match all the single-token literals
- parens also mostly straightforward
- but here, finally get to interesting case
- consume()
- can have parse *errors*

## parse errors

- if matched "(" and parsed expr inside grouping, *must* find ")" after it
- parser has two jobs:
    - convert valid token sequence to syntax tree
    - tell user if token sequence is not valid
- equally important!
- especially in ide while user is typing, code is often in invalid state
- good guidance helps get it correct again

- good error handling is hard
- no perfect answer: code is by definition not in state where user's intent is
  clear
- parser can't read your mind
- if code is wrong, who knows what you meant to write?

- couple of hard requirements
  - must detect and report any syntax error
    - can't ignore it and let interpreter try to run malformed code
  - must not crash or hang
    - syntax errors happen all the time, tool must be robust
- soft requirements
  - fast
  - report error as close to where code goes bad as possible
    - early in token stream
  - if code contains multiple separate errors, report as many as feasible
  - minimize cascaded errors
    - after first error, parser not sure where it is any more and gets confused
      by later actually correct code

- last two in tension
  - trying to report more errors usually increases chance of mistakenly
    reporting cascaded errors
  - avoiding too many cascaded ones can mask real errors

- lots of research into "error recovery" in 60s
- when had to submit program to compile overnight and check results next day,
  really wanted parser to find every single error
- now parse times in ms
- simple error recovery is fine

### panic mode error recovery

- winning strategy is "panic mode"
- as soon as error recovery is found, parser reports it and "synchronizes"
- gets its own state and current point of token stream into alignment at
  well-defined point in grammar
- to get own state in sync, jump out of any nested prods back to high level
  rule syncing at
- then sync token stream by discarding tokens until reach token that begin
  prod at that level or until after token that can end it

- canonical sync point is at end of statement
- don't have statements yet, so won't actually synchronize
- but put stuff in place for when we do

### implementing error handling

- where were we?
- consume()

^code consume

- sort of like match(), first line of code same
- but if doesn't match, error
- we know error is on this exact token because it's not what we looking for,
  so use peek()
- calls

^code error

- little helper
- tells main lox class to show user error, then returns (not throws)
  parse error
- over in lox class, add:

^code token-error

- convenience method to report error at position of given token

- now user knows about error
- what does parser do next?

- error returns parser error
- parse error class is

^code parse-error (1 before, 1 after)

- simple sentinel class parser can catch
- code that calls error controls whether or not this is thrown
- if not, discarded

**TODO: aside on error productions**

- some errors occur in places where parser not likely to get tripped up
- for example, if fn call has too many args, still easy to parse the extras
- interp just doesn't support them
- there, parser can report error and just keep going

- other syntax errors nasty enough that parser needs to synchronize and clear
  its head
- to sync token stream, discard tokens
- how do we sync parser state?
- in rec desc, parser state is implicit in callstack
- "where" parser is exactly defined by which methods are on the call stack
- to get out of deeply nested state, need to escape those methods
- exceptions!

- when want to synchronize, throw parse error
- higher up in grammar, catch that
- now we know what state parser is in
- then synchronize token stream for that state

- when have statements, we'll catch exception there, so want to sync to
  stmt boundary
- easy to spot
- after ";" probably finished statement
- many statements start with keyword: `if`, `while`, `return`, etc.
- if next token is any of those, probably about to start one

^code synchronize

- discards tokens until either after ";" or before stmt-starting keyword
- after catching ParseError, call this
- then parser and token stream (hopefully) in sync
- discarded any tokens that would have likely causes cascaded errors

## wiring it up

- mostly done
- one other place need to add error handling
- parser bottoms out on primary expr
- what if none of those cases match?
- means we have token have no idea what to do with
- that's error too, so add to end of primary

^code primary-error

- now can hook up parser to lox class and try out
- don't have interp yet, so main just parse and print ast

^code print-ast (1 before, 1 after)

- make method to parse expr

^code parse

- will get replaced when add statements
- catches parse error
- (don't have to worry about returning null, not used if hadError)
- lox has to check hadError and not use parser result
- parser promises not to crash or hang on bad input, but does not make any
  guarantees about returned syntax tree when it happens

- if syntax is correct, parser returns tree for it
- use last chapter's ast printer to print it so we can see if it parsed right
- try it out
- does it handle precdedence and correctly?

- challenges
  - add "--", "++", "|" and "&" to grammar
    - where should latter two go in grammar? is where c puts them right?
  - add "?:"
  - add error productions for some common errors

**TODO: design note on familiarity versus idealism. should fix c grammar mistake for "|"?**
