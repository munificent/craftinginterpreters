Question: Smalltalk implementations doesn't have to check that the number of
arguments to a method matches the number of parameters it expects. Why not?

--

In function chapter, have challenge to extend it with lambda expressions.

--

Define "parameter" and "argument" early on.

--

When defining "parameter", also mention "formal parameter".

--

Put check for too many arguments outside of parsing loop so that the parser
doesn't get into an ugly state if there are more than 8 arguments/params.

--

Was in "Statements and State", maybe better here:

- fns are real core motivation for having scope
- fns have parameters -- variables that store values passed to fn
- if want to support recursion, may have multiple calls to same fn going on at
  same time
- each needs its own set of parameters
- so really need some kind of fn scope at minimum
- [early fortran did no support recursive fn calls. early machines had no
  concept of stack. without recursion, each fn's parameters could be statically
  allocated (think like "static" modified on variable in c) at compile time.
  was contentious when added to algol 60]
- much simpler, but really limiting
- imagine trying to write recursive descent parser without recursion
