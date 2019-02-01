^title Local Variables
^part A Bytecode Virtual Machine

## snippets

### compiler

...

^code compiler-struct (1 before, 2 after)

...

**todo: move comments into prose.**

^code local-struct (1 before, 2 after)

...

^code uint8-count (1 before, 2 after)

...

^code current-compiler (2 before, 2 after)

...

^code init-compiler

...

^code compiler (1 before, 2 after)

### block syntax

...

^code parse-block (2 before, 1 after)

...

^code block

...

^code begin-scope

...

^code end-scope

### declare locals

...

^code parse-local (1 before, 1 after)

...

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
