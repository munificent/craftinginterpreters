^title A Tree-Walk Interpreter in Java

With this part, we begin the first of our two interpreters for Lox, jlox. The
world of programming languages a big one to try to cram into your brain all at
once. There are lists of new terminology to learn and the concepts behind them.
There's theory -- though not too much in this book -- to get acquainted with.
Much of it requires a level of precision and rigor in our thinking that you
probably haven't had to summon since your last Calculus final.

Interpreters use a few architectural tricks and design patterns not that common
in other kinds of applications, so we'll be getting used to those too. Since all
of that is a long enough shopping list, we'll keep the code itself as simple and
plain as possible.

In less than two thousand lines of clean Java code, we'll build up a complete
interpreter for Lox that implements every single feature of the language,
exactly as we've specified. The first few chapters work front-to-back through
the phases of the interpreter -- [scanning][], [parsing][], and [evaluating
code][]. After that, we'll add language features one at a time, incrementally
working our way from a simple calculator to a complete scripting language.

[scanning]: scanning.html
[parsing]: parsing-expressions.html
[evaluating code]: evaluating-expressions.html
