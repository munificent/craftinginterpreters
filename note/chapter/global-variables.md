--

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
