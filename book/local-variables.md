^title Local Variables
^part A Bytecode Virtual Machine

- this chapter, add blocks, block scope and local variables
- key goal is make creating and accessing as fast as possible

- globals late bound
- locals lexically scoped
- means can figure out what var ident refers to by looking at text
- in other words, compile time
- good because can use for perf

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

## Blocks

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

- in there, if in local block scope, treat differently
- since local not looked up by name at runtime, don't need ident const
- just return 0
- will fix code that's passed to soon
- instead of existing global var code, call

^code declare-variable

...

^code identifiers-equal

...

^code compiler-include-string (1 before, 2 after)

...

^code add-local

**todo: add -1 for declared-not-yet-defined in separate step?**

**note: don't need to do anything in defineVariable() since already on stack.**

...

^code pop-locals (1 before, 1 after)

...

### get and set local

...

^code named-local (1 before, 2 after)

...

^code resolve-local

...

^code emit-get (1 before, 1 after)

...

^code emit-set (2 before, 1 after)

...

^code get-local-op (1 before, 1 after)

...

^code interpret-get-local (1 before, 2 after)

...

^code set-local-op (1 before, 1 after)

...

^code interpret-set-local (1 before, 2 after)

**note: does not pop because assign is expr, not stmt**

### use in initializer

...

^code declare-undefined (1 before, 2 after)

...

^code define-local (1 before, 2 after)

...

^code own-initializer-error (1 before, 1 after)

...

### disassemble

...

^code disassemble-local (1 before, 1 after)

...

^code byte-instruction

---

notes:

Talk about what the stack frame would look like if we allowed variables to be
declared in the middle of an expression.

storing compiler structs on stack means deep nesting could be problem. deep
nesting also problem with recursive descent in general. may be good to set
max depth and check on recursive calls.

lot of work in compiler, only two new instructions. this is goal: any work do
in compiler only done once, code in runtime executes every time.

no parser changes (aside from block) because grammar already there. just that
existing "var" stmt, ident expr, and assign expr mean different things when
referring to local.
