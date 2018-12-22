^title Global Variables
^part A Bytecode Virtual Machine

---

## unsorted

^code pop-op (1 before, 1 after)

...

^code ops (1 before, 1 after)

...

^code op-print (1 before, 1 after)

...

^code parse-fn-type (1 before, 1 after)

...

^code check (1 before, 1 after)

...

^code match (1 before, 1 after)

...

^code forward-declarations (1 before, 1 after)

...

^code parse-variable (1 before, 1 after)

...

^code define-variable (1 before, 1 after)

...

^code binary (1 before, 1 after)

...

^code parse-literal (1 before, 1 after)

...

^code grouping (1 before, 1 after)

...

^code number (1 before, 1 after)

...

^code string (1 before, 1 after)

...

^code unary (1 before, 1 after)

...

^code named-variable (1 before, 1 after)

...

^code variable (1 before, 1 after)

...

^code table-identifier (1 before, 1 after)

...

^code prefix-rule (1 before, 1 after)

...

^code infix-rule (1 before, 1 after)

...

^code invalid-assign (1 before, 1 after)

...

^code var-declaration (1 before, 1 after)

...

^code expression-statement (1 before, 1 after)

...

^code print-statement (1 before, 1 after)

...

^code synchronize (1 before, 1 after)

...

^code declaration (1 before, 1 after)

...

^code statement (1 before, 1 after)

...

^code compile (1 before, 1 after)

...

^code disassemble-pop (1 before, 1 after)

...

^code disassemble-global (1 before, 1 after)

...

^code disassemble-print (1 before, 1 after)

...

^code vm-globals (1 before, 1 after)

...

^code init-globals (1 before, 1 after)

...

^code free-globals (1 before, 1 after)

...

^code read-string (1 before, 1 after)

...

^code interpret-pop (1 before, 1 after)

...

^code interpret-get-global (1 before, 1 after)

...

^code interpret-define-global (1 before, 1 after)

...

^code interpret-set-global (1 before, 1 after)

...

^code interpret-print (1 before, 1 after)

...

^code op-return (1 before, 1 after)

...

^code undef-read-string (1 before, 1 after)

...

^code identifier-constant (1 before, 1 after)

...

---

In "Global Variables", talk about how checking for undefined is needed, but
very rare in practice. There are alternatives to checking it on every access:

- When running a script (instead of the REPL), it's possible to make it a
  static error. Whether or not it should be (you may never call the function
  containing the undefined access) is an open question.
- After a lookup succeeds once, you could rewrite the bytecode to a different
  instruction that skips the check. Once a global is defined, it is defined
  forever. This only optimizes a single look-up, though. You have to optimize
  each callsite individually.
- Maybe you could globally rewrite all accesses to a variable after it is
  defined the first time. This is slow (it's basically a full trace), but since
  definitions are rare, it might be feasible. In fact, in this model, any
  unrewritten access instruction would be an unconditional failure -- it would
  only execute if the global wasn't defined.

Maybe make some of these options exercises.

See chapter 5 of Lisp in Small Pieces for some discussion.
