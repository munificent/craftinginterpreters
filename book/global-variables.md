^title Global Variables
^part A Bytecode Virtual Machine

> If only there could be an invention that bottled up a memory, like scent. And
> it never faded, and it never got stale. And then, when one wanted it, the
> bottle could be uncorked, and it would be like living the moment all over
> again.
>
> <cite>Daphne du Maurier, <em>Rebecca</em></cite>

**todo: quote**

- last chapter one big cs data structure
- this not conceptual, just engineering
- start adding state
- begin with global variables
- no scope yet

- declaring var is statement, so time to add statements
- side effects imply statements
- before worry about globals, get basic statements working

## statements

- recall two levels of "prec" for statements, statement and decl
- prohibit decl stmts right inside flow control like:

```lox
if (monday) var croissant = "yes"
```

- divide statements into those that declare names and those that don't
- "statement" just ones that don't, like print statement
- "declaration" rule includes all
- top level of prog and inside block is decls

- right now "program" is single expr
- should be list of statements, do now

^code compile (1 before, 1 after)

- until run out of code, keep parsing decls
- since flow control and blocks contain statements, will be recursive, so
  forward declare

^code forward-declarations (1 before, 1 after)

- not doing actual decls yet, so first pass

^code declaration

- just calls statement (subsumes)

^code statement

- do two kinds right now, print and expr
- if token is "print" keyword, print
- uses

^code match

- had similar parsing util fns in jlox
- if current token is given, consumes and returns true
- uses

^code check

- just returns true if current token is given one
- will use more later

- if did match `print`, call

^code print-statement

- compiles rest of print statement
- print evals and prints expr, so compile expr
- terminated by semicolon
- then add new instruction to print

^code op-print (1 before, 1 after)

- interp is

^code interpret-print (1 before, 1 after)

- compile expr then emit OP_PRINT instr
- expr evaluates to value and leaves on stack
- so print inst pops that and prints it

- key difference between expr and stmt in vm
- *stack effect* -- way that instruction or chunk of code modifies stack
- for example, `OP_ADD` pops two and pushes result
- stack effect -1, stack gets shorter
- overall compiled expression always leaves one result value on stack

- statements have side effect, not result value
- compiled statement never leaves anything on stack
- important because when get to looping, if statement changed stack size, could
  overflow or underflow

- while in vm

^code op-return (1 before, 1 after)

- when vm just evals on expr, inserted temp OP_RETURN to print result
- now prog list of stmts
- OP_RETURN just exits interp loop
- [revisit again when add fns]

- as usual, new instr needs dis

^code disassemble-print (1 before, 1 after)

- that's print

```lox
print 1 + 2;
print 3 * 4;
```

- exciting
- next expr statement

^code expression-statement

- just expression at "top level" of statement, terminated by semicolon, like:

```lox
hi("friend");
```

- semantics are eval expr and discard result
- compiled code literally that
- compile expr
- then emit `OP_POP` to pop and discard value expr leaves on stack

^code pop-op (1 before, 1 after)

- won't believe impl

^code interpret-pop (1 before, 2 after)

- or dis

^code disassemble-pop (1 before, 1 after)

- statements
- way back when first started compiler, put half of support for panic mode
  error recovery in
- missing piece is sync
- want to sync to statement boundary
- have now

^code call-synchronize (1 before, 1 after)

- if error occurred while compiling statement, sync before continuing
- calls

^code synchronize

- skip tokens until either run out or reach something that looks like statement
  boundary
- if pass semicolon, next token should begin statement
- also if next token is keyword that starts statement, probably good place to
  stop

## variable declarations

- that structure in place, start doing interesting statement
- refresher
- global variables resolved dynamically "late bound" in lox
- intent is to handle mutually recursive fns
- **todo: example**
- also plays nicer with repl
- so don't require var to be declared textually before used
- just have to define it before use is executed

- three operations for variables
- declare, using `var` statement
- access using variable name as expression
- assign new value to existing var using assignment expr
- do in that order, first decls

