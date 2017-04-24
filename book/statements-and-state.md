^title Statements and State
^part A Tree-Walk Interpreter

**todo: figure out illustrations**

- last chapter created interpreter
- can evaluate code like calculate
- doesn't really feel like programming
- can't build up program
- no abstraction
- can't define thing -- function, value by name
- then use later
- for bindings need state
- between chunks of code -- need to remember that certain names associated
  with certain things
- [don't necessarily mean mutable state]

- this chap, start introducing state so you can write bigger programs
- start to make lox feel like real lang
- also good time to introduce statements
- when introducing binding like var decl, intent is to modify state, not
  produce value
- [could have var decl be expr that produces value too]
- statements do that
- unlike exprs statements don't produce value
- to be useful, must do something else
- "side effect"
- change the world in some visible way
- produce output
- modify state
- since we are adding state, now good time to add statements
- lot to get through
- [expressions can have side effects too
- calling fn is expr, but function may produce output or do something else]

## statements

- simplest part of chapter is adding statements as grammatical category
- not really very diff from expr

- start with two simplest stmt types:
- expr stmt lets you put single expr where stmt expected
- mainly so you can call fn for its side effect
- exists in java and c, though probably don't think about
- [pascal func / proc sep]

- print evals expr and prints result
- weird to bake into lang, but as see, let's us start testing that side effects
  are working before we have functions implemented
- concession to book structure of building lang incrementally
- in real lang, would make fn

- need new grammar rules for these
- finally, can parse entire lox script
- since imperative, dynamic lang, top level is list of statements
- [top of static lang usually decls]

```lox
program     = statement* eof ;

statement   = exprStmt
            | printStmt ;

exprStmt    = expression ";" ;
printStmt   = "print" expression ";" ;
```

- now entry rule is "program" which represents entire lox script
- (or single entry in repl)
- program is list of statements
- explicitly match EOF at end
- otherwise, could match a program that ends in unexpected tokens which would
  then be ignored
- want that to be an error

- right now, statement, either expr or print
- both pretty similar: single expr followed by mandatory semicolon
- print stmt starts with "print" keyword
- next is representing in code

### stmt syntax trees

- going to create separate family of syntax tree classes for statements
  separate from expr, called stmt
- some impls unify and have single "tree" or "node" base class that covers both
- like keeping them separate
- grammar rules treat them separately
- no production that allows expr and stmt in same position
- ex: operands of binary are always expr, never stmt
- ex: then and else clauses of if always stmt, never expr
- separate superclass lets syntax tree classes encode that

- also, diff operations apply to each
- ex: when visiting expr, interpreter returns result value
- when visiting stmt, there is no result value, so visitor has different return
  type
- splitting out lets us rep that

- presciently, script to generate syntax tree classes anticipates that
- add separate section to create stmt class and subclasses:

^code stmt-ast (2 before, 1 after)

- run generate ast now and get separate Stmt.java file with syntax tree classes

### parsing statements

- have new syntax trees
- next step is get parser to produce
- old entrypoint to parser was temporary
- new entrypoint
- corresponds to "program" rule

^code parse

- need java boilerplate for lists

^code parser-imports (2 before, 1 after)

- typical recursive descent pattern of method for rule

^code parse-statement

- will add more statement types later
- right now, really simple
- if see "print" keyword, must be print statement
- otherwise, assume it must be expr statement
- typical to "fallthrough" to expr statement since it's hard to recognize
  expr explicitly based on next token

- methods for each rule

^code parse-print-statement

- already matched and consumed "print" token
- require expression arg
- then semicolon
- produce tree

^code parse-expression-statement

- likewise, expr followed by semicolon
- wrap in Stmt syntax tree node so that we have obj of type Stmt
- if had single base class for both exprs and statements, could avoid this
- though might be useful if wanted syntax tree to have ref to semicolon token

### visiting statements

- sort of running through previous few chapters in microcosm
- after parsing, ready to interpret
- again using visitor pattern
- separate visitor interface for statements since separate base type
- interp impls

^code interpreter (2 before, 1 after)

- unlike exprs, stmt produce no value
- so return type of visitor void, not object
- [capital Void in java]

- have two stmt types, so two visit methods
- expr

^code visit-expression-stmt

- simply evaluates expr and discards result
- [need explicit return null because of java weirdness around type erasure and
  Void type]

- print

^code visit-print

- similar, but prints result to stdout first
- uses same stringify() method from last chapter to convert val to string

### wiring up to main

- main lox class set up to interpret single expr at a time
- need to use new entrypoint for list of stmts
- add old expr interp method with

^code interpret

- executes list of statements, one at a time
- like old interp method, catches and reports runtime error
- relies on

^code execute

- similar to evaluate() but for stmt

- also need some java boilerplate to use List

^code import-list (2 before, 2 after)

- over in main lox class, replace expr parsing code with:

^code parse-statements (1 before, 2 after)

- and interp line with

^code interpret-statements (2 before, 1 after)

- fire up interp and give it a try
- can now write scripts that look more like real progs, like:

```lox
print "one";
print true;
print 3;
```

- ok, not very useful, but making progress
- note from here on out, when using repl, have to give it full stmt instead of
  expr

## global state

- starting to have imperative lang where can do one thing after another with
  side effects
- next step is side effects for defining state
- before get into lexical scope, blocks, etc. start simple
- giant bag of global scope
- even so, still have to think deeply about interesting semantic corners
- [good enough for BASIC]

- syntactically, want to add two new forms to language:
- var statements bring new variable into being

```lox
var beverage = "espresso";
```

- define binding that associates name with some value
- variable expressions -- identifier -- access binding
- look up value associated with name and return it

- will refine that later with assignment, lexical scope, etc. but start with
  just those

### syntax

- as before, do this front to back
- syntax then semantics
- with addition of declarations, actually splitting statement grammar in two
- have sort of two "levels" of statements, almost like expression precedence
- there are "declaration" statements that introduce new names:
- obv "var" but later "fun" and "class" too
- then other statements like print, control flow, etc.
- make distinction because we don't allow declarations as clauses of control
  flow statements
- this is error:

```lox
if (monday) var beverage = "espresso";
```

- could allow this, but don't because scoping gets confusing
- does beverage go out of scope after if? if so, why allow at all
- c et al disallow this, so we do too
- [can of course use block]

- grammar for statements looks like:

```lox
program     = declaration* eof ;

declaration = statement ;

statement   = exprStmt
            | printStmt ;
```

- so top level is list of decls
- since decl falls back to statement as last option, allows statements there
  too

- now extend grammar with var decls and use
- decl becomes:

```lox
declaration = varDecl
            | statement ;

varDecl     = "var" identifier ( "=" expression )? ";" ;
```

- var declaration is "var" followed by name then semicolon
- also allow optional initializer expression between name and ";"

- and add new clause to primary expr for accessing var

```lox
primary     = "true" | "false" | "null" | "this"
            | NUMBER | STRING
            | "(" expression ")"
            | IDENTIFIER ;
```

- identifier at end matches single var name token

### parsing var stmt

- two new grammar productions lead to new syntax trees
- new statement form for declaring variable

^code var-stmt-ast (1 before, 1 after)

- has a token for name of variable being declared, and then expression for
  initializer

- expression form for accessing

^code var-expr (1 before, 1 after)

- simply wrapper around token for name of var being accessed
- [aside about naming tree: "var", "identifier", "name"]

- don't forget to run ast generator

- to parse, need to add those, but also need to shift stuff around because of
  new "decl" level

- top level program rule is now list of decls, so change parse to

^code parse-declaration (1 before, 1 after)

- calls

^code declaration

- finally at point that we can do error sync introduced way back in previous
  chapter
- given chunk of code containing multiple statements, helps recover from error
  in earlier statement before moving on to later ones

- falls back to calling statement, because anywhere we allow a decl, we also
  allow regular statement

- now can add new bit of code to match and parse var decl

^code parse-var-decl

- starts with "var" keyword
- if found, branches to

^code parse-var-declaration

- much like previous rule
- gets variable name following "var" token
- if we see "=", must be initializer, so match that
- otherwise, leave null
- interpreter will check that

- parsing variable use -- identifier expr -- easy

^code parse-identifier (2 before, 2 after)

- as in rule, simply adds another clause to primary expr parsing code

- have front end working
- next feed it into interp
- new visit methods for var stmt and var expr
- before get to those, need infra

## environment

- bindings need to be stored somewhere
- traditional term "environment"
- data structure that holds associations between variables and their
  current values
- think of it like map where keys are var names and values are... values
- in fact, that's how we'll implement it
- could stuff environment directly in interp class, but cleaner to treat it
  as separate concept
- [like to think of "environment" literally, as delightful woodland area where
  variables frolic around with their values hovering above them like balloons
  on strings]
- var stmt will add binding to environment
- identifier expr will look it up

- make new file and enter

^code environment-class

- like said, wraps map of variable names to values
- uses strings for names, not tokens
- want two references to same variable to have same hash key, so unwrap token
  to get to actual name
- not much here yet, but will grow

- two operations
- var statement defines a variable

^code environment-define

- dead simple
- mainly just unwraps token
- interesting semantic choices sneaking in
- does no checking to see if var is already defined
- this is ok:

```lox
var a = "before";
print a; // "before".
var a = "after";
print a; // "after".
```

- var doesn't just define new var, but can redefine existing var
- could choose to make this error instead
- user may not intend to redefine previous var when they do this
- error would bring to attention
- could be runtime error, or even static error
- interacts poorly with repl
- handy to be able to redefine existing var in repl without yelling at you
- could require user to use assignment instead, but not friendly
- could allow in repl but disallow in scripts
- adds complexity, forces users to think about which mode in
- code copied from script and pasted into repl might behave diff
- [once have closures, even more interesting question about whether redefining
  existing var or defining new one]

    var a = "before";
    fun f() {
      print a;
    }
    var a = "after";
    f(); // ???

  - equivalent prog in scheme prints "after"
  - are redefining existing var
  - var decl is treated exactly like assignment (at top level) if var exists
  - in ml, prints "before"
  - second "var a" introduces new a that is only visible to later code
  - existing code, like f() still sees orig
- lox is friendly little scripting lang, so err on side of permissive
- will allow, and treat as same as assignment
- (for globals)

- once var exists, need way to look it up

^code environment-get

- little more semantically interesting
- if var is found, simply returns value bound to it
- what if not found?
- again, have choice
- could be syntax error, runtime error
- could even allow it by returning some default value like nil
- lox permissive, but seems like asking for trouble
- syntax error seems like smart choice
- using a var not defined almost always mistake
- problem is *ref* var not same as *using*
- can refer to var while not immediately using it by having it wrapped in fn
- if we make it static error to refer to var not declared yet, recursive fns
  much harder

- can handle *single* recursion by putting fn's own name in scope before looking
  at body of fn
- doesn't help with mutual recursion

- consider:

```lox
fun isOdd(n) {
  if (n == 0) return false;
  return isEven(n - 1);
}

fun isEven(n) {
  if (n == 0) return true;
  return isOdd(n - 1);
}
```

- [contrived, of course. and risky in lang without tail recursion.]
  - hypothesis: there are no recursive functions that only make single recursive call that do not look
    constrived in imperative lang

- isEven isn't defined yet by time looking at body of isOdd where it's used
- if we move its definition before isOdd, run into same problem with isEven
  in body of isOdd

- fully statically typed langs handle this by making top level purely contain
  decls and no imperative code
- decls can all be simultaneous -- lang puts them all in scope before looking
  inside any bodies
- how java and c# work
- [old langs like c and pascal don't work like]
  - even though top level only decls, still require explicit forward decls
  - tech limitation of time
  - wanted to compile in single pass through source file, so couldn't gather
    all decls before compiling any bodies

- instead, make it runtime error
- means it is fine to refer to variable that isn't defined yet, as long as code
  containing ref isn't evaluated before definition occurs
- above isOdd() isEven() works fine
- but get runtime error here:

```lox
print a;
var a = "too late!";
```

- unlike define() which immediately yanks out lexeme, passing in token important
  here
- when var isn't defined, use its position information to report runtime error
  at right location

## interpreting global vars

- that's very minimal env for global variables
- enough to add support to interp for new syntax
- first, interp needs to keep track of global env

^code environment-field (1 before, 1 after)

- add field to class
- this way, globals live as long as interp does

- two new visit methods, one for var stmt, one for identifier

^code visit-var

- if var has initializer, evaluates it
- otherwise uses null, which is java rep for lox nil
- again, semantic choice appears
- decided that uninitialized var defaults to nil

```lox
var a;
print a; // "nil".
```

- could easily make this syntax error
- simply *require* `=` and initializer expr
- most langs don't, so feels too harsh
- could make runtime error
- allow declaring uninitialized var, but require assigned value before used
- if not, access is runtime error
- not bad idea
- [might slow access to var, have to check if init]
- for simplicity's sake, say uninitialized var is implicitly nil

- to access var:

^code visit-variable

- simply forward to env, which does heavy lifting to check for defined var

- try out interp
- starting to feel like real lang
- can store values in vars and access them

```lox
var a = 1;
var b = 2;
print a + b;
```

- can't reuse code yet, but can start to build up programs by reusing data

## assignment

- it's possible to make lang without any ability to assign to -- mutate -- vars
- haskell one example
- mutating existing var is side effect
- as name implies some think side effects are inelegant
- "impure" is other synonym
- both come from func comm which prefers computation to be like math
- [given many claim to hold logically superior position, funny how they can't
  resist using emotionally loaded terms for things they don't like]
- lox not so austere
- mutation typical part of imperative langs

- semantically, not much to do
- only have global vars and already decided they can be redefined, which
  reassigns
- machinery all there
- but want real syntax for explicit assignment to existing var
- when add local vars, will be semantic differences
- as see, assignment syntax more complex than might first seem

### assign syntax

- like most c-derived langs, assignment is expression, not statement
- [is statement in python, expr in go but ++ is statement]
- as in c, lowest precedence expr form
- so change expr grammar to:

```lox
expression  = assignment ;
assignment  = identifier ( "=" assignment )?
            | equality ;
```

- expr falls through to lowest prec rule, assign
- that can be var name followed by `=` and new value, or any higher prec op
- next level is equality, so falls to that
- later, will get more complex when support property setters on objects like

```lox
instance.field = "value";
```

- easy part is adding new syntax tree node

^code assign-expr (1 before, 1 after)

- run the ast generator to output new Expr.Assign class

- swap out body of expr to call new assign method

^code expression (1 before, 1 after)

- here where it gets tricky
- single token lookahead recursive descent parser isn't well set up for
  handling assign exprs
- problem is that, unlike other operators, lhs of assignment isn't express that
  evaluates to *value*
- it's sort of pseudo-expression that evaluates to thing that can be assigned
  to
- in:

```lox
a = "value";
```

- we don't *evaluate* a by looking up its *value* before executing the `=`
- we just want to know var being assigned to so know where to store "value"
- classic terms "lvalue" and "rvalue"
https://en.wikipedia.org/wiki/Value_(computer_science)#lrvalue
- thing on lhs of assign "evaluates" to lvalue -- destination that value can
  be stored in
- rhs is normal expression -- rvalue
- names from them appearing on left and right side of `=`

- right now, only kind of lvalues we have are simple var names
- when add classes and fields will have lvalues that may contain complex
  expressions like

```lox
makeList().head.next = node;
```

- because lvalue isn't evaluated like normal expr, want different syntax tree
  nodes
- (note how assign expr has *token* for lhs, not Expr)
- problem is parser doesn't know it's parsing lvalue until it hits `=` which
  may occur much later
- without arbitrary lookahead, how to tell?

- will use trick, looks like

^code parse-assignment

- code for assignment looks almost like other binary operators
- parse lhs, which can be any expr of lower prec
- if match `=`, then parse rhs and wrap both in expr for `=`
- [one diff from other binary ops is `=` is right assoc. note how in grammar,
  assignment recursive production is on right side of `=`, not left as in, say
  `+`. that's why recursively call assignment() for lhs instead of doing while
  loop]

- trick is that before creating assign expr, look at lhs expr and figure out
  what kind of assign target it is
- since every valid assign target is also valid
  expression, we know this works
- opposite is not true
- many expressions that are not valid assign target, like `1 + 2 = 3;`
- so if valid target, produce appropriate assign expr
- otherwise, fail with syntax error
- right now, only valid target is var expr, will add more later with fields on
  classes

- this is why in first parsing chapter we made expr node for parentheses instead
  of discarding during parse
- when get here, need to be able to tell that `a = 1;` is valid but `(a) = 1;`
  is not

- [possible for lang to have assignment target syntax that is not also valid
  expression
- for ex, maybe we support multiple assignment like `a, b = 3, 4`, but don't
  have any general "comma" expr
- in that case, we'd get parse error on first `,` before reached `=`
- can solve that by defining "cover grammar"
- parser first accepts looser faux grammar that covers both all exprs and all
  possible assign targets
- make fake comma expr node
- then later check that non-expr faux exprs only appear as assignment targets
  like opposite of above check]

- end result is lhs expr node is discarded and we create assign node that knows
  what it is assigning to and the expr for value being assigned

### assignment semantics

- to support new expr in interp, need new visit method

^code visit-assign

- similar to var declaration (except always know have rhs now)
- evaluate rhs to value
- store in variable

- can't use `environment.define()` as see soon, so add new method

^code environment-assign

- like `define()` stores value in map under var's name
- main difference is assign cannot create new var
- key must be present, or is runtime error
- lox does not do implicit var decl
- assign() handles that

- finally, back in visit method, last thing is to return rhs value
- this is because assign is expr
- can be used as sub-expr of other expression so needs to produce value
- as in c, value is result of rhs

```lox
var a = 1;
print a = 2; // "2".
```

- with only one flat set of global variables, state pretty simple
- but also pretty messy and hard to scale
- not many people remember writing big programs in old basics, but got hard to
  not accidentally reuse existing var
- want local vars
- time for scope

## scope

- scope -- region of code where certain names refer to certain entities
- different from env
- env data structure storing actual values at runtime
- scope is more about textual understanding of code
- closely related
- as interpreted works through source, syntax that affects scope will change
  environment
- in c-ish language, scopes introduce use blocks
- [hence "block scope"]
- fn bodies also introduce scopes, but c and lox conveniently use same curly
  brace syntax for those too
- fns are real core motivation for having scope
- fns have parameters -- variables that store values passed to fn
- if want to support recursion, may have multiple calls to same fn going on at
  same time
- each needs its own set of parameters
- so really need some kind of fn scope at minimum
- [early fortran did no support recursive fn calls. early machines had no
  concept of stack. without recursion, each fn's parameters could be statically
  allocated (think like "static" modified on variable in c) at compile time.
  was contentious when added to algol 60]
- much simpler, but really limiting
- imagine trying to write recursive descent parser without recursion

- don't have functions yet, but can still do scope because lox is block scoped
- curly braced block has two functions
  - allows you to provide series of statements where one is expected
    - (mainly useful in control flow statements)
  - introduces new local scope for variables declared in block
  - variables declared inside block disappear "go out of scope" at end of block

    ```lox
    {
      var a = "in block";
    }
    print a; // Error! No more "a".
    ```

  - [some langs, like python and js until recently only have fn scope. blocks
    let you group statements, but don't affect scope]

### nesting and shadowing

- might think simple way to implement scope is to:

- as visit statements in block, build up list of variables that get declared
- after last statement, tell environment to remove all those variables
- seems right, but recall key goal of local vars is to help encapsulate code
- block of code in one place shouldn't interfer with another

- consider:

```lox
var counter = 0;

// Everytime this is called, increments and prints counter.
fn increment() {
  counter = counter + 1;
  print counter;
}

fn countToTen() {
  for (var counter = 1; counter <= 10; counter = counter + 1) {
    print counter;
  }
}
```

- with proposed behavior, if you call countToTen(), after its done, it will
  delete *global* counter variable!
- not right
- variable in block should not interfere with variables in outer scope
- means inside block, shouldn't see them
- but code outside still can
- need to keep them around

- called "shadowing"
- if same variable is declared in multiple scopes, inner declaration shadows
  outer one

**todo: illustrate**

- think of it as casting shadow that obscures outer ones
- uses of name always refer to innermost declaration
- but outer declarations are still there
- later uses of same name outside inner block can still see and use outer
  var

- interpreter has to keep around outer variables even when shadowed by inner
  ones
- to do that, instead of local scope *modifying* environment, it will create
  new one
- new environment contains only variables declared in that local scope
- outer environment still exists
- when looking up var, use this local block to find it
- when we reach end of block, that local environment is discarded, but outer
  one(s) remain
- that way, var can be shadowed within local block and then reemerge at end of
  it
**todo: illustrate**

- other piece is outer vars that are not shadowed:

```lox
var global = "outside";
{
  var local = "inside";
  print global;
}
```

- when block begins, create new env for it
- that's where local ends up
- but `print global` won't work if only look in that env
- global isn't defined there, it's in outer env
- so var lookup needs to be able to see not just innermost env but all
  surrounding ones
- keeps looking through them until finds var

- do this by chaining environments together
- each env for local scope has reference to env of immediately surrounding
  scope
- when looking up or assigning var, walk chain from innermost to outermost
  until we find var
- if hit null ref, means we are at global scope
- if didn't find var there, it's unedefined

**todo: illustrate**

### nested env

- that's idea, now can beef up env class to handle all this
- first, give env ref to enclosing env

^code enclosing-field

- now that have field, need constructors

^code environment-constructors

- default no-arg ctor for global scope has null enclosing
- other ctor is for creating local scope nested inside some outer one -- either
  global or other local

- now can tweak two env operations, lookup and assignment
- first get

^code environment-get-enclosing (2 before, 1 after)

- if var isn't found in this env and there is outer one, try there instead
- [could be faster to iterate instead of recurse, but this interp is about
  clarity]

- likewise assignment

^code environment-assign-enclosing (2 before, 1 after)

- if didn't find var in this env map's keys, try the outer one
- this highlights difference between var decl and assigment
- when defining, always define in innermost
- with assign, may modify outer env if defined there

### implementing block scope

- now env is ready, can add syntax and runtime support for locals
- as in c, block is series of statements (possibly zero), including var decls,
  surrounded by curlies
- block is itself a statement and can appear wherever statement is allowed
- grammar

```lox
statement   = exprStmt
            | printStmt
            | block ;

block       = "{" declaration* "}" ;
```

- before can parse, need to define syntax tree

^code block-ast (1 before, 1 after)

- simply contains list of inner stmts
- run generator
- like other statements, can tell if we hit block by first token
- in this case "{"
- in statement() method, add:

^code parse-block

- calls

^code block

- in separate helper method because also use it later for fn bodies
- it creates list and keeps adding statements to it until it hits the
  closing "}"
- note also explicit check for isAtEnd()
- in recursive descent parser, have to be careful can't get stuck in loop with
  bad code
- if missing "}", still need to exit loop
- every while loop we add to parser will usually have this check

- over in interp, as always, need corresponding visit method for syntax tree

^code visit-block

- environment field in interpreter used to always store global scope
- now, stores *current*, innermost scope

- first thing visit does is make new env nested inside current one
- passes to this helper

^code execute-block

- that executes given list of statements in context of given environme
- it reassigns the env field to the new one temporarily
- executes all statements
- then restores the previous env
- finally ensures does that even if excep is thrown

- [mutating field is little ugly way to do this]
  instead, some tree-walk interps pass env as explicit parameter to each visit
  method
  then impl lang stack automatically handles setting and restoring nested
  envs
- felt like a lot of tedious code for jlox, so set field instead

- now can nest vars arbitrarily deeply
- try out:

```lox
var a = "global a";
var b = "global b";
var c = "global c";
{
  var a = "outer a";
  var b = "outer b";
  {
    var a = "inner a";
    print a;
    print b;
    print c;
  }
  print a;
  print b;
  print c;
}
print a;
print b;
print c;
```

- starting to look like programming
- still more like recipes since code always runs top to bottom one time
- to do more than that, need control flow
- next chapter

<div class="challenges">

## Challenges

1. make repl support both exprs and stmts. if expr, print result value.

2. make it runtime error to access uninitialized var.

3. what does this do?

    ```lox
    var a = 1;
    {
      var a = a + 2;
      print a;
    }
    ```

    what do you expect it to do? what does analogous code do in other languages
    you know? what do you think the user will expect it to do?

</div>

<div class="design-note">

## Design Note: Implicit Variable Declaration

- lox has different syntax for declaring new var and assigning value to
  existing var
- (though former can be used as latter at top level)
- other langs collapse and just have assignment
- assigning to non-existing var implicitly declares it in current scope
- ex: python, ruby, coffeescript
- js has explicit var decl, but also allows assignment to implicitly create
  global var
- [vb has option to enable or disable!]

- because assignment is also used for decl, each lang decide handle how interact
  with shadowing and what scope to put decl in
  - js, if var is defined in any outer scope, assigns to it
  - otherwise, creates at global scope
  - coffeescript assigns to existing var if in any outer scope. if not, goes in
    innermost fn scope
  - python says always local to function, even if surrounding fn has var with
    same name
  - ruby avoids some complexity by having different naming conventions for
    local and global vars
  - but ruby blocks have own scope, so still have to deal with it
  - assignment to var inside block assigns to existing local in outer scope
    if one
  - otherwise, creates new var in block scope
  - similar to coffeescript [which is inspired by ruby in many ways]

- main advantage less syntax, fewer concepts to learn, just assign away
- more static langs like c benfit from explicit decl because place for user to
  to specify var type, and for compiler to alloc storage
- dynamic, gc langs don't need either
- implicit decl feels more lightweight and scripty, more "do what i mean"

- coffeescript deliberately does implicit to prevent shadowing, which creator
  feels makes code harder to read and maintain

- implicit has some problems

- user may intend to assign to existing var and not create new one, but lang
  can't see that intent
  - typo in var name in assignment will silently create new var
  - particularly nasty in js, because creates global var

- in some js, coffee, and ruby, creating var in outer scope can change behavior
  of existing code
  - what was new local var becomes assignment to existing outer one
- in python may want to assign to surrounding var instead of creating new local
  one, but can't

- over time, langs with implic decl added more features to deal with problems

- vb has option to enable or disable implicit
  - defaults to explicit
  https://msdn.microsoft.com/en-us/library/xe53dz5w(v=vs.100).aspx

- implicit decl of globals widely considered mistake in js
  - strict mode completely disables
  - have to make it opt in to avoid potentially breaking existing

- [js also add 'let' for true block scope var instead of 'var' which is fn
  scope]

- python added global statement to support assigning to global var from within
  fn
- later, as nested fns and closures became more common, also added 'nonlocal'

- ruby extends block syntax to allow declaring vars explicitly local to block
  even if same name exists in outer scope

  |x;y|

- simplicity argument mostly lost since almost every lang with impl ends up
  adding more lang features later to let user be more explicit
  - [coffee, which chooses impl for principled reasons interesting exception]
  - granted, don't have to use 'global', 'nonlocal' and block-level vars in
    ruby as often as have to use explicit var decl in other langs
  - argument that they chose right default about what to force users to be
    explicit about

- implicit decl made more sense when code pretty flat, not lot of nested, and
  especially not many nested fns
- as closures and fn style more common, nested fns and access to surrounding
  vars more common

- broadly, one of main challenges in maintaining code is thinking about state,
  especially mutated state
- have to reason about not just what code does, but how current stuff in memory
  affects that
- variables are state, so making them more explicit can help reader know what
  to reason about
- if deep in bowel of nested code, see:

    a = "blah"

  nice to quickly tell if this is declaring new local var or assigning to some
  existing outer or global var

- generally prefer explicit
- why lox does

</div>
