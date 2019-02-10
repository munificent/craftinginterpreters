^title Local Variables
^part A Bytecode Virtual Machine

- this chapter, add blocks, block scope and local variables
- key goal is make creating and accessing as fast as possible

- globals late bound
- locals lexically scoped
- means can figure out what var ident refers to by looking at text
- in other words, compile time
- good because can use for perf

## Storing Local Variables

- if know at compile what locals exist, can use efficient rep at runtime to
  store

- globals live in hash table owned by vm
- flexible but slow

- local variables one of most used parts of language
- [and fn params, which work much like locals]
- faster they are, faster everything is
- want runtime representation that is fast to create new local, fast to access
  fast to assign, and fast to discard

- how do c and java manage locals?
- on the stack!
- can't easily use native stack for lox locals because lox is vm
- but vm itself has stack
- currently only using it for temporary storage when eval exprs
- also put locals there
- allocating space for new local is fast
  - pushing just incrementing stack top pointer
- if know stack slot that contains local, access and assign is fast
- discard fast, decrement pointer

- need to be careful
- vm stack already expects stack semantics
- can only discard local if on top of stack
- if local is on top of stack, can't discard anything below
- could be problem because also use stack for temps

- fortunately, lang handles
- [not surprise, designed lox for this]
- new local always created by statement
- since statements don't nest inside expr, won't be in the middle of evaluating
  expr when create new local
- so no temporaries on stack when local comes into being

- locals go out of scope at end of block
- again, won't happen in middle of expr
- so when locals added and removed, no temps on top of stack

- what about other locals?
- again, lang helps
- when scope ends, always innermost scope from innermost block
- can't have outer block end before inner
- all locals in same block go out of scope at same time
- so can push locals onto stack in order declared
- and when go out of scope, follows stack sem

**todo: illustrate examples**

- works out!
- bottom of stack will contain currently in-scope locals
- in order that decls executed
- when block ends, only locals on top of stack and pop all locals declared in
  block

- at compile time know exact slot where local lives in stack
- [when add fns, will change slightly. no longer absolute, but still at fixed
offset from well-defined base]
- so instructions can use slot as operand
- at runtime, just offset into stack, very fast

- any temporaries will float above locals on stack, don't interfere

## Tracking Local Variables

- to do all this, do need to keep track of locals at compile time

- need some state during compilation
- to resolve ident, need to know what locals in scope
- jlox used chain of envs, each map
- clox use lower level representation

^code compiler-struct (1 before, 2 after)

- flat array of all locals in scope, in order that declaration appears
- use fixed-size array
- op code for local uses only byte operand, so max 255 locals

^code uint8-count (1 before, 2 after)

- track number of slots in array in use
- also track current scope nesting depth
- jlox used chain of maps
- clox just numbers each scope
- 0 is glocal scope
- 1 is block, 2 block inside that, etc.

- local is array of

^code local-struct (1 before, 2 after)

- store name of var
- what use to resolve identifier expr against
- scope depth where declared
- [since only use this for locals, never have var at depth zero here]
- use for a few different things

- rep very different
- store essentially same state
- all local vars currently in scope, and where declared

- various functions in compiler need
- would be cleaner to pass around
- [maybe want to run in separate thread]
- to minimize code change, make global

^code current-compiler (2 before, 2 after)

- initialize

^code init-compiler

- when begin compiling, set up

^code compiler (1 before, 2 after)

- few operations modify state
- begin new scope
- end scope
- declare new variable in current scope
- get to them as add each lang feature

## Block Statements

- before can have local vars, need local scopes
- come from two things: fns and blocks
- fns big chunk of work, get to in next chapter
- for now just blocks
- start with syntax

^code parse-block (2 before, 1 after)

- block is statement starts with `{`
- call out to helper to parse contents

^code block

- [reuse for fn bodies later]
- keeps parsing declarations until hit end of block
- need to check for eof too so don't get in infinite loop on bad code

- scope fns are fun

^code begin-scope

- to "create" scope just bump current depth
- only at compile time, so not perf sensitive, but already much faster than
  jlox
- if scope never gets any vars, no reason to alloc data struct or anything

- can guess what end scope does

^code end-scope

- now have scopes, start putting vars in

## Declaring Local Variables

- compiler already supports syntax and semantics for declaring vars
- just only does globals
- don't need new parsing support, just need to hook up new semantics to
  existing code

- `varDeclaration()` is main parse fn for var
- calls

^code parse-local (1 before, 1 after)

- first declare var
- then, if in local block scope, treat differently
- since local not looked up by name at runtime, don't need ident const
- just return 0
- will fix code that's passed to soon
- declare is

^code declare-variable

- only for locals, so bails if in top level scope
- [could hoist inside if check in parselocal but will call this elsewhere later]
- calls

^code add-local

- appends new local to end of list
- stores name and marks as being in current scope

- good for correct code, but what about invalid code?
- need error handling
- first is not user's fault, but limitation of vm
- only support max number of locals because of operand
- compiler needs to ensure doesn't overflow

^code too-many-locals (1 before, 2 after)

- more interesting

```lox
{
  var a = "first";
  var a = "second";
}
```

- at top level, allow redeclaring var with same name because useful for repl
- but in local scope, pretty weird
- probably an error
- most langs disallow
- [rust does not. useful for ownership model]
- compiler can report error for this

