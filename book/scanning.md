^title Scanning
^part A Tree-Walk Interpreter in Java


**TODO: context lines aren't correct. showing content from later or earlier snippets.**

---

- first step of lang is scanning
- also great first chapter because pretty easy
- by end of chapter, be able to take any string of lox code and chunk into
  tokens to later feed into parser

## The Interpreter Shell

- since just started, before we get to scanner, need to sketch out app skeleton
- very first bit of jlox

^code lox-class

- doesn't do much
- still, makes you get your project set up and figure out ide and stuff
- also explains notation here for code snippets
- note file name this should go in

- lox is scripting lang
- two ways to run code
- if give jlox path to file
- loads and runs it

^code run-file

- other way is "interactive prompt"
- lets user incrementally build up program one line at time
- if run jlox with no arg, enters this mode

^code repl

- reads line of input, executes it and loops
- (ctrl-c exits)
- called "repl" -- lisp history
- both use:

^code run

- eventually, this will plumb through parser and interpreter
- for now, just prints tokens so we can see what scanner produces

### error handling

- another key part of interpreter is how it manages errors
- often left out of textbooks, but vital when comes to real impl
- users need help them most when program isn't doing what they want
- error handling pervasive part of job
- sooner start, the better
- confess: pretty simple here, though

- put in framework now and use it later

^code lox-error

- tells user error occurred on given line
- telling user that error occurred not very useful
- need to tell them where
- if get error trying to add num to bool, not helpful to say "some + somewhere
  in prog is bad, good luck finding it!"
- better would be line, column and length
- best would allow multiple source locations since some errors involved multiple
  points in code

- just line to keep book simpler

- reason defining in lox class is because of hadError

^code had-error (1 before)

- when error occurs while loading script, want to set exit code
- nice to be good command line citizen
- [yes static is hacky]

^code exit-code (1 before, 1 after)

- also generally code to separate error *reporting* code from error *generating*
  code
- scanner detects error, but not really its job to know how best to present it
  to user
- in prod language, should pass in some kind of ErrorReporter interface to
  scanner
- abstract how error displayed
- can print to stderr, show popup on screen, add errors to ide's error log, etc.
- to keep simple, don't have actual abstraction here, but do at least split it
  out some

- shell is in place
- once have scanner class with scanTokens() working, can start using
- before get to that, talk about tokens

## Tokens and Lexemes

- what is token?
- smallest sequence of chars that is meaningful
- in `name = "lox";` "name", "=", `"lox"` and `;` all meaningful
- `na` is not, neither is `ox"`.
- scanner's just is to go through string of chars, find meaningful units
- each is called lexeme
- lexeme just raw sequence of chars

- in process of recognizing lexemes, also figure out other useful stuff

### token type

- if lexeme is a word, like `while` can also recognize that it's keyword `while`
- since keywords affect grammar of language, parser will often need logic like,
  "if next token is `while` then ..."
- technically, is redundant with lexeme
- could compare strings
- but very slow and kind of ugly
- so at point when we recognize lexeme, which also store which *type* of token
  it represents
- which keyword, punctuation, operator, or literal
- simple enum

^code token-type

### literal

- some lexemes for literals
- at point that scanner detects literal, can also produce runtime value
- if lexeme is number `123` can convert to actual number value 123

### line info

- as we saw, error reporter needs to know where error occurred
- have to keep track of that through all phases of interpreter
- can't drop on floor if error may occur later
- start in scanner
- each token tracks which line appeared on

- bundle all of that together into token class

^code token-class

- java really verbose for dumb data object, but that's it

## Regular Languages and Expressions

- now know what we need to produce, let's produce it
- core of scanner is loop
- starting at beginning of source, figure out what lexeme first char is part of
- consume as many chars as belong to that lexeme
- produce token
- repeat with rest of string
- when whole string is done, done scanning

- process of matching chars might seem familiar
- if ever used regular expression, might consider using regex to do it
- ex: if source is `breakfast = "croissant";`, first lexeme is `breakfast`
  identifier
- could use regex like `[a-zA-Z_][a-zA-Z_0-9]*` to match it
- captures underlying rule that identifier start with letter or underscore
  followed by zero or more letters, underscores or digits
- you have deep insight here

- rules that determine what chars are allowed in each kind of lexeme are called
  "lexical grammar"