- decl grammar production now does something

^code match-var (1 before, 2 after)

- if see `var` keyword, call

^code var-declaration

- after `var` is identifier for variable name
- then optional initializer
- if see `=`, then have initializer
- otherwise, implicitly init to `nil`
- finally end with semicolon

- two new fns here, `parseVariable()` and `defineVariable()`
- first is helper for compiling reference to identifier

^code parse-variable

- expects identifier token
- if not found, reports given compile error
- otherwise calls

^code identifier-constant

*Creates a string constant for the given identifier token. Returns the index of
the constant.*

- takes lexeme of identifier token and stores as string in chunk's constant
  table
- global variables are accessed by name at runtime
- means vm needs to be able to get to name
- constant table is how compiler hands string over to runtime
- fn returns index of string in constant table

- [doesn't deduplicate constants. string is interned, but would be good to not
  use multiple constant table slots for same identifier.]

- take that index and pass to

^code define-variable

- outputs instruction to actually create variable at runtime
- takes index of var name in const pool as instr operand

- as usual in stack-based vm, emit this instruction last
- initializer is evaluated and leaves result on stack
- this takes that value, creates and stores global

- [revisit this fn when add locals]

- new instr

^code define-global-op (1 before, 1 after)

- since already have hash table, impl not too hard

^code interpret-define-global (1 before, 2 after)

- look up variable name
- take value from top of stack and store in hash table
- then pop

- [peek then pop to make sure var still on stack while inserting in hash table.
  otherwise, gc could collect it.]

- note that don't check to see if already in table
- recall that lox allows redefining var at top level
- handy in repl

- new helper macro

^code read-string (1 before, 2 after)

- reads one-byte operand from chunk
- treats as index into const table
- reads value at that index and casts to string
- doesn't check, so only safe to use this macro for code where compile ensures
  constant has right type

- as usual, undef at end of interpret fn

^code undef-read-string (1 before, 1 after)

- need place to store globals

^code vm-globals (1 before, 1 after)

- give vm hash table for all global variables
- storing directly in vm ensures they persist across multiple repl entries
- as with string table, need to initialize empty

^code init-globals (1 before, 1 after)

and then free when done

^code free-globals (1 before, 1 after)

also have enough inst to dis

^code disassemble-define-global (1 before, 1 after)

- can define global vars
- but can't actually use them

## reading variables

- like every lang ever, access var by identifier expr
- slot into parsing table

^code table-identifier (1 before, 1 after)

- calls

^code variable-without-assign

- indirection little pointless now, but will reuse this later for handling
  `this` and `super`

^code read-named-variable

- uses same `identifierConstant()` fn from above to add identifier to constant
  pool as a string
- then emit instr to load global var
- like `OP_DEFINE_GLOBAL`, has operand for index of name in constant table

^code get-global-op (1 before, 1 after)

- impl mirrors define

^code interpret-get-global (1 before, 1 after)

- get name by looking up operand in constant table
- then look up in global var hash table
- if not found, report runtime error and abort
- otherwise push value onto stack

^code disassemble-get-global (1 before, 1 after)

- little dis

- with that, starting to feel like real language implementation
- can store intermediate computation in vars, use for later expressions
- last operation is assigning vars

## assignment

- this part surprisingly hard
- not for fundamental reason
- because of implementation choice
- compiler parses and generates bytecode in single pass with no intermediate
  ast
- assignment difficult
- in:

```lox
menu.brunch(sunday).beverage = "mimosa";
```

- don't know entire left-hand side is lvalue until reach `=`
- in jlox, parsed as regular expression to ast
- then when hit `=`, transformed left side to new ast
- can't do that

- not as dire as seems
- in ex, even though whole thing is lvalue, most evaluated as normal
- `menu.brunch(sunday)` executes like normal expr
- compiler can compile it as normal
- only part different is `.beverage`
- when see `=` after that, treat as setter, not getter
- so don't need that much lookahead to handle assignment
- should be able to fit into single pass

