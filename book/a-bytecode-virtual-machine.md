Our Java interpreter, jlox, taught us many of the fundamentals of programming
languages, but we still have much to learn. First, if you run any interesting
Lox programs in jlox, you'll discover it's achingly slow. The style of
interpretation it uses -- walking the AST directly -- is good enough for *some*
real-world uses, but leaves a lot to be desired for a general-purpose scripting
language.

Also, we implicitly rely on runtime features of the JVM itself. We take for
granted that things like `instanceof` in Java work *somehow*. And we never for a
second worry about memory management because the JVM's garbage collector takes
care of it for us.

When we were focused on high-level concepts, it was fine to gloss over those.
But now that we know our way around an interpreter, it's time to dig down to
those lower layers and build our own virtual machine from scratch using nothing
more than the C standard library...
