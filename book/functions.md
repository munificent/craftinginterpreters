^title Functions
^part A Tree-Walk Interpreter

- this chapter culmination of all of our hard work
- previous added useful features, but each one also prereq for this one
- finally have enough in place can add support for defining and calling fns
- use almost every feature of interpreter from earlier chapters

## calling fns

- chicken egg problem
- if add calls first, nothing to call
- if add fns first, can't call them
- have to pick, so start with calls

- familiar with syntax, but both complex and more subtle than may realize
- whiel typically calling things by name, like `foo(arg)`, name isn't part of
  call syntax
- thing being called -- callee -- can be any expression
- (well, any high enough precedence expr)
- it's parentheses after callee that indicate fn call
- fn call is a postfix operator
- highest precedence operator
- higher than unary
- if unary op doesn't match, instead of primary, bubbles up to call now

```lox
unary     = ( "!" | "-" ) unary | call ;
call      = primary ( "(" arguments? ")" )* ;

- that is primary expr followed by zero or more fn calls
- if there are none, is equivalent to plain primary expr
- otherwise, is pair of parens with optional argument list inside
- can have multiple calls, as in `foo(1)(2)`
- not common, but does happen if fn returns other fn
- [currying]

- arg list is

```lox
arguments = expression ( "," expression )* ;
```

- at least one expr
- followed my zero or more others separated by commas
- call rule itself makes `arguments` optional to allow calls with no args
- wish there was clean notation in bnf for comma-separated list of zero or more
- always seems awkward

- corresponding ast node

^code call-expr

- has expr for callee, thing to left of parens
- then list of exprs for arguments
- also stores token of left paren
- use that for location when reporting runtime errors

- over in parser, first wire into unary
- instead of calling primary, call... call

^code unary-call (3 before, 1 after)

- calls

^code call

- parses primary expression
- then starts looking for calls
- as long as it sees `(`, parses arg list and wraps in call ast node
- similar to infix ops
- except arg list on "right" instead of single right operand
- code could be simpler, using `while (match(LEFT_PAREN))`
- did it this way because when get to method calls, will add other else case to
  that if
- thinking ahead!

- work to handle arg list is in this method

^code finish-call

- similar to `arguments` rule, but also handles zero-arg case
- look for right paren to see if in zero-arg case
- if so, don't parse any args
- otherwise, keep parsing args as long as we see commas
- then consume final ")" and wrap in call node

- in theory, could keep parsing args forever
- is there limit to how many args can pass to fn?
- c says impl must support at least 127, but free to handle more
- java says 255 (254 for instance methods) is max
- for lox, pick smaller upper limit

^code check-max-arity (1 before, 1 after)

- java interp doesn't care
- but when get to c interp, small limit keeps code simpler

- note report error, but don't *throw* it
- don't want to go into panic mode here
- if user has too many args, parser isn't in weird confused state
- so simply parse them all then report error and continue

### interpreting calls

- don't have anything can call yet, but worry about then when get there
- see how to interpret

- need an import first

^code import-array-list (1 after)

- new expr gets visit method

^code visit-call

- first, evaluate callee
- typically just variable name expr that refers to fn, but could be other expr
- then evaluate list of arguments passed to fn
- [evaluation order]
- finally, to call, cast callee to "callable" and invoke "call" method on that

- not java std lib "callable" interface
- [lot of good names taken]
- our own interface for object that can be called like fn
- use abstraction here because have couple of callable things
- user-defined fns, naturally
- but when add classes, class object can be called to to create instance
- also use for "native" fns, see soon

- interface is simple

^code callable

- takes in interpret so class implementing this can access interp state
- takes list of arguments passed to callable
- returns result value

- before see class that implements this, need to be more robust
- swept lot of potential problems under rug
- first, what if thing being called isn't callable?
- what if do:

```lox
"totally not a function"()
```

- java string doesn't implement callable, so cast will fail
- don't want interp to die with nasty java callstack
- can't rely on static type checker to catch this before program runs
- check dynamically

^code check-is-callable (2 before, 1 after)

- if try to call lox object that isn't callable, generate runtime error

### checking arity

- another problem related to *arity*
- arity is number of arguments or operands an operation expects
- here:

```lox
fun add(a, b, c) {
  return a + b + c;
}
```

- defines three parameters, `a`, `b`, and `c`, so expects three arguments
- arity is three
- what if do:

```lox
add(1, 2, 3, 4); // Too many.
add(1, 2);       // Too few.
```

- different langs take different approaches
- static types ones, of course, check at compile time
- js ignores extra arguments
- if too few arguments, parameters for missing arguments get "undefined"
- python raises runtime error if too many or two few
- find passing too many or two few arguments is common source of bugs
- don't want lox to be lax
- take python approach
- before calling, check argument list against callable's expected arity

^code check-arity (2 before, 1 after)

- requires new method in interface

^code callable-arity (1 before, 1 after)

- [could have implementor of callable be reponsible for checking arity
  - would then only need one method
  - but redundant code across impls of interface
  - doing in call expr lets us do it in one place

## native fns

- before get to user defined fns, now good time to introduce very important
  aspect of interp
- **native functions** are functions interpreter exposes to user code but that
  aren't implemented in interpreter's own implementation language, not lang
  being impl
- here, means native fn can be called from lox but is impl in java
- sometimes called "primitive", "external", or "foreign"
- ["foreign" versus "native" is interesting
  - depends on perspective of who coins term
  - if think of self as living within lang being implemented, then fns not
    impl in that lang are "foreign"
  - if think of self as living inside lang implementation outside of lang's
    own bubble, then these fns are in same lang as rest of impl, so are
    "native"

- these fns form part of language's runtime, set of facilities impl provides
  that can be used while program is running

- many langs also allow user-defined native fns
- lets you interface with code and libraries not written in same lang
- system for doing so called "foreign function interface", "native extension",
  "native interface", etc.

- won't define full ffi for jlox, but will define one native fn to give idea
- when get to part three and care about perf, need to write **benchmark**
- program that does exercises some language feature and whose exec time measure
- could measure from outside of lox by seeing how long entire executable run
  takes
- but exec time of actual code obscured by os app startup, jvm startup etc.
- nice to be able to time region of code within lox
- currently, no way to tell time in lox program
- can't implement in lox -- need access to os's clock
- add clock(), native fn that returns number of seconds app has been running
- can use difference between two calls to tell how much time elapsed

- when create new interp, automatically register function in global scope:

^code interpreter-constructor

- defines variable named "clock"
- functions live in same namespace as other variables
- [unlike some langs where fns and vars have different namespaces]
- value of variable is java anonymous class that implements callable
- interface is handy now
- clock takes no args, so arity is zero
- call method uses corresponding java function and converts to double and
  seconds

- now have one function can call
- but no fair for interp to have all fun
- users want to define fns too

## function declaration syntax

- finally get to add new production to `declaration` rule
- functions, like variables, bind a new name
- [just sugar for lambda + var]
- only allowed in places where declaration is allowed

```lox
declaration = funDecl
            | varDecl
            | statement ;
```

- new rule is

```lox
funDecl     = "fun" function ;
function    = IDENTIFIER "(" parameters? ")" block ;
```

- `funDecl` uses helper rule `function`
- declaration is `fun` keyword followed by function declaration itself
- will later use `function` for class methods, which are not preceded by `fun`
- [too classy to be fun]
- function itself is named followed by parenthesized parameter list
- then body
- body is always braced block, using same rule as for block statement
- `parameters` is similar to `arguments` expect each param is an identifier,
  not an expr

```lox
parameters  = IDENTIFIER ( "," IDENTIFIER )* ;
```

- lot of new grammar to chew through
- ast not too complex, though

^code function-ast (1 before, 1 after)

- has name, list of parameters (their names), body statement

- to parse, add new clause in `declaration()`

^code match-fun (1 before, 1 after)

- that calls

^code parse-function

- corresponds to `function` grammar rule since we already matched `fun`
- lot in there, but straightforward
- first, consume required identifier for name
- then parameter list surrounded by parens
- have that goofy if wrapped around a do-while pattern to handle zero or
  more comma-separated items, like in arg list
- here, though, params is list of tokens, one for each param name, not exprs
- then consume `{` before parsing rest of block body
- have to do that here since `block()` assumes `{` has already been consumed
- [also lets us show good error message]

- also pass in "kind"
- will later use this method for parsing methods, and want error message to
  adjust for that

## function objects

- before can interpret fn decl statement, need to think about object rep for
  fns
- what does a user-defined lox fn look like in java?
- needs to keep track of parameters so we can bind them to args
- needs to keep track of code for fn -- ast of body so can execute it
- basically what Stmt.Function class itself is
- just use that?
- not quite, also needs to implement callable so can actually call
- so wrap in new class:

^code lox-function

- here's how implement call

^code function-call

- one of most fundamental pieces of code interp
- as saw in statements and state and will see even more in next chapter,
  managing environments core part of lang impl
- fns deeply tied to that

- when execute fn, some magic needs to happen with environments
- create a new environment
- in that environment, each fn parameter gets bound to corresponding arg
- fn parameters are like local variables that are implicitly initialized to
  the values you pass to fn
- then body of fn is executed in that environment

- this is key reason why we even have local scope
- parameters need to be in scope inside body of fn, but not elsewhere
- likewise, need to create environment for fn call *dynamically*
- otherwise, can't have recursion
- each recursive call needs own unique environment at runtime, even though all
  for same fn declaration

- [early fortran did no support recursive fn calls. early machines had no
  concept of stack. without recursion, each fn's parameters could be statically
  allocated (think like "static" modified on variable in c) at compile time.
  was contentious when added to algol 60]

- little method does that
- creates new environment empty environment, whose parent is global scope
- [will refine later]

- walks through arg and param lists in lockstep
- for each one, creates new binding in env with param name and arg value
- then tell interp to execute fn body in that new env
- after that's done, environment is discarded

- assume param and arg lists same length
- remember, can do that because `visitCallExpr()` checks that before calling
  `call()`
- need to implement `arity()` for that

^code function-arity

- finally, make fns little nicer if you print them

^code function-to-string

- will refine soon but enough to get started interpreting
- over in interp, need to keep track of global env independent of current env

^code global-environment (1 before, 1 after)

- where `environment` will get assigned as enter and exit scopes, `globals`
  always points to outermost env

- now can interp decl

^code visit-function

- basic idea is simple, similar to other literals
- take fn *syntax node* and convert to runtime object rep
- here, LoxObject
- fns little different from other literals because decl also implicitly binds
  it to name
- so after creating LoxObject, create new binding in current env and store
  ref to fn

- now can define and call our own fns
- try:

```lox
fun sayHi(first, last) {
  print "Hi, " + first + " " + last + "!";
}

sayHi("Dear", "Reader");
```

- like a real programming language!

## returning values

- we can pass data into fns using parameters, but no way to get data out
- since body of fn is statement, `call()` on LoxFunction just executes then
  returns nil
- if lox was expression-oriented like ruby or scheme, body would implicitly
  evaluate to value and we'd return that
- because have statements, need return statements
- do that now

- grammar simple

```lox
statement   = exprStmt
            | forStmt
            | ifStmt
            | printStmt
            | returnStmt
            | whileStmt
            | block ;

returnStmt  = "return" expression? ";" ;
```

- get one more -- final in fact -- statement production
- return statement itself is `return` keyword followed by return value, then `;`
- like most languages, make return value optional
- in statically typed languages, "void" fns that don't return a value must use
  return without value expression and non-void fns must provide a value
- since lox dynamically typed, little looser
- every fn implicitly returns some value even if doesn't have return
- use nil
- so return statement without value follows same path, implicitly returns nil

- syntax tree node keeps token for "return" keyword for error reporting
- then has value

^code return-ast (1 before, 1 after)

- parse it like other statements, starting with the keyword

^code match-return (1 before, 1 after)

- that calls

^code parse-return-statement

- since lots of tokens may begin expr, hard to tell if return value is *present*
- instead, check if *absent*
- if token after `return` is semicolon, must not have return value
- in that case, use null

- when executing, little trickier
- return stmt can occur anywhere inside fn, even nested inside other statements
- when executed, needs to jump all the way out and cause fn to return
- when you hear "jump all the way out" in java, think exceptions
- use exception to unwind past containing statements and get back code where
  began exec body

- here's how to interp

^code visit-return

- if we have return value, evaluate it
- then wrap in exception and throw it
- exception class is

^code return-exception

- mostly wraps return value in custom type we can catch
- super constructor call with booleans disables some stuff to make it a little
  faster
- we're using except for control flow, not real error handling, so don't need
  stack trace

- back in loxfn where impl call, catch exception

^code catch-return (3 before, 1 after)

- when we catch return, unwrap return value and return that from `call()`
- if never catch return, then reached end of fn without encountering return
  stmt
- in that case, still implicitly return nil

- give it a try

```lox
fun average(a, b) {
  return (a + b) / 2;
}

print average(2, 6);
print average(5, 10);
```

## local functions and closures

- made lot of progress
- one loose end to tie up
- will need to revisit it again in next chapter
- when first added support for exec fn, made sure it created new env
- glossed over important point
- what is parent env?
- in above code, simply said "globals"
- but consider:

```lox
fun outer() {
  var a = "value";

  fun inner() {
    print a;
  }

  inner();
}

outer();
```

- in lox, fn decl is statement and can occur both at top level and in block
- can have nested or local fns
- this code is allowed
- what happens if you run this with interp have so far?
- what should happen?

- when call inner(), parent env is globals
- does not include a
- users do expect this to work
- when fn is declared inside some scope, should have access to surrounding scope

- note that fn may need to hang on to that scope even after execution has left
  scope
- classic example:

```lox
fun makeCounter() {
  var i = 0;
  fun count() {
    i = i + 1;
    print i;
  }

  return count;
}

var counter = makeCounter();
counter(); // "1".
counter(); // "2".
```

- here `count()` refers to `i`, which is declared outside in containing
  `makeCounter()`
- `makeCounter()` then returns reference to count fn
- (remember, fns are first-class and can be passed around, returned, etc.)
- we take that returned fn and call it
- that assigns an accesses `i`, even though `makeCounter()` has already returned

- for this to work, `count` needs to hang on to environment where `i` is
  declared even after code that creates environment has discarded it
- the runtime rep of a fn needs to have both the code for the fn, and that
  environment
- this data structure is called "closure" because it "closes over" and holds on
  to surrounding variables it uses
- lots of ways to implement
- do something much more efficient in part 3
- for now, do simplest thing
- loxfn will also store ref to env where fn is declared

^code closure-field (1 before)

- add field to store environment
- need ctor to init

^code closure-constructor (1 after)

- when create loxfn, pass in current env

^code visit-closure (1 before, 1 after)

- note that this is env in scope when fn is *declared* not when *called*

- finally, when call fn, create env for funtion scope where params bound
- parent of that is now env containing fn decl instead of hard-coding to global
  scope

^code call-closure

- now try running previous `makeCounter()` example
- works!

- means we have made lox much more powerful
- fns let us abstract over, reuse, and compose code
- but closures mean fn can also abstract and reuse data
- surprising, but can use closure to rep data structure
- since it contains env, *is* data structure

- will add direct support for data comp later when add classes
- but even now can make fns that work like objects
- [poor man's]
- cogitate on

```lox
fun makePoint(x, y) {
  fun closure(method) {
    if (method == "x") return x;
    if (method == "y") return y;
    print "unknown method " + method;
  }

  return closure;
}

var point = makePoint(2, 3);
print point("x"); // "2".
print point("y"); // "3".
```

- have crossed real threshold
- lox much more powerful than simple calculator used to be
- alas, in rush to cram closures in, have let slip in tiny bit of dynamic
  scoping
- next chapter we'll look more closely at scope and fix

<div class="challenges">

## Challenges

1.  Our interpreter carefully checks that the number of arguments passed to a
    function matches the number of parameters it expects. Since this check is
    done at runtime on every call, it has a real performance cost. Smalltalk
    implementations don't have that problem. Why not?

1.  Lox's function declaration syntax performs two independent operations. It
    creates a function and also binds it to a name. This improves usability for
    the common case where you do want to associate a name with the function.
    But in functional-styled code, you often want to create a function to
    immediately pass it to some other function or return it. In that case, it
    doesn't need a name.

    Languages that encourage a functional style usually support "anonymous
    functions" or "lambdas" -- an expression syntax that creates a function
    without binding it to a name. Add anonymous function syntax to Lox so that
    this works:

        :::lox
        fun count(fn) {
          for (var i = 1; i <= 3; i = i + 1) {
            fn(i);
          }
        }

        count(fun (a) {
          print a;
        });
        // "1".
        // "2".
        // "3".

1.  Is this program valid?

        :::lox
        fun scope(a) {
          var a = "local";
        }

    In other words, are a function's parameters in the same scope as its local
    variables, or in an outer scope? What does Lox do? What about other
    languages you are familiar with? What do you think a language *should* do?

</div>
