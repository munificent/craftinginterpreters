^title A Bytecode Virtual Machine

Our Java interpreter, jlox, covers a lot of territory, but it's not perfect.
First, if you run any interesting Lox programs on it, you'll discover it's
achingly slow. The style of interpretation it uses -- walking the AST directly
-- is good enough for *some* real-world uses, but leaves a lot to be desired for
a general-purpose scripting language.

Also, we implicitly rely on some fundamental stuff from the JVM itself. We take
for granted that things like `instanceof` in Java work *somehow*. And we never
for a second worry about memory management because the JVM's garbage collector
takes care of it for us.

Those were useful training wheels while we were getting using to the basic
concepts in interpreters. But now that we have those concepts down, it's time
for us to take those wheels off and build our own virtual machine from scratch
using nothing more than the C standard library...
