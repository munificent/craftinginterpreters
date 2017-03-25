^title Evaluating Expressions
^part A Tree-Walk Interpreter

- frankenstein

**TODO: illustrate**

- lots of ways to execute code
  - once have syntax tree, may translate to other form, do opt, compile to
    native code
  - first interp take simplest path
  - execute syntax tree itself

- just have tree for expression
- execute means evaluate expression to produce value
- for each kind of expression, need chunk of code to evaluate it
  - literal, operator, etc.
- two questions: what do values look like, and where does code live?

## Representing Values

- clarify how different from literals and literal exprs
- since lang dynamically typed, need to access type of obj at runtime
- need to define truthiness, equality
- jvm tracks types, lets us do instanceof
- so don't need to wrap primitives in our own type, just box
- null for nil
- Boolean for bool
- Double for num
- String for strings
- get to functions, classes, instances later
- [easy here because jvm gives us a lot. more complex when get to clox.]

## Evaluating Expressions

- need separate code for each kind of expression
- could stuff into expr classes themselves by adding "interpret" method
- [that's interpreter pattern]
- like we discussed earlier, mixes too many things into expr
- instead, use visitor
- like astprinter but instead of producing string, produce value

- live inside new interpreter class

^code interpreter-class

- impls visitor
- return type is Object since that's base type of obj rep
- start filling in methods
- easiest is literals

^code visit-literal

- leaves of expression tree
- returns value
- can see how literal flows through three reps -- token, expr, obj
- expr and obj are different because many values don't come from literals
- in `2 + 3`, will produce obj for `5`, but number `5` appears nowhere in
  source

- keep going

^code visit-grouping

- slightly more interesting
- has single subexpression
- group expr itself doesn't do anything interesting
- value is equal to value of subexpr
- [some lang impls don't even represent grouping in syntax tree. parser
  discards and is only used to allow lower prec sub exprs. need to keep it
  around in lox when we get to assignment.]
- evaluates it

^code evaluate

- helper to bounce back to visitor

- start to see how evaluating expr usually requires evaluating subexprs
- do subexprs first, which makes expr interp a post-order traversal

^code visit-unary

- now have expr starting to do work
- unary `-` negates operand, which must be number

^code unary-bang (1 before, 1 after)

- switch is there because two unary ops
- other unary operator does logical not
- `!` is obvious if operand is bool
- lox dynamically typed, so needs to handle other types too
- "truthiness"
- defined in intro
- time to implement

^code is-true

- moving on to binary arith ops

^code visit-binary

- same deal but with two operands
- eval subexprs
- perform correct op

- one more arith op, little more complex

^code binary-plus (3 before, 1 after)

- can add two strings to concat
- [could have defined separate syntax. would make this more explicit, little faster.]

- other binary operators
- comparison

^code binary-comparison (1 before, 1 after)

- not really any different
- produce boolean as result

- [could be useful to allow comparing other types. even mixed types useful.
  python defines that so that heterogeneous lists can be sorted.]

- last couple

^code binary-equality

- unlike above, work with ops of any type, even mixed
- ok to ask if bool is equal to num
- uses

^code is-equal

- place where details of object rep matters
- where we translate lox semantics of obj equality into java sem
- fortunately, pretty similar
- have to handle null/nil specially so we don't call .equals() on null ref
- .equals() on double, string, bool same as in lox

## Runtime Errors

- speaking of lox semantics and java, what about type errors?
- in above, lots of java casts to numbers for arith ops
- what happens if op is not double?
- what *should* happen?

- time to talk about runtime errors
- spent lot of time talking about error handling
- syntax errors
- now runtime
- static error reported before any code is run
- runtime error reported before wrong code is run
- line gets a little blurry
    - consider python program that nests import inside if
    - imported module has syntax error
    - if import isn't executed, program still runs even though module is
      sort of part of program
    - think about type errors in static lang
    - most caught at compile time
    - what about cast in java?
    - some portion of type system checked at runtime

- because lox dynamic all type errors at runtime
- what do on runtime error?
- lots of options
- most important is can't keep going
- runtime error means operation can't be performed
- [if keep going, not really error]

- let's say have deeply nested expr
- 2 * (3 + -false)
- can't negate "false"
- when try to eval that, need to abort
- thus can't evaluate 3 + _ either
- or 2 *

- nuclear option is to abort process
- definitely ensures don't keep going
- not super user friendly

- most langs do exception handling
- ensures failed operation and everything depending on it is not executed
- but also gives user to handle failure at well defined point and keep going
- would be cool to have for lox -- fun to impl -- but left it out to keep book
  a little shorter

- in lox, runtime error can't be caught by user code
- however, don't want to take down entire process
- when running in repl, just aborts current prompt

- since interp evals exprs using recursive fn calls, need to escape all of them
  and get back to top
- so we will use java execp internally

- java cast failure will throw exception, so could use that
- but doesn't have context, in particular source loc we want to tell user
  where runtime error happened
- instead, check types ourselves and throw own exception
- in unary -, need to check type before casting

^code check-unary-operand (1 before, 1 after)

- code to check op looks like

^code check-operand

- on failure, throws new exception class

^code runtime-error-class

- [name a little confusing because java also has runtimeexception. annoying thing about lang impl is often run into names that are already in use by host lang. wait until we get to implementing lox classes in java.]

- give it a token that identifiers where in program error came from
- like in parser, use that to tell user where to fix code

- need to do same thing for binary ops

- `>`

^code check-greater-operand (1 before, 1 after)

- `>=`

^code check-greater-equal-operand (1 before, 1 after)

- <

^code check-less-operand (1 before, 1 after)

- <=

^code check-less-equal-operand (1 before, 1 after)

- -

^code check-minus-operand (1 before, 1 after)

- /

^code check-slash-operand (1 before, 1 after)

- *

^code check-star-operand (1 before, 1 after)

- all use

^code check-operands

- + just throws since tested types already

^code string-wrong-type (3 before, 1 after)

- now correctly detect and throw runtime errors for all type errors
- what happens to error?
- for that, need to wire up interp

## Hooking Up the Interpreter

- have guts of interp
- need to give skin
- public api is

^code interpret

- if expr evals correctly, produces obj
- convert that to string then display to user
- to convert to string

^code stringify

- cross membrane between lox obj rep and java
- again, fairly similar
- little hack because java insists on ".0" for doubles
- [main reason have this hack is so that print numbers same way in jlox and clox. low level compatibility like this across impls is annoying but important part of job. users relies on these details and if impls aren't consistent, breaks their code.]

- now back in main lox class
- if runtime error thrown, caught by interp
- lets us report error gracefully
- lets repl keep running after runtime error
- interp shows users by calling

^code runtime-error-method

- uses token to tell user line number of code that was executing when error
  occurred
- better would be to show entire callstack, don't have calls yet!
- sets hasRuntimeError field, similar to hasError for syntax errors

^code had-runtime-error-field (1 before, 1 after)

- hasError was used to not execute any code if syntax was wrong
- too late for that
- what for?

^code check-runtime-error (4 before, 1 after)

- when running from script, set exit code if runtime error occurs
- repl doesn't care

- lox needs to create interp

^code interpreter-instance (1 before, 1 after)

- make it static so that each call to run() in runPrompt() reuses same interp
- not needed now, but will be important when interp has state like globals

- final bit of code
- remove temp code for printing parsed syntax tree

^code interpreter-interpret (3 before, 1 after)

- look, have entire lang pipeline now
- scan, parse, execute

- congrats, now have own personal calculator
- really simple
- not too much to this chapter, but ok
- interpreter class and visitor pattern is skeleton
- going to fill it up with more interesting organs in later chapters
- but already alive now

**TODO: illustrate**

- challenges
  - allowing comparison of types other than num can be useful for things like
    sorting collections
    - even comparing mixed types can be useful for sorting heterogeneous
      collections
    - allow comparing nil, numbers, and strings
    - what are your ordering rules? justify your choices and compare to other
      languages
  - many langs define "+" such that if one operand is a string, the other is
    converted to a string
    - make lox do that
  - what happens if you divide by zero?
    - what do you think should happen?
    - how do other languages you know handle that, and why do they make the choices they do?
    - make lox handle division by zero and report a runtime error
