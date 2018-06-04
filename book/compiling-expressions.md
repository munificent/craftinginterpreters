^title Compiling Expressions
^part A Bytecode Virtual Machine

> There are no happy endings.
>
> Endings are the saddest part,
>
> So just give me a happy middle
>
> And a very happy start.
> <cite>Shel Silverstein</cite>

**todo: more illustrations**

**todo: quote**

- chapter exciting three reasons
- first, last section in execution pipeline of vm
- once in place, can pipe source code all the way through to run
- two, get to write actual compiler
- parses syntax and outputs low level binary bytecode
- real lang hackers
- three, get to teach one of favorite techniques
- vaughan pratt's top down op prec parser
- [aside on pratt]

- before get to fun stuff, prelim
- first replace temp code for driving scanner with something closer to final

^code interpret-chunk (1 before, 1 after)

- create new empty chunk
- pass to compiler to generate bytecode from source
- if got compile error, discard chunk and bail

- otherwise, set up vm like did with hand-rolled chunk
- run it

- compile sig changed

^code compile-h (2 before, 2 after)

- now passing in chunk to compile
- return true if compiled without error
- over in impl

^code compile-signature (1 before, 1 after)

- tear out temp code to drive scanner
- instead call advance to scan first token
- then expression to parse single expr
- eventually replace with whole program
- should be done with input after that

^code compile-chunk (1 before, 1 after)

- rest of chapter making this fn work, especially expression()
- take long time to get there
- work way in from both ends

- compiler has two jobs
- parse source syntax
- compile -- produce bytecode
- many langs split into two passes
- [often many more than two]
- parser parses syntax and produces ast
- second pass walks ast and generates code
- ["code gen"]
- in clox, merge into one
- while parsing, immediately output bytecode with no intermediate rep
- "single-pass compiler"
- classic way to do it
- doesn't work great with all langs
- hard to optimize
- very simple and fast (to compile, not run)

- so compiler module has functionality familiar from jlox for parsing
- consuming and matching tokens, etc.
- and functionality for emitting bytecode
- walk through those first

- then do actual lox grammar that calls those
- little more bottom-up than like, but means when get to tricky op prec parser,
  have as much done as possible

## parsing tokens

- first, front half
- very similar to jlox
- primary fn is advance

^code advance

- scans next token and stores
- before doing so, copies old current token into previous
- useful to extract data from token after consuming it

- loop is because need to catch and report error tokens
- caller to advance wants next real token
- so this reports and skips over error tokens

- current and previous tokens are stored in parser struct

^code parser (1 before, 2 after)

- like in other modules, have single module var for struct so don't have to
  pass around

### error handling

- if scanner emitted error token, need to report it

^code error-at-current

- this fn reports given error using current token for location
- uses more primitive

^code error-at

- sometimes want to report error at previous token pos, so takes token as
  param

- first print error location
- try to show lexeme if possible
- then error message

- finally, set flag
- tracks if any compile error occurred
- lives in parser

^code had-error-field (1 before, 1 after)

- that way compile() returns false if compile error occurred

^code return-had-error (1 before, 1 after)

- one other useful flag
- want to avoid cascaded errors
- after one compile error, compiler may be confused about where it is in program
- lead to slew of errors
- not useful to report all

- used "panic mode" in jlox
- there, escaped to sync point by throwing exception to unwind out of parse
  methods
- no exceptions in c
- [could do setjmp, but risky. easy to leak mem or forget to clear state]

- instead, do simple trick
- add flag for when in panic mode

^code panic-mode-field (1 before, 1 after)

- set flag when error occurs

^code set-panic-mode (1 before, 1 after)

- after that, keep compiling as normal, but simply don't report any other
  errors
- will discard resulting bytecode without running anyway, so ok to keep going
- later when get to statements, will synchronize and clear flag

- new fields need to be inited

^code init-parser-error (1 before, 1 after)

- also need include

^code compiler-include-stdlib (1 before, 2 after)

- often will report error at location of token just consumed
- helper for that too

^code error

- one last

^code consume

- like advance, but expects next token to be certain type
- if not, reports it as error
- foundation of most compile errors

- enough base functionality to start reading and using tokens

## emitting bytecode

---

- now jump to other end
- after parsing and figuring out code, produce bytecode instrs
- already passed in chunk
- some helpers for that end
- at very end, when done compiling everything

^code finish-compile (1 before, 1 after)

- calls

^code end-compiler

- right now, just compile single expression
- when run, should evaluate and return it
- remember use return instr for last part

^code emit-return

- relies on this more primitive

^code emit-byte

- writes given bytecode and says on current line
- will use throughout next chapters

- need to get chunk to write to
- use helper fn for this

^code compiling-chunk (1 before, 1 after)

- right now, chunk stored in module var
- later, when have functions, gets more complicated
- set var right before compiling

^code init-compile-chunk (2 before, 2 after)

- often need to emit couple of bytes

^code emit-bytes

- just shorthand
- enough for now

