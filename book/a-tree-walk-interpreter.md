^title A Tree-Walk Interpreter

With this part, we begin jlox, the first of our two interpreters for Lox.
Programming languages a huge topic with piles of concepts and terminology to
cram into your brain all at once. There's theory -- though not too much in this
book -- which requires a level of mental rigor that you probably haven't had to
summon since your last Calculus final.

Interpreters use a few architectural tricks and design patterns not that common
in other kinds of applications, so we'll be getting used to the engineering side
of things too. Given all of that, we'll keep the code we have to write as simple
and plain as possible.

In less than two thousand lines of clean Java code, we'll build a complete
interpreter for Lox that implements every single feature of the language,
exactly as we've specified. The first few chapters work front-to-back through
the phases of the interpreter -- [scanning][], [parsing][], and [evaluating
code][]. After that, we add language features one at a time, growing a simple
calculator into a full-fledged scripting language.

[scanning]: scanning.html
[parsing]: parsing-expressions.html
[evaluating code]: evaluating-expressions.html