- question how
- need to intercept compiling expression if followed by `=`
- do so before generating code
- so leave it up to each expr that can appear on left of `=` to lookahead
  and compile appropriately

- in lox, only two

```lox
variable = "value";
```

- identifier expr on left hand side becomes assignment instead of access

```lox
object.field = "value";
```

- getter expr becomes setter
- do this in later chapter
- [if had lists, subscript operator would be third case]

- so in fn that compiles named var, look for `=`

^code named-variable (1 before, 1 after)

- if see one, then compile expr on rhs
- then emit instr to assign to var
- otherwise, compile as access as normal

- new instr similar to others
- take const index as operand

^code set-global-op (1 before, 1 after)

- interp similar to defining

^code interpret-set-global (1 before, 2 after)

- only difference is that runtime error to assign to non-existent var
- dis

^code disassemble-set-global (1 before, 1 after)

- done?
- no, made mistake, consider

```lox
a * b = c + d;
```

- walk through parsing
- first parse `a` using `variable`
- then enter infix loop of pratt parser
- hits `+` and calls `binary()`
- parses right operand
- gets to `variable` again for `b`
- that looks for trailing `=`
- sees it and parses as assignemnt
- so above compiled like:

```lox
a * (b = c + d);
```

- totally wrong
- both `=` and `+` lower precedence than `*`
- checking for `=` in `variable` sidesteps precedence handling

- when compiling expr that can precede `=`, also need to know whether or not
  nested inside larger expr
- if inside expr with higher precedence than `=`, then can't consume the
  `=`

- code that tracks precedence is in `parsePrecedence()`
- it knows prec of current fn
- to solve, pass in to parsing fns
- all they need to know is if in low enough prec to allow assignment, so pass
  bool
- pass to prefix parse fn (like `variable()`)`

^code prefix-rule (4 before, 2 after)

- also infix

^code infix-rule (1 before, 1 after)

- setters are infix fns starting with `.`
- means parse fn type is different

^code parse-fn-type (2 before, 2 after)

- takes bool arg
- gets passed to `variable`

^code variable

- passes through

^code named-variable-signature (1 after)

- now can use that to tell if allowed to parse `=`

^code named-variable-can-assign (2 before, 1 after)

- `expressionStatement()` calls `expression()`
- that calls `parsePrecedence(PREC_ASSIGNMENT)`
- so allows low enough prec to handle `=` in cases like:

```lox
variable = "value";
```

or even chained

```lox
variable = another = "value";
```

- but checking `canAssign` correctly prohibits parsing `=` in

```lox
a * b = c + d;
```

- what should happen?
- code is erroneous
- need to report error
- do here

^code invalid-assign (5 before, 1 after)

- infix loop will hit `=`
- no parse reg for that so exits loop
- then hit this code which sees `=` and reports as error
- after reporting error, parses expression
- user likely has expr after `=`, so consumes that to minimize cascaded errors

- changed parse fn type to take param
- means all parse fns need to accept arg even though others don't handle `=`

^code binary (1 after)

...

^code parse-literal (1 after)

...

^code grouping (1 after)

...

^code number (1 after)

...

^code string (1 after)

...

^code unary (1 after)

- tedious
- as with jlox, feels like big milestone
- no longer simple expression calculator
- vm has brain
- can remember and update facts

<div class="challenges">

## Challenges

1.  compiler adds var name to constant pool every time encountered
    optimize by reusing name if already in there
    what is perf?

2.  resolving global dynamically by name important for some use cases
    in common case, know global is defined
    slow
    how would optimize while still having same behavior?

    [todo: answer. globals hash table maps names to indexes. globals stored in
    separate array. at compile time, add/look up name in array. instr operand
    is array index. have special "undefined" value to tell if not defined yet.

3.  when running from repl, may refer to global not defined yet and need to
    handle gracefully
    but when running script, after compiling entire script, can tell if there
    are any globals that are used but never defined
    still need to handle case where definition isn't executed before use, but
    if literally is not definition, statically know use will never succeed

    should that be a static error?

</div>