## operator precedence parsing

- left is one fn:

^code expression

- few kinds of exprs to support in this chapter
  - number literals
  - parentheses for grouping
  - unary negate
  - binary arithmetic

- usually explain top down
- harder here because code that drives hard to understand without context
- take different approach
- outside in

- core of algo is table-driven parser
- row in table for each token type
- each row has parsing data assoc with that token type

**todo: illustrate**

- if introduced one feature at a time, have to keep rewriting table to add
  columns
- instead, show table last
- first cover everything need table to do
- have handful of fns introduce but not implement until near end
- have to hold lot in your head, best i could do

### number

- imagine exprs only single token
- different token different kind expr
- logic for compiling each kind of expr
- table of fn pointers
- index is token type
- fn is function to parse expr for that token
- TOKEN_NUMBER:

^code number

- assume number token already consumed, so in previous
- convert lexeme to value (remember lexer doesn't do)
- generate code to load value from constant table
- will have other code that uses const later, so put in helper

^code emit-constant

- add value to const table
- emit bytecode to load that const

^code make-constant

- add to constant table for chunk compiling
- return index

### grouping

- table nice
- easy to add new exprs for tokens
- easy to see which tokens already in use
- alas exprs not all single token

- some exprs start with token
- prefix expr
- if parsing expr and next token is `(`, know grouping expr

- table still works
- token is token that *starts* expr
- fn can then consume more tokens

^code grouping

- assumes "(" consumed which is how got to fn
- can have any expr inside parens, so recursively call expression()
- [parser still recursive, since grammar recursive. not recursive *descent*.]
- then must end with ")"
- grouping expr itself just "grammar hack" to let have low prec expr where
  higher expected
- so doesn't emit any code on its own
- call to expression() does work

### unary

- unary negate also prefix expr
- assume parsed `-` token, then

^code unary

- start with token, then need to compile operand
- emits instructions to execute operand expr
- result of executing those instrs leaves value on stack
- right where want
- then emit instruction for unary operator itself
- that will pop value, perform op, push result

- problem here
- unary allows any expr as operand
- once add binary ops, does wrong thing

```lox
-a.b + c;
```

- here, operand to `-` is only `a.b`, not whole `a.b + c` expr
- operand is expr, but only expr above certain precedence level
- need way to parse expr but only allow exprs at given prec or higher
- in old recursive descent parser, had fn for each level
- could call directly
- fns different here
- each fn only compiles expr of exactly one type
- doesn't cascade to higher precedence too

- need different solution
- create fn that parse expression at any given precedence level or higher

^code parse-precedence

- get to body later
- define precedence numerically

^code precedence (1 before, 2 after)

- highest prec has highest enum value
- then when compiling operand, can pass in prec and say "only this or higher"

^code unary-operand (1 before, 2 after)

- since unary op pretty high prec, operand has to be call or higher

### infix ops

- last expr type is binary op
- different from others
- before, token that keys parsing logic is first
- binary different
- by the time hit operator, already compiled left operand

```lox
-1 + 3
```

- first, see `-`, ok unary negate
- compile 1 operand
- don't treat `+` as part of `-` operand since too low precedence
- unary() returns
- now what?

- need way to handle token appearing *after* expression
- call "infix"
- after parsing expression, look at next token
- look up in table
- if infix parse fn associated with it, call it
- only one infix so far:

^code binary

- when called, left operand already compiled
- works out fine
- because stack-based, result of operand sitting on stack where want
- next, compile right operand
- finally, emit instruction for op
- again, works great with stack
- two operands end up on stack, then inst for binary op pops them, does op
  pushes result

- interesting part is what precedence to use to parse rhs
- get to parserule stuff soon
- understand that that code gets precedence of current binary operator
- then parses operand at one level above that

- ensures operands always higher precedence than operator itself, which is
  what want
- [+1 is because left assoc. if right assoc would be +0. example.]
- for example, prec of "+" is PREC_TERM
- next one up is PREC_FACTOR
- is indeed right prec for operand to "+" - can be multiply, divide or anything
  higher

- this one binary() fn handles all four arithmetic ops and their different
  precedences

### parse table

- those are all fns around table driven parser
- can now define table and see how works

- review, for each token type, can have
    - pointer to fn for parsing prefix expr that starts with that token
    - pointer to fn for parsing infix expr with that token following operand
    - precedence level for infix expr for that token
- all optional, many tokens don't support
- [can't use `if` as infix op! except in python and ruby]

- table is array
- index in array is token type
- each row is struct:

^code parse-rule (1 before, 2 after)

- prefix is pointer to fn for parsing expr that starts with token
- infix is pointer to fn for parsing expr that has token after left operand
- prec is precedence of token when used in infix
- [don't need prefix prec because all prefix ops have same prec in lox.]

- parse fn just simple fn ptr typedef

^code parse-fn-type (1 before, 2 after)

- then have big static array of rules

^code rules

- will fill in table over time as add new expressions in later chapters
- [cool part of this approach is very clear which tokens available for use as
  expressions. can see nulls right there.]

- now can go back and fill in missing fns

- getrule fn used in binary() just looks up array element

^code get-rule

- need to wrap in fn to avoid cycle in decls
- binary() defined above table so table can store pointer to it
- so define some forward decls above all parse fns

^code forward-declarations (1 before, 2 after)

- that way can recursively call
- needed for subexprs

- here where gets interesting
- parse prec where real work happens
- start with prefix handling

^code precedence-body (1 before, 1 after)

- reads next token
- looks up rule for it
- if no prefix parser, then token can't begin expr
- must be a parse error because expect expression
- otherwise, let prefix parser handle
- simple

- now that have parseprec, expr is

^code expression-body (1 before, 1 after)

- just starts at lowest prec level, which subsumes all

- last part is trickiest
- most powerful code in entire chapter
- very simple, but subtle with how interacts with other fns that call back to
  parsePrec()

^code infix (1 before, 1 after)

- when parsing expr, first tokens always belong to some prefix expr
- may be deeply nested inside infix, but left to right, first is prefix
- in:

```lox
-1 + 2 * 3 == 4
```

**todo: illustrate ast**

- here, outermost expr is `==`, which don't get to until much later
- but leftmost is `-1`

- walk through how parseprec parses this
- comsume first token, `-`
- look up prefix rule
- find `unary()`
- that recursively calls `parsePrec(PREC_CALL)`
- consume next token, `1`
- look up prefix rule
- find `number()`
- compile number constant and return

- now enter infix section of `parsePrec()` for inner call
- next token is `*`, so there is infix
- but remember, parsing operand to unary, so should *not* parse `+`
- unary op should just be `1`

- in while loop, look up prec of current token `+`
- PREC_TERM is lower than PREC_CALL, so while condition fail
- don't go into loop, return
- so entire operand of `-` is just `1`

- now back in outer call to `parsePrec()`
- prec is PREC_ASSIGN
- get to while loop again
- this time, PREC_TERM is higher than PREC_ASSIGN, so can get into loop body
- consume `+`
- look up infix parse fn for it
- [know won't be null because all null infix rules have PREC_NONE which is lower than all]
- call it, binary()
- calls parsePrec() again for rhs
- passes in own prec + 1, so PREC_FACTOR

- next token is `2`, so prefix parser is number()
- enter infix loop because next token, `*` is PREC_FACTOR
- call binary() for *
- calls parsePrec() with PREC_UNARY, which is 1 + PREC_FACTOR
- next token is `3`
- prefix parse is number()
- don't get into infix loop because next is == which is lower than PREC_UNARY
- return, so rhs of * is just 3
- now back in rhs parser for `+`
- next token still `==` which also too low for `+` so return again
- rhs of `+` is `2 * 3`
- back in outermost parsePrec() call with PREC_ASSIGN
- finally low enough to get into infix loop
- again call binary()
- consume `4` as prefix expr
- next token is TOKEN_EOF with PREC_NONE, so don't get into infix loop
- so outermost expression is `==` and done

- may take while to wrap head around
- worth stepping through in debugger to see how recursive calls climb prec
  until run out and return
- love love love this fn
- remember, one `binary()` handles all arith ops, at all prec levels
- once understand this, can write parser like this whenever need
- can easily extend to more prec levels, postfix and prefix ops, even mixfix
  like ternary

## dump chunk

- one last small thing
- make our lives easier hacking on vm
- add support for dumping disassembled bytecode after running compiler
- did with hand-rolled chunk
- now add support to compiler

^code define-debug-print-code (2 before, 1 after)

- add flag so can disable it
- end users don't want bytecode spew

^code dump-chunk (1 before, 1 after)

- after compile finishes, dump bytecode
- don't do this if had compile error
- when error occurs, compiler does not promise to generate valid code after
  error
- ok, because will never be executed
- but could be confusing to look at disasm

^code include-debug (1 before, 2 after)

- compiler also need to include disasm module

- now tied knot on vm
- have full pipeline from source string to tokens to parsing and compiling to
  bytecode, to executing
- go ahead and run
- can type in arith expr and executes and prints result
- made little calculator

- not seem impressive, but strong foundation inside
- next, start fleshing out calc to turn into real lang

<div class="challenges">

## Challenges

1.

- no prefix prec because all prefix expr in lox same prec
- most other langs same
- js has low prec prefix expr, yield

    yield 1 + 2;

- yields result of 1 + 2
- operand to yield allow binary op
- when parsing yield operand, need to allow lower prec than just PREC_CALL
- could do separate parse fn with direct call to parsePrec with right prec

**todo: not sure if this q makes sense**

2.

- parse rule for `-` has both prefix and infix parse rule
- because unary negate and binary subtract
- in full lox lang, any other token types used for both?
- what about c?

3.

- can use same infix support for "mixfix" expr where have more than two operands
  separated by tokens
- classic example is "?:" in c
- add to parser

</div>
