^title Types of Values
^part A Bytecode Virtual Machine

- simpler than previous chapters
- palette cleanser
- one main concept -- tagged unions

- lox dynamically typed
- same var can hold bool, num, string, etc. at different points in time
- right now, values can only be numbers
- ["unityped" like forth, bcpl]

- introduce two new types, bool and nil
  - these two easy because fixed size, immutable
  - other types like strings and obj later

## tagged unions

- build value rep out of raw bits
- c gives nothing
  - at runtime, just undifferentiated array of bytes
- up to us to decide how many bits to use and what they mean

- two problems to solve rep value
  - how does value carry type?
    - need to be able to tell at runtime if value is bool, number, etc.
    - c doesn't, up to us
  - how do we store value itself?
    - different numbers
    - true versus false
    - hard because different sizes
      - need 64 bits for double
        - [sort, see opt chapter]
      - one bit for bool, true/false
      - no extra bits for null
    - if have an array of values of mixed type, how big each array element?
- meta problem is doing so efficiently

- soln is straightforward
- to store type, each value have enum identifying type

^code value-type (2 before, 2 after)

- then for some types, have additional type for value
- could do struct with fields to store bits for each type

**todo: illustrate struct fields**

- but value has only one type, so other fields would be unused
- union
- like struct, but fields share same memory

**todo: illustrate union fields and showing largest dictates size**

- size is size of largest field
- since fields reuse same bits, have to be careful know what doing
- if store one field then access another, misinterpret bits
- [can be useful to do so on purpose]
- value is struct wraps two

^code value (2 before, 2 after)

- field for type enum, called "tag"
- field for data union
- call "as" because accessing field sort of like converting to primitive type

**todo: illustrate**

- "tagged union"
- on 64-bit, value is 16 bytes
  - 8 for largest union field, double
  - 8 for type tag
  - only have a few different types, so really wasteful
  - [could shrink enum but doubles need to be aligned to 8-byte boundary
    so in things like array of value, would pad anyway]
  - do better later
  - but good enough for now
  - small enough can pass around on stack by copying value
  - safe to do because values immutable

## converting in and out of values

- that's it for value rep, but rest of interp assumes value *is* double
- now value can represent double, but not same as
- call all broken now, need to go through and fix

- since can't directly cast c double to value, need way to produce or wrap
  raw c data into value
- couple of macros

^code value-macros (1 before, 2 after)

- take c value of appropriate type and produce Value
- set type field and store data field
- use c99 struct init
- [in c89 no way to create struct in expr, have to write fn that returns struct]

- before interp c code can do anything useful with value, need to get raw
  value back out
- more macros

^code as-macros (1 before, 2 after)

- macros directly access union field
- if call AS_NUMBER() on value was created from, say, BOOL_VAL(), get weird
  bits back
- not safe

- need to check that value has type first
- core of dynamic typing
- [in static lang, know what type is at compile time so can directly access
  bits as type. type system is math proof that doing so is safe]
- couple more macros

^code is-macros (1 before, 2 after)

- these sets of macros let us safely move between lox dynamic world and c
  static world

## dynamically-typed numbers

- have value rep, have tools, fix existing lox to use new value rep
- kind of grind, just blowing through code

- start at beginning
- first values created as constants in compiler

^code const-number-val (1 before, 1 after)

- after converting lexeme to c double, convert to value

- over in runtime
- print statement uses this fn to output value to stdout

^code print-value (1 before, 1 after)

- delete old code assumed value was double
- now have few different types to handle
- cases for each type
- inside each case, unwrap now that we know type

- next simplest operation is unary negate
- pops number off stack, negates, pushes result

- now have to worry about value type
- what if user does

```lox
print -false; // Uh...
```

- have entered realm of runtime errors
- before performing an op that requires certain type, have to check that
  value has that type
- [major source of perf diff between dynamic and static lang]
- for negate:

^code op-negate (1 before, 1 after)

