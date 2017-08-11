^title Resolving and Binding
^part A Tree-Walk Interpreter

> Once in a while you find yourself in an odd situation. You get into it by
> degrees and in the most natural way but, when you are right in the midst of
> it, you are suddenly astonished and ask yourself how in the world it all came
> about.
>
> <cite>Thor Heyerdahl</cite>

- added variables and blocks and more or less got scope right
- hole in it
- to fix, think more precisely about scope
- by end of chapter, have much more rigorous understanding of scope in lox and
  in other langs whose scope rules similar -- c, java, etc.
- also static analysis, useful technique for many tools work with langs

## static scope

- lexical scope means can tell what declaration use of variable refers to just
  by looking at source text
- so in:

```lox
var a = "outer";
{
  var a = "inner";
  print a;
}
```

- can figure out that the `a` being printed is `a` declared on previous line
  and not global one
- don't need to run program to figure that out
- shouldn't be able to write a program, no matter how weird or complex, where
  that isn't true

- rule for what var decl var expr refers to is
- var expr refers to preceding declared var in innermost scope
  enclosing var expr

- preceding means

```lox
var a = "outer";
{
  print a;
  var a = "inner";
}
```

- here printed a is outer one
- [unlike js with hoisting]
- "preceding" means in text, not time

```lox
var a = "outer";
for (var i = 1; i <= 2; i = i + 1) {
  if (a == 2) print a;
  var a = "inner";
}

- here, inner a decl will execute before print a execs
- but inner a does not precede print stmt

- "innermost" is for shadowing

```lox
var a = "outer";
{
  var a = "inner";
  print a;
}
```

- here a is inner

- implies variable expression always refers to same declaration throughout
  entire execution of program
- why it's called "static scope"

- [not very precise, real spec would define scope precisely, precede, etc.]

- mostly impl rule right
- but when we added closures, poked hole
- it is possible to write program that breaks preceding rule:

```lox
var a = "global";
{
  fun showA() {
    print a;
  }

  showA();
  var a = "block";
  showA();
}
```

- before run, think about what *should* print
- expect to print "global" twice
- first call to `showA()` should definitely print "global" since haven't even
  reached declaration of inner `a` and evaluated initializer expr yet
- [js hoisting]
- by rule of static scope that variable expr always refers to same decl, then
  second print should be same

- run this now, prints

```
global
block
```

- program never reassigns variable
- only contains one print
- same print for never-assigned var prints two different values
- oops

---

### scopes and mutable environments

- env is dynamic sister of static scopes
- exec order in interp follows textual order often
- control flow changes order var visited
- closure really changes
- when out of sync, problem
- interp looks up var every time used, not just once

- prob because of how chose to impl environments
- chose that because of informal thinking about scope of block
- walk through prog and see what envs look like at each step

**todo illustrate**

- when declare `showA()`, create closure
- captures reference to env for block surrounding fn
- java reference to actual env object
- env is mutable hash table
- when variables later declared in same block, add to same hash table
- means closure can then "see" new vars even though declared after closure

### persistent env

- block not all same scope
- scope after var decl different from before
- contains var

- style of data struct called "persistent"
- immutable
- any time "change", orig left alone produces new one

- could have var decl produce new env
- closure would ref prev one, not see later change
- would work, but require changing lot of code
- instead

- each time declare local variable, produces *new* env
- original env left unchanged
- closure captured previous one where a not declared
- doesn't see new one
- since every time declare var produce new env, each env only contains single
  decl
- instead of chain of maps, have chain of vars
- many schemes work this way

**todo: illustrate**

- little different since lox has block syntax unlike scheme
- at end of "}", need to discard all vars declared in block
- since now spread across multiple env, need to track somehow how many envs
  to pop

- did implement this for lox
- works fine
- one problem and one opportunity

- problem is would have had to implement persistent env way back when vars
  first added
- back then, not clear why need that
- hashmap for env seems more intuitive
- and do want hashmap for global vars which special
- so would have needed more complex impl of envs for locals with no clear
  motivation until much later when add closures

- opportunity is that can solve this problem another way
- solution is example of general technique fundamental part of compiler impl
- lets us keep intuitive impl of envs
- and also teach you new tool

## static resolution

- interp does var resolution each time var used
- always resolves to same decl (should!)
- static scope means can do it textually, why we doing it dynamic?

- instead, "bake" resolution of each var expr to var decl refers to
- lots of ways could represent this association
- could redo entire env rep
- [will for clox]
- try to minimize collatoral damage
- in current lookup code, problem is that doesn't always go same number of hops
  as walks env chain
- if could ensure always hopped three before looking for 'a' would find same
- even though `a` would be in block scope, would skip past it as counted to
  three
- find same `a` every time

- only need to calculate how many envs away variable decl is from var expr
  once, not each time var expr is evaluated
- in other words, static property of var, not dynamic
- which is what want for static scope

- can't calculate which actual env has var because remember envs created
  dynamically during exec
- in recursive fn, may even have multiple envs that contain same var decl but
  for different invocations
- but given some var expr, relative distance from it to var decl is always
  fixed

**todo: illustrate**

- so if can calc how many envs away var is from current env, then can ensure
  always access same var

- interesting q is when to resolve
- in parser (do clox)
- traditional approach
- want excuse to show other technique
- separate pass

## resolution pass

- make resolution separate pass in front end
- after parser, before interp
- useful technique
  - type checker
  - optimization
  - any work doesn't depend on dynamic execution can be done here

- resolver or other analysis pass much like simple interp
- [abstract interp really blurs line between dynamic exec and static analysis]
- walks ast tree visiting each node

- couple things make static instead of dynamic

1. no side effects. doesn't execute statements or native fns. prints don't print
2. no control flow. when resolving if, look at both branches once. only look at
   loop body once. not turing complete!

- [more adv analysis may visit code multiple times for fixpoint. too complex for this book. important part is those algs reliably terminate.]

- single pass, no control flow or side effects

- new visitor
- impls lexical scope resolution rules
- work much like portion of interpreter for vars
- when block entered, create new scope
- when var decl add to scope
- when var used, look it up
- "look it up" now means resolve
- figure out which decl var refers to
- store that somewhere

## resolver class

^code resolver

- visits ast top to bottom (top down)
- only couple nodes interesting for vars
  - block
  - var decl
  - var expr
  - assign
- have to implement others to traverse full tree
- start with block

### resolving blocks

^code visit-block-stmt

- enters new scope for contents
- visits contents
- exits scope

- helper

^code resolve-statements

- uses

^code resolve-stmt

- interesting is scope

^code begin-scope

- scopes work like stack
- each scope is map of names to... we'll see

^code scopes-field (1 before, 2 after)

...

^code end-scope

- scopes stack tracks local scopes - blocks and fns
- globals not tracked
- more dynamic
- any var can't resolve locally assumed to be global

### var decl

^code visit-var-stmt

- store in scope, if any
- splits decl and defn?
- more precise about scope
- var a = a; ???
- want to be error, even if outer a
- declare var before initializer

^code declare

- puts in scope so shadows outer one
- but mark as "not ready yet", false in map
- after initializer, mark as totally ready

^code define

- between two, dead zone where var in scope but can't be used

### var expr

- have vars in scopes, can resolve now

^code visit-variable-expr

- first check for use in own initializer
- only do for locals
- var a = a; fine at top level, since globals dynamic

^code resolve-local

- like env, walk scopes innermost out looking for var
- diff only do once in single pass over code
- when find, tell interp resolve
- discuss that later

### fn decl

- other syntax that touches names
- both defines name for fn and binds params

^code visit-function-stmt

- decl and define fn name, like var
- define *before* resolve body
- ok to refer to own fn inside body since ref won't *execute* until after
  fn is fully defined
- enable recursion

^code resolve-function

- helper use for methods
- create scope for body where params bound
- define params
- resolve body

### assignment

^code visit-assign-expr

- resolve lhs
- recurse into value

^code resolve-expr

### traversing tree

- needs to impl all visit methods for stmts and exps, so got lot of code to
  get through
- most don't do anything except recurse into subtrees
- nothing to resolve in binary operator itself, but operands may contain var
  so need to visit them too

**todo: illustrate ast with var in leaf**

- do statements

^code visit-expression-stmt

- expression stmt has one expr node to recurse into

- then if stmt

^code visit-if-stmt

- can see here where different from interp
- resolves condition expr and then body
- also resolve else body if there is one
- always resolves both branches
- in static pass, need to touch all code that could be reached

- moving along

^code visit-print-stmt

- nothing interesting, just recurse into expr
- return is same

^code visit-return-stmt

- again, no control flow or exiting
- just traverse tree

- last boring statement

^code visit-while-stmt

- like if, always goes into body
- resolves it once

- that's easy stmts, now exprs

^code visit-binary-expr

- good old binary
- recurse into operands
- same for call

^code visit-call-expr

- walk arg list and resolve them all
- parens easy

^code visit-grouping-expr

- even easier is literal

^code visit-literal-expr

- no subexpr so no work to do at all
- logical just like binary

^code visit-logical-expr

- last operator ast

^code visit-unary-expr

## interp resolved vars

- what did resolver do?
- talk about dynamic envs
- could change entire way envs represented
- do for clox
- store res minimally invasive way
- original bug was var resolved at different number of hops away in envs
- fix that number
- scope chain mirrors env chain
- resolver notes how many scopes it had to look before finding var
- same # envs dynamically

^code resolve

- could store in var expr ast, but require changing ast, mess with generator
- store "on the side" in map that associates int with each expr

^code locals-field (1 before, 2 after)

- one monolithic map for all exprs in prog since each expr different java
  object with different identity
- no collisions

^code import-hash-map (1 before, 1 after)

...

^code import-map (1 before, 2 after)

### accessing resolved var

- replace body

^code call-look-up-variable (1 before, 1 after)

- helper

^code look-up-variable

- use previously resolved distance
- (if not resolved, assume global)
- get from env

^code get-at

- previous get walked chain looking for var at each step
- now know exactly where var is
- walk that many, return it
- don't even need to check if in map -- always will be
- [aside: interp and resolver tightly tied, interp assumes resolver does
  job right for correctness. be careful. can create subtle bugs if out of
  sync.]
- means not just correct, but faster too
- in clox, will resolve vars in way that is very fast to interp

### assigning resolved var

- assignment similar

^code resolved-assign

- similar env helper

^code assign-at

- other interp code fine
  - wanted to keep changes minimal
  - asts that modify env ok already

## running resolver

- need to insert resolution pass
- after parser, if no error, do resolution

^code create-resolver (3 before, 1 after)

- can resolve run correct code, but error?

## resolution errors

- get more precise not just which var, but correctness too
- consider

```lox
fun bad() {
  var a = "first";
  var a = "second";
}
```

- allow dupe at top for repl, but not local

^code duplicate-variable (1 before, 1 after)

- resolver can produce errors too

### bad returns

- more interesting:

```lox
return "at top level";
```

- return outside of fn bad
- like resolver tracks scopes, track whether in fn

^code function-type-field (1 before, 2 after)

- type

^code function-type

- could bool, but add more later

^code set-current-function

- enter fn, set
- store previous value, handle nested fns

^code restore-current-function

- restore when done
- stack of values using java stack
- [could do same for scopes, but need to walk all of them, would
  have to explicitly link]

- in return, check

^code return-from-top

- need to check for error

^code resolution-error

- also "syntax" or "compile" error
- check between both parser and resolver
- don't want to resolve code with parse errors -- too many cascades

- imagine doing other analysis
- track which fns don't access outer vars and use cheaper rep

---

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