- in lox, as in most languages, rules are simple enough to fit within
  restriction called "regular language"
- lot of interesting theory here about what makes language regular, how it
  ties to fsms
- most other pl books cover well, not getting into here
- same "regular" in "regular" expression
- you *can* make a scanner that uses regexs to match lexemes
- could use java's regex lib
- also tools like lex/flex that will take whole file of regex rules and
  generate scanner

- want to understand how they work
- hand-build scanner for our language's rules
- basic scan loop

## The Scanner

- let's sketch out class

^code scanner-class

- (seems like creating awful lot of files. early chapters do lot of framework
  gets better later)

- like said, scanner walks string
- finds range of chars that map to lexeme
- when hits end, creates token
- loop until reach end
- then done
- core looks like:

^code scan-tokens

- eat way through string emitting tokens as we go
- when done, add special eof token to end of list
- not strictly needed, but makes parser little cleaner
- few fields to keep track of where we are

^code scan-state (1 before, 2 after)

- current index of next char to consume in string
- tokenStart beginning of next token
- since later tokens will be more than one char long, need to remember beginning
- when produce token, will be substring from start to current
- line is current line number

- little helper fn

^code is-at-end

## Recognizing Lexemes

- finally get to real heart of scanner
- start simple
- imagine all tokens only single character
- how implement?
- easy, just consume next char
- based on what it is, produce token of right type
- lox does have few single-char tokens, so let's do those

^code scan-token

- uses couple of helper fns

^code advance-and-add-token

- advance adds next char to current lexeme and returns it
- can call even when don't know what lexeme is yet, do know it's going to be
  some lexeme

- addtoken grabs current lexeme and line info and adds new token to end of list
- also have one for literal we'll use later

### invalid tokens

- before fill in rest of language, what happens if next character doesn't
  match any lexeme?
- lox doesn't use `@`
- what if user types that in
- add little error handling

^code char-error (1 before, 1 after)

- note still called advance(), so still consume char and move on
- important so don't get stuck in infinite loop
- note also don't stop scanning after this
- even though error occurred, want to keep going
- source file may have more than one error
- if possible, should find them all in one go
- no fun if user gets one error, fixes, then another appears and so on
- so strategy is to report error and try to keep truckin'
- won't actually *execute* code though
- if any compile error occurs, no running
- just want to find as many compile errors as we can
- would be better here to consume multiple unrecognized chars in one go so
  don't shotgun errors if user has pile of bad chars

### operators

- ok, get back to filling in grammar
- have punctuation and single-char operators

- not all operators
- what about `!`?
- it's a single char token too, right?
- not if followed by `=`
- if `!` char appears by itself, should be `!` token
- but if very next char is `!=`, then should be `!=` token
- likewise `<`, `>`, and `=`
- all can be followed by `=`

- for those, need to check next char too

^code two-char-tokens (1 before, 2 after)

- uses helper fn

^code match

- like advance(), but conditional
- only advances if current char is what looking for
- returns whether or not it matched
- lot like `?` in regex, right?
- could actually use this at top level of scan loop instead of switch
- long chain of matches
- but would look at first character multiple times, first for `!=`, then for
  `!` if second char wasn't `=`
- switch actually much faster
- surprisingly, scanning often important bottleneck in compiler
- only part of compiler that looks at each char of source
- everything later at coarser granularity, even though some algs have worse
  complexity

- now have scanner that can handle `(+-!!==)` stuff like that

### maximal munch

- our language doesn't have '--', but if it did, how would handle:

    a---b

- could be valid, if scanner broke it up like:

    a - --b

- or even

    a - - - b

- but means when scanner is looking at first two `--`, would have to know what
  grammar context it is in to know where to split up
- do scanners do that?
- no: adds way too much entanglement
- basically destroys separation between scanner and parser
- instead, simple rule, called "maximal munch"
- scanner always eats as many characters as it can when forming current token
- so above is scanned as `a -- - b` even though that causes later parse error
- simple is better

### comments and whitespace

- missing one operator, `/`
- little trickier because of `//`

^code slash

- same general idea, after match one `/` if next char is `/`, need to handle
  it differently
- comment consumes any char to end of line

^code comment

- another helper

^code peek

