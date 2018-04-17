^title Scanning on Demand
^part A Bytecode Virtual Machine

**todo: quote**

**todo: illustrations?**

- clox roughly three phases
- scanner, compiler, vm
- scanner and compiler comm with tokens
- compiler and vm comm with chunks
- started at back end with chunks and vm
- now front end scanner and tokens
- next chapter, middle

**todo: illustrate phases scanner -> tokens -> compiler -> chunk -> vm**

- not most exciting chapter
- mostly similar technique as java scanner
- few interesting differences scattered
- some driven by mem manage in c
- get started

## app skeleton

- building front end so can start get work like real interp
- repl and run scripts
- like jlox

^code args (2 before, 1 after)

- clox run from cmd line
- if pass no arg, repl
- if one, run script at path
- otherwise error
- [args[0] is executable in c]

^code main-includes (1 after)

- always need indluces
- repl easiest

^code repl

- rudimentary
- only single-line
- hardcoded line length

^code line-length (1 before, 1 after)

- real work is interpret()
- get there soon

### running scripts

- if pass path, load file and run as script

^code run-file

- readFile() dynamically allocs string, so free here
- use result of interp() to determine exit code

- helper

^code read-file

- reads the contents of the file at path
- need to manage mem for string
- how big?
- depends on file
- trick: seek to end of file
- get current file pos -> length of file
- seek back to beginning

- alloc string that size
- [+1 for terminator!]
- read into buffer, just right size

- close and return

- what if error?
- most c stdlib fns can fail
- need to handle

^code no-file (1 before, 2 after)

- if couldn't open file at all

^code no-buffer (1 before, 1 after)

