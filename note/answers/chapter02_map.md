## 1. Find the various parts in an open source implementation.

TODO

## 2. Why not use a JIT?

1. It's really complex to implement, debug, and maintain. Few people have the
   skill to do it.
2. Like a native code compiler (which it is), it ties you to a specific CPU
   architecture.
3. Bytecode is generally more compact than machine code (since it's closer to
   the semantics of the language), so it takes up less memory. In platforms
   like embedded devices where memory may matter more than speed, that can be
   a worthwhile trade-off.
4. Some platforms, like iOS and most game consoles, expressly disallow
   executing code generated at runtime. The OS simply won't allow you to jump
   into memory that can be written to.

## 3. Why do Lisp compilers also contain an interpreter?

Most Lisps support macros -- code that is executed at compile time, so the
implementation needs to be able to evaluate the macro itself while in the middle
of compiling. You could do that by *compiling* the macro and then running that,
but that's a lot of overhead.