- sort of like advance, but doesn't consume
- **lookahead**
- only looks at current unconsumed char, so *1* char lookahead
- important to keep this number small, affects perf of scanner
- grammar of language defines how small it can be
- if not *constant* adds lot of complexity to grammar

- aside about lookahead and complexity?

- don't want to use match() because want to handle newline later to keep track
  of line
- note, don't emit token
- comments consumed, but not turned into token
- just discard
- no addToken() call
- that way rest of pipeline doesn't have to worry about them
- brings to other thing can discard, whitespace:

^code whitespace (1 before, 3 after)

- just line comments, spaces and other whitespace are consumed
- (remember, already advanced c)
- but emit no tokens
- newline char little special
- also discarded, but increment line
- that's all that's needed to keep track of what line we're on

- code more free-form now
- can correctly scan

```lox
// this is a comment
(()){} // grouping stuff
!*+-/=<> // operators
```

- making progress!

### String literals

- if can handle tokens like `!=` that are two chars, ready to tackle longer
  ones like number and string literals
- start with strings
- string token always starts with `"`, so begin there

^code string-start (1 before, 2 after)

- calls:

^code string

- like two-char operators, consume additional characters after first
- but here, do it in a loop until we hit closing `"`
- also need to safely handle hitting the end of the source without finding the
  closing quote
- report unterminated string error

- otherwise, produce actual string literal value by stripping off quotes
- store that in token
- lox has no string escape sequences
- if did, would handle them here so literal had real chars

- note allow multiline strings
- need to update line when newline appears in string too
- could make this an error but most langs seem to end up supporting

### Number literals

- lox has one number type, but both int and floating point literals
- int is series of digits
- floating point is series of digits, followed by `.` and more digits
- unlike some other langs, don't allow leading or trailing dots
- just

```lox
1234
12.34
```

- don't want cases for every digit in switch, so stuff in default

^code digit-start (1 before, 1 after)

- little helper

- (not use `Character.isDigit()` has stuff like devangari)

^code is-digit

- like string, goes to separate fun to scan rest of number

^code number

- scan sequential digits
- when run out of digits, look for fractional part
- only allow it if there is digit after dot
- (could allow trailing dot, but little weird in language with `.` method
  syntax. don't currently allow methods on number literals like `123.abs()`, but wouldn't want to rule out.)
- need two chars of lookahead

^code peek-next

- then use Java to convert number to string
- could do this ourselves
- common interview question
- but kind of silly

- last literals are boolean and null, but handle those as keywords, which
  gets us too...

### Identifiers and keywords

- almost everything
- in beginning was word, but for us its at the end

- might think we could handle reserved words like we handle multi-char operators like `<=`
- like:

```java
case 'o':
  if (peek() == 'r') {
    addToken(OR);
  }
  break;
```

- consider if user types in 'orchid'
- [fortran aside]
- remember maximal munch!

- can't distinguish between reserved word and identifiers until know we've
  reached end
- where term comes from "reserved word" is a "word" (ident) that is reserved by
  the language
- really is an identifier, just one with special meaning
- so handle identifiers first

**TODO: make sure context lines look right here**

^code identifier-start (3 before, 3 after)

- like number, put in default so don't need cases for every char that can start identifier
- calls

^code identifier

- both of those use these obvious helpers

^code is-alpha

- to handle keywords, after finish identifier, see if lexeme matches any of
  known set of reserved words
- [if use flex to generate lexer, rolls keywords into main fsm. advanced hand-written lexers still sometimes use fsm for this since perf critical]
- each keyword has its own token type
- makes parser simpler
- so need to associate keywords with token types
- use a map

^code keyword-map

- static since immutable global property of lox language
- [may be first time ever used static block in java]

- then when scanning ident, see if keyword

^code keyword-type (2 before, 1 after)

- after scanning ident, look up lexeme in map
- if found, use that type
- otherwise, must be ident

## challenges

- challenge
  - many langs use newlines as statement separator
    - have to handle case where newline occurs in place that should not end
      statement
    - explain how js, ruby, python, go, and lua handle that
    - which do you prefer?
  - python's lexer isn't regular, why not?
  - aside from separating tokens, spaces aren't used for much. it does
    come into play in CoffeeScript, Ruby, and C preprocessor. where?
  - scanner discards whitespace and comments
    - not meaningful to lang, so don't need
    - some scanners keep them
    - why?