- if couldn't alloc enough mem for file
- [unlikely, but would crash hard if didn't]

^code no-read (1 before, 1 after)

- read failed
- also unlikely
- if make it through these, return string with script
- runFile() passes to interp

## compilation pipeline

- old interp() was temp to run hard-coded chunk
- change to closer to real

^code vm-interpret-h (1 before, 1 after)

- job now is to wrap entire exec pipeline
- take in raw source, compile, run

^code vm-interpret-c (1 after)

- first step is compile
- not full compiler this chapter, but start laying out
- uses new module

^code vm-include-compiler (1 before, 1 after)

- looks like

^code compiler-h

- signature will change, but good for now
- impl

^code compiler-c

- first phase of compile is scanning
- compiler right now just sets that up
- lot of scaffolding

### scanner

- start get to real code
- new scanner module

^code scanner-h

- entrypoint fn sets up scanner to scan given source code
- impl

^code scanner-c

- scanner is stateful
- similar to vm, store all state in struct
- create single global instance so don't have to thread through fns

- few fields
- start is pointer to first char of current token being scanned
- point directly into source string
- current is point to current char being scanned
- next char to be consumed
- line is current line number
- as in jlox, track line info through compilation for reporting compile and
  runtime errors
- all we need

- don't even store pointer to beginning of source
- scanner walks through string once and never again
- doesn't need to remember beginning

- initialize

^code init-scanner (1 before)

- start pointers at beginning of source code
- on line one

## a token at a time

- finally ready to start doing work
- feel like beginning of concert
- set up sheet music
- adjusted bench
- sit down
- shoot cuffs, adjust skirt
- music begins

- don't have compiler to consume tokens yet
- temp code for chapter so can see scanner working

^code dump-tokens (1 before, 1 after)

- repeatedly scans tokens until reaching special eof token
- prints each token to stdout

**todo: show example output**

- goal for rest of chapter is make that code work
- key fn

^code scan-token-h (1 before, 2 after)

- tells scanner to scan and return next token
- main difference from previous scanner
- in jlox, scanner eagerly scanned all input and returned list of tokens
- in clox, have to worry about memory for tokens
- simplest solution is to not create token until compiler requests one
- where old pipeline pushes data from front to back, new one pulls
- scanner doesn't scan until compiler asks it to

- returns token by value
- token lives directly on c stack, no dynamic alloc needed
- struct is

^code token-struct (1 before, 2 after)

- similar to jlox
- enum for what kind of token it is
- line number

- difference is lexeme
- in jlox, stored as normal java string
- would require us to manage memory for string in clox
- instead, point back into source code string
- start is first char in source where token begins
- length is num chars

- means no mem management for token
- can copy around freely
- as long as memory for source code outlives all tokens, fine
- [do have to ensure that policy happens! would be bad if, say, dealloc source
  string before running code but runtime had token]

- token type enum virtually idential to clox
- blast all out at once

^code token-type (2 before, 2 after)

- aside from "TOKEN_" prefix since c doesn't scope enum names, only difference
  is error type
- in clox, scanner not directly report lexical error
- pass to compiler as error token
- let's compiler handle, helpful for panic mode

- that's token data
- now to produce
- start simple then refine

^code scan-token

- each call scans and returns next token in source
- since call beginning new token, first set scanner.start to current char
- new token starts where last token ended

- then, if call after reach end of source, return eof token
- [can call repeatedly get infinite series of eof]

- otherwise, if reach end of this fn return error token
- soon, before last line, will add code for chars that *are* valid
- need few helpers

^code is-at-end

- assume source string is good nul-term c string
- need to be careful not to advance current *past* end of string

- helper to create token of given type

^code make-token

- uses start and current chars in scanner to determine range for lexeme
- likewise current line
- returns token by value, so gets copied

- also have helper for creating error token

^code error-token

- similar to maketoken
- "lexeme" now point to given error message string
- must ensure all calls to this pass string should lifetime is long enough for
  compiler to access string
- in practice, always call with string literal, which are constant and eternal
- [need to be careful. c lang doesn't help.]


- basically have functional scanner now for language whose lexical grammar is
  empty
- reports errors on all chars
- start filling in grammar

### simple tokens

- as before start with tokens that are single char

^code scan-char (1 before, 2 after)

- read next char from source
- switch on it
- if any of single char punctuators, immediately make token
- uses new fn

^code advance

- advances pointer to current char and returns previous just consumed one
- [would be smart to assert current char is not '\0' to ensure can't advance
  past end of input]

- next-simplest are tokens that can be one or two chars
- in all cases, token can be single char or is valid two char token with `=`
  as second
- like `!` or `=`, `=` or `==`

^code two-char (1 before, 2 after)

- for these, after consume first char, look at second
- conditionally consume second if `=`
- otherwise, just consume first
- that logic in

^code match

- if current char is given one, consumes it and returns return
- else false
- now can handle all of various punctuation tokens

- before more interesting, side trip to handle chars that aren't tokens at
  all

### whitespace

- scanner needs to consume whitespace, but not emit token
- [whitespace is meaningful, though, separates other tokens]

- don't want to handle whitespace in main switch because need to ensure that
  call to scanToken() does ignore whitespace and get to actual token it can
  return
- would have to call itself or loop or something if waited until main switch
  to look for whitespace chars
- instead, before starting next token, first try to just skip over whitespace

^code call-skip-whitespace (1 before, 2 after)

- advances scanner past any whitespace chars
- if there are none, does nothing
- after calling this, know next char is meaningful

^code skip-whitespace

- loops
- as long as next char is whitespace, advances past it and keeps looping
- once hits non-whitespace, exits
- unlike previous switch, use peek(), not advance()

^code peek

- if next char is not whitespace, need to *not* consume it so that scanToken()
  can
- want to just *look* at it
- peek does that

- above handles most whitespace chars
- one char left, newline

^code newline (1 before, 2 after)

- treat a little specially because need to increment current line too

- other kind of ignored text is comments
- handle here too

^code comment (1 before, 2 after)

- if current char is `/` and next char is `/` too, then found line comment
- enter little inner loop that consumes anything until hit end of line
- have to check for eof too -- can have comment on last line
- again use peek here
- don't want to consume '\n' in this loop because want to handle that in outer
  loop so increment current line after line comment too

- also handle `/` not followed by `/`
- in that case, not comment, meaningful char, so exit entirely
- need to ensure don't even consume first `/` in this case, so have to look
  two chars ahead before consume either
- define

^code peek-next

- could make peek take param, but explicit fn makes it clear scanner only uses
  two chars of lookahead
- [also avoids having to pass 0 to most calls to peek since c no overload]

- easy tokens and whitespace
- get more interesting

### literal tokens

- number and string tokens
- literal values
- strings always start with quote

^code scan-string (1 before, 2 after)

- calls

^code string

- pretty similar to jlox
- keep reading chars until find closing quote
- also track newlines inside string
- [lox allows newlines in string]
- and handle unterm string error

- main difference from jlox something not there
- again about mem mgmt
- in jlox, token had value field to store converted literal value -- string,
  double
- field's type was object
- lot of baggage -- runtime type id, gc

- no easy way to store string or number in token in c
- instead, defer creating runtime rep until later
- token just has raw lexeme
- then in compiler, will produce value when can immediately store in constant
  table

- [does mean little redundant work
  code to recognize lexeme similar to code to produce real value from it
  would be more if had string escapes and stuff]

- number can start with any digit
- instead of lot of cases in switch, handle separately

^code scan-number (1 before, 2 after)

- utility

^code is-digit

- to finish scanning number

^code number

- virtually identical to jlox
- except don't parse lexeme to double value yet

### identifiers and keywords

- last set of tokens are identifiers
- including special reserved identifiers -- keywords
- idents start with letter

^code scan-identifier (1 before, 2 after)

- which is any of

^code is-alpha

- once found, scan rest of ident using

^code identifier

- keep reading allowed chars -- letter or digit -- until hit other
- or reach end

- then make token with proper type
- type calculated here

^code identifier-type

- first pass, no reserved words at all
- everything is user-defined ident

- how recognize keywords?
- on jlox, looked up lexeme string in map of keyword name to token type
- no maps in c

- could write our own map
- will need it eventually
- think little more fundamentally
- let's say scanned identifier lexeme is "gorgonzola"
- how much work need to do to tell if that's a reserved word?
- well, no lox keyword starts with "g", so looking at first char enough to
  rule it out
- much faster than hashing string, doing map lookup, etc.

- what about "cardigan"?
- do have one keyword that starts with c
- but second char rules out "class"
- so sometimes need to walk farther into lexeme

- what about "foreigner"?
- "for" is keyword, but remaining chars rule out

- imagine tree
- root has child for letter that starts keyword
- each of those have children for each letter that is second letter of keyword starting with that letter
- and so on
- if char node is last letter in keyword, mark it with token type

- to recognize, walk tree
- at each node, take branch that is next char in lexeme
- if reach token type node at last char of lexeme, matched full keyword
- if didn't have node for lexeme char, or still had chars left (as in foreigner)
  when reach tokentype node, it's an identifier

**todo: tree**

**todo: illustrate example traversal**

- may sound familiar
- very similar to two classic data structures
- first is trie

https://en.wikipedia.org/wiki/Trie

- [bad name]
- tree-like way to store set of strings
- instead of storing each string as separate object, split in to characters
- organize into a tree where path along branches reconstructs original string
- with special marker on nodes where string ends
- to tell if string is in set, walk tree just like did above

- trie is special case of dfa

https://en.wikipedia.org/wiki/Deterministic_finite_automaton

- finite automata other name for state machine
- [gpp link]
- state machine has set of "states" -- node in graph
- has set of allowed transitions -- labeled edges
- can go from one state to another if transition exists and "event" occurs that
  matches edge label

- can think of like old text adventures where each state is room, edges are doors, and events are navigation commands "go n"

**todo: illustrate**

- [metaphor from steve klabnik]

- dfa can be used to recognize strings
- states represent how much of string has been consumed
- each edge consumes specific char
- can transition from one state to next by consuming char
- in general dfa, can have cycles and reuse states
- in simple keyword recognizer, no loops, so tree

- could in fact use single giant dfa for entire scanner
- what scanner generators like lex and ragel do
- most regex engines generate dfa under hood

- all we need is tiny tree one for keywords
- how to map tree walk to code?

- idiot simple thing is to use switch for each level of branching
- start with first char, and easy keywords

^code keywords (1 before, 1 after)

- these are keywords where only single keyword starts with given char
- if we see "a" as first letter could only possibly be "and"
- might not be though, so still need to check rest of letters too

^code check-keyword

- need to test two things
- lexeme must be exactly as long as keyword -- not too long or short
- remaining chars much match exactly
- if so, it's keyword
- otherwise, regular ident

- couple of tricky cases
- if first letter is "f", could be "false", "for", or "fun"
- have to check second letter to see which it could be
- next level of branching in tree
- also need to make sure there is second letter

^code keyword-f (1 before, 1 after)

- "t" is similar -- could be "this" or "true"

^code keyword-t (1 before, 1 after)

- that's it
- couple of nested switches very very efficiently recognizes keywords
- [link to v8 code]

https://github.com/v8/v8/blob/master/src/parsing/scanner.cc#L1653

<div class="challenges">

## Challenges

- many newer langs support string interpolation (define). how would you
  write scanner to handle that? what tokens would emit for "Hi, ${person.name}!",
  assuming ${...} was interpolation syntax

- how do java scanners handle ">>" in generics?

- how would handle contextual keywords?

</div>