- note different from:

```lox
{
  var a = "outer";
  {
    var a = "inner";
  }
}
```

- ok to have vars with same name in different scopes, even when overlap
- shadowing
- only error to have same name in same scope
- detect error with

^code existing-in-scope (1 before, 2 after)

- locals appended to array when declared
- current scope always at end of array
- start at end and work backwards
- look for local with same name as one declaring
- if reach beginning of current scope then can stop

- to see if two idents same

^code identifiers-equal

- do length check first because fast and catches many
- otherwise compare chars
- can't check hashes because tokens aren't lox strings
- to use memcmp, need

^code compiler-include-string (1 before, 2 after)

- can now bring vars into being
- also need to eliminate when they go out of scope

^code pop-locals (1 before, 1 after)

- whenever pop scope, walk through local array looking for locals in current
  scope being exited
- discard each one by decrementing count
- also emit code to discard var at runtime
- since local lives on stack can use op_pop from prev chapter to remove

## Using Locals

- have local variables, now need instrs to get and set
- do both at same time since touch same code

- already have code for accessing and assigning global vars
- want to reuse what we can

^code named-local (1 before, 2 after)

- instead of hardcoded instrs for get and set, use var
- try to find local with given name
- if found, use instructions for local
- otherwise, assume global and use existing instrs

- then emit instrs as before
- set

^code emit-set (2 before, 1 after)

- get

^code emit-get (2 before, 1 after)

- to tell if var is local, use

^code resolve-local

- pretty straightforward
- walk list of locals currently in scope
- if any have same name as ident token, must refer to that var
- return index of local in array
- since locals appended to array, first local is at index zero
- increase from there
- at runtime, locals on stack
- first local at bottom at slot zero
- so local array index directly correspond to stack slot
- return that index

- if didn't find, must be global
- use `-1` to signal that
- [or an error. detect at runtime]

- note walk array backwards
- important
- consider prev ex

```lox
{
  var a = "outer";
  {
    var a = "inner";

    print a;
  }
}
```

- two locals in array whose name is `a`
- when compile access of `a` in print stmt, look for it
- for inner to shadow, need to find it first
- searching backwards finds innermost var first

- all we need to do to compile
- two new instrs

^code get-local-op (1 before, 1 after)

- in interp

^code interpret-get-local (1 before, 2 after)

- takes byte operand for stack slot where local stored
- reads value from slot and pushes onto stack as temporary

- also have set

^code set-local-op (1 before, 1 after)

- similar

^code interpret-set-local (1 before, 2 after)

- take value from top of stack and store in stack slot for local
- note don't pop
- assign is expr, so need to leave val on stack in case surrounding expr uses
- [in common case where assign appears in expr stmt, compiler emits pop]

- dis new instrs

^code disassemble-local (1 before, 1 after)

- compiler compiles locals to direct slot access
- name of local never goes into chunk at all
- good for efficiency and perf
- bad for debugging
- [real problem if want to impl debugger for vm
  need to output additional debugging info that tracks name of each local
  in each slot
  can change over time since slot may be reused when one block ends and new
  one begins]

- when dis, can't show name
- instead just show slot

^code byte-instruction

## Another Scope Edge Case

- already spent time handling weird cases around scope
- make sure shadowing works right
- report error if two vars in same scope with same name
- scoping has surprisingly many of these issues
- still one more

```lox
{
  var a = "outer";
  {
    var a = a;
  }
}
```

- i know right? what was author even thinking writing this?
- but, really, what *were* they thinking?
- here, probably meant to access shadowed var and store same value in inner
- but what about:

```lox
{
  var a = a;
}
```

- when, precisely, does var become in scope?
- is `a` in scope in its own initializer expression?
- definitely not useful
- `a` doesn't have value yet... since haven't finished executing initializer
- probably just want to be error
- need to detect
- [similar to local fn decl. but there do want to be in scope so can call
  self recursively
  safe because can't execute fn until after decl finishes]

- one solution is to not declare var until after compile initializer
- would cause first shadow example to access outer var
- don't like
- think user intuition is that code mostly left to right
- as soon as "past" var name, should be in scope
- almost never see real code like ex anyway

- instead, make more explicit prohibit
- as soon as begin var decl, name becomes in scope
- but put in special "uninit" state
- if access var in own initializer, compile error
- after initializer, var then becomes fully usable

- so when declare local, need to indicate "uninit" state
- do by setting depth to sentinel value

^code declare-undefined (1 before, 2 after)

- later, when var defined after initializer compiled replace with real depth

^code define-local (1 before, 2 after)

- when resolving use of local, see if usable

^code own-initializer-error (1 before, 1 after)

- after find var, if depth has sentinel value, must be compiling use inside
  init
- report error

- that it
- added blocks, block scope, real lexical scoping
- not too much code for what did
- not just semantics, but also very clean, efficient impl
- note almost all new code in compiler, very little in runtime
- consistent theme for how to get good perf
- more do in compiler, the better
- [naturally extends to static types where can do all checking for type errors
  in compiler]

challenges

- use hash table instead of linear array of locals

- how do other langs handle shadowing and accessing var in own initializer?
  what would you do?

- allow "let" expression that declares var in middle of expr
how do deal with temporaries