- check to see if value on top of stack is num
- if not, report runtime error and bail
- [lox primitive error handling, no exceptions, etc.]

- only after validate number can we unwrap value, negate, rewrap and push

- to check value, use

^code peek

- returns value on stack but doesn't pop
- [why not just pop? could here. in later chapters will be important to leave
  stuff as long as possible so that if gc happens in middle of op, value
  doesn't accidentally get collected]
- arg is how far down the stack
- zero is top, 1 is one down from stack, etc.

- to report runtime error

^code runtime-error

- used variadic fns often in c, like printf
- if never defined one before, this what it looks like
- va_list stuff is weird little api for accessing variadic args
- won't go into detail here
- just used to forward runtimeError() var args to vfprint(), which is
  version of printf() that takes explicit va_list for args

- later will want to generated formatted runtime error messages with parameters
- lets us do that

- after show hopefully helpful error message, tell user where in program error
  occurred
- similar to reporting compile error
- compiler uses line info in token
- passes that line info in when generating chunk
- now read that back out of chunk
- get line at current bytecode instruction offset
- should be line user's prog that bytecode was generated from
- [better would be to show full stack trace, but don't have fns yet, so don't
  even have stack]

- need to include va_list stuff

^code include-stdarg (1 after)

### binary arithmetic operators

- that's unary, binary little more of a production
- have four binary ops that different only in underlying c op: +, -, *, /
- to reduce copy/paste, defined macro to reuse code for all four
- really useful now because replace that macro to do runtime checking and
  covers all four:

^code binary-op (2 before, 2 after)

- similar to unary
- first check that operands are both numbers
- runtime error if not
- if ok, pop, unwrap, perform op, and push
- to wrap value back, use `valueType` param passed to macro
- let's use macro for binary operators where result is any type
- for arith, result is num, so pass in `NUMBER_VAL`:

^code op-arithmetic (1 before, 1 after)

- for comparison, like >, takes num but returns bool, so will use `BOOL_VAL`

## true, false and nil

- existing code all works now
- clox is basically less efficient unityped lang
- can internally rep other types, but no way to produce
- now flesh out by adding bool and nil literals, and operators on them

- start with literals
- simple, so do all at once
- with numbers, many different values, so number literals go into constant
  table and have instr to load them
- could do same thing for bool and nil
- just store, say `true` in const table and load from there
- reuse existing `OP_CONST`
- but given only three magic vals, overkill to have two-byte instr and const
  table entry
- [many bytecode instr sets have dedicated op codes from small number values]
- instead, define ops for true, false, nil that push respective values directly

^code literal-ops (1 before, 1 after)

- scanner already handles as keywords, so to parse,
  slot parser fn into pratt parser table at appropriate token

- three token types, use same fn for all three

^code table-false (1 before, 1 after)

...

^code table-true (1 before, 1 after)

...

^code table-nil (1 before, 1 after)

- when encounter `false`, `nil` or `true` in prefix position, all call

^code parse-literal

- keyword token already consumed, so only need to output instr
- look at token type to see which literal
- [could have defined separate parser fns for each too]
- front end can now compile `true`, `false`, and `nil`, in user prog
- now vm need to interp

^code interpret-literals (5 before, 1 after)

- self-explanatory
- also want to keep dis complete

^code disassemble-literals (2 before, 1 after)

- have literals for new types now, but can't do anything with them
- nil won't be very useful yet, but can at least start adding boolean ops

### logical not and falsiness

- simplest logical op is unary not, `!`

```lox
print !true; // "false"
```

- add new instr for operation

^code not-op (1 before, 1 after)

- to parse, can use existing unary parser fn wrote for `-`

^code table-not (1 before, 1 after)

- that fn already has switch on oper token type to decide what op to write
- switch only had one case before, now add another

^code compile-not (1 before, 4 after)

- what does it do?

^code op-not (1 before, 1 after)

- like other unary, pops operand, performs op, pushes result
- as usual, have to worry about different types
- can do:

```lox
print !nil;
```

- rule for how different types when used in Boolean context called "falsiness"
- implement in fn

^code is-falsey

- back when first defined lox, somewhat arbitrarily said `nil` and `false`
  falsey, everything else true

- new op, so new dis

^code disassemble-not (2 before, 1 after)

### comparison

- have enough in place to do comparison operators too
- equality, inequality, and < >
- even though latter take numbers, couldn't do in last chapter because result
  isn't number
- now can
- [still can't do logic `||` and `&&`. need control flow for that. later]

- six operators, here's new ops

^code comparison-ops (1 before, 1 after)

- only three?
- what about `!=`? `<=`? `>=`?
- could create ops for each
- honestly, would be faster, so *should* do this in real interp
- book about teaching
- want to get you thinking that bytecode doesn't have to closely follow
  source
- `a != b` is same as `!(a == b)`, so can implement `!=` using `OP_EQUAL`
  followed by `OP_NOT`
- same for `a <= b` -> `!(a > b)`
- `a >= b` -> `!(a < b)`

- all six operators use existing binary parser fn

^code table-equal (1 before, 1 after)

- inside binary, switch to output instrs gets new cases

^code table-comparisons (1 before, 1 after)

- for `==`, `<`, and `>` output single op
- for others, do pair of instrs
- first do opposite op, then `OP_NOT`
- latter pops result of former, negates, pushes result

- six operators for the price of three instrs
- so in vm, only need to implement three

^code comparison-operators (1 before, 1 after)

- equality is most general

^code interpret-equal (1 before, 1 after)

- any two value of any type can be compared for equality
- equality logic defined in separate fn
- fn result always c bool, so can safely wrap in value and push result
- fn related to values, so defined over in value module
- declare in header

^code values-equal-h (2 before, 1 after)

- then implement

^code values-equal

- if values have different types, definitely not equal
- [no weird implicit conversions like js or php. no `0` is not same as `"0"`]
- otherwise, unwrap pair of values and compare directly
- [why not just memcmp entire value struct? contains padding and unused bits
  when union uses smaller case like bool or nil. c does not guarantee what's in
  there. could have, say, two `nil` values with different unused bits in there
  would end up unequal.]
- have to be safe

- this fn will grow more cases as add other kinds of values -- strings, fns, objs, etc.

- comparison ops easier

^code interpret-comparison (1 before, 1 after)

- already extended macro to handle binary ops that return other type
- can reuse here with correct c operator and `BOOL_VAL` to indicate result
  value type is bool

- last is dis

^code disassemble-comparison (2 before, 1 after)

- numeric calculator growing to general expression evaluator
- can do

```lox
!(5 - 4 > 3 * 2 == !nil)
```

- one more built in value type missing: strings
- lot more complex because variable sized
- get own chapter, next

## design note

**todo: move to previous chapter?**

- commodity parsing

- maybe unpopular opinion
- won't water down but ok if you disagree

- people, especially academ got really into parsing and parser theory
- as branch of math produces interesting results
- fp folks got really into parser combinators
- compiler folks got really into parser generators
  - (given any problem, compiler hacker will always solve it by creating a
    new compiler, even when problem is "how do i create compiler?")

- don't want to denigrate that work
- all really interesting
- touches on other fun areas of math and cs
- really important that have proofs of memory and time bounds of typical parsing
  techniques
- want guarantee that front end won't go exponential

- parsing does matter

- but if goal is just to implement lang, don't need to be into it
- pick technique you like and be done with it
- recursive descent, pratt, combinator, antlr
- whatever you like, pick one and be done with it
- will be commodity part of impl, not strategic advantage
- [unless doing something interesting like user-extensible syntax]

- lisp shows can have successful that barely even has parser and forces user to
  write ast

- instead of spending four hours researching some crazy memoizing GLR algorithm or something, spend that tweaking the error syntax messages in your parser to be more actionable
- users will thank you
- honestly, > half of time hacking on parser should be error messages and error
  recovery
- that's where real value prop is
