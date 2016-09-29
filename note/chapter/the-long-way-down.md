When we first talk about compiling Lox, explain that "compiling" doesn't always
mean "statically typed". See:

http://akaptur.com/blog/2013/12/03/introduction-to-the-python-interpreter-4/

--

One way to look at bytecode is a really dense serialization of the AST.

--

One way to look at compilation is that it separates out the behavior of a
program into two halves. The static stuff is the same no matter how the code
executes. The dynamic stuff is only known at runtime. A compiler then does all
the static work once, before the code is run.

See chapter 6 of Lisp in Small Pieces.
