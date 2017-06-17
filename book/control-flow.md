^title Control Flow
^part A Tree-Walk Interpreter

> Logic, like whiskey, loses its beneficial effect when taken in too large
> quantities.
>
> <cite>Edward John Moreton Drax Plunkett, Lord Dunsany</cite>

Compared to the last chapter's grueling amarathon, today's journey is a
lighthearted frolic through a daisy pasture. But while the work we'll be doing
is short and light -- my favorite kind of work -- the reward is surprisingly
large.

Up to this point, our interpreter is still little more than a calculator. A program can only do a fixed amount of work before completing. To make a program
run twice as long we have to make the source code twice as big.

After this chapter, our little interpreter will take a big step towards the programming language major leagues: Turing-completeness.

## Turing Machines (Briefly)

In the early part of last century, mathematicians worked hard to pin down and formalize the lowest foundations that math is built on. Starting from a minimal set of axioms, logic, and set theory, they hoped to rebuild mathematics on top of an impenetrable foundation.

They wanted to rigorously answer questions like, "Can all true statements be
proved?", "Are there functions that we can define but not [compute][]?", or even
the more general question, "What do we mean when we claim a function is
'computable'?".

[compute]: https://en.wikipedia.org/wiki/Computable_function

For the last question, the definition they settled on was:

1.  The description of the procedure to compute the function must be finite
    length. It's pretty hard to publish a paper containing a function that's
    infinitely long. The journal editors tend to frown on that.

2.  Executing the procedure must take only a finite amount of time. It can take
    a *really long* time -- we're talking heat-death-of-the-universe long -- but
    it does have to reach a stopping point eventually. Mathematicians are
    patient, but not immortal. (If you give the function an input where the
    function doesn't define a result value, it's allowed for the procedure to
    not complete.)

3.  Each step of the procedure must a simple mechanical operation that a
    reasonable person could perform without any special leap of intuition.
    Today, we'd say it had to be something you could easily program a computer
    to do.

    This is a little hand-wavey, but mathematicians addressed that by inventing
    "machines" or models that could do computation and defined each of the
    primitive operations they supported.

They hoped the answer to the first two questions would be "yes". All that remained was to prove it. It turns out that the answer to both is "no" and, in fact, the two questions are deeply intertwined. This is a really fascinating corner of both programming and mathematics that reaches outwards to touch fundamental questions what our brains are able to do and how the universe works. I can't do it justice here.

What I do want to note is that in the process of proving that those questions are false, Alan Turing and Alonzo Church devised a precise definition of what kinds of functions are computable. They each crafted a tiny system with a minimum set of machinery that is still powerful enough to compute <span name="uncomputable">any</span> of a (very) large class of functions.

<aside name="uncomputable">

They proved the answer to the first question is "no" by showing that the function to compute the truth value of a given proof is *not* in the class of computable functions.

</aside>

These are now considered the "computable functions". Turing's system is now called a <span name="turing">"Turing machine"</span>. Church's is called the lambda calculus. Both are still widely used as the basis for models of computation and, in fact, many modern functional programming languages use the lambda calculus at their core.

<aside name="turing">

Turing called his inventions "a-machines" for "automatic". He wasn't so self-aggrandizing to put his own name on them. Later mathematicians did that for him. That's how you get famous while still retaining some modesty.

</aside>

**todo: illustrate turing machine**

Turing machines have a little better name recognition (there's no dramatized Hollywood film about Alonzo Church), but the two formalism are [equivalent in power][thesis]. In fact, any programming language with some minimal level of expressive is powerful enough compute *any* computable function.

[thesis]: https://en.wikipedia.org/wiki/Church%E2%80%93Turing_thesis

One way to prove that is by writing a simulator for a Turing machine in your
language. Since Turing proved his machine can compute any computable function,
that by induction means your language can too. All you need to do is translate
the function into a Turing machine, and then run that on your simulator.

If your language is expressive enough to do that, it's considered
**Turing-complete**. Turing machines are pretty dang simple, so it doesn't take
much power to do this. You basically need arithmetic, some basic flow control,
and the ability to allocate and use (theoretically) arbitrary amounts of memory.
We've got the first. By the end of this chapter, we'll have the <span
name="memory">second</span>.

<aside name="memory">

We *almost* have the third already: You can create and concatenate strings of
arbitrary size, so you can *store* unbounded memory. But we don't have any
syntax for splitting up or accessing parts of strings.

</aside>

## Conditional Execution

Enough history, let's jazz up our language. We can divide control flow roughly into <span name="zero">two</span> kinds:

*   **Conditional** or **branching** control flow is used to *not* to execute
    some piece of code. Imperatively, you can think of it as jumping *ahead*
    over a region of code.

*   **Looping** control flow is used to execute a chunk of code more than once.
    It's jumping *back* so that you can do something again.

**todo: illustrate jumping over and back**

<aside name="zero">

Another way to look at it is that all control flow is about choosing how many
times to execute some code, and where conditionals just mean executing it *zero*
times.

</aside>

The first is the simpler of the two, so we'll start there. C-derived languages have two main conditional execution features, the if statement and the perspicaciously named "conditional" or "ternary" operator (`? :`). If statements let you conditionally execute statements and the conditional operator lets you conditionally execute statements.

For simplicity's sake, Lox doesn't have a conditional operator, so we'll do if statements. That means a new production under the statement rule in our grammar:

```lox
statement   = exprStmt
            | ifStmt
            | printStmt
            | block ;

ifStmt      = "if" "(" expression ")" statement ( "else" statement )? ;
```

An if statement has an expression for the condition that determines what code to execute, then a statement to execute if the condition is truthy. Optionally, it may also have an `else` keyword and a statement to execute if the condition is falsey. Of course, if you want to execute more than one statement in either of those cases, you can stuff a block in there.

The syntax tree node has fields for each of those three pieces:

^code if-ast (1 before, 1 after)

Like other statements, the parser recognizes an if statement by the leading `if` keyword:

^code match-if (1 before, 1 after)

When it finds one, it calls this to parse the rest of the if statement:

^code if-statement

<aside name="parens">

The parentheses around the condition are only half useful. You need some kind of delimiter *between* the condition and the then statement, otherwise the parser can't tell when it's reached the end of the condition expression. But the opening parenthesis after `if` doesn't do anything useful. Dennis Ritchie put it there so he could use `)` as the ending delimiter without having unbalanced parentheses.

Other languages like Lua and some BASICs use a keyword like `then` as the ending delimiter and don't have anything before the condition. Go and Swift instead require the body to be a braced block. That lets them use the `{` at the beginning of the body to tell when the condition is done. Since most of the time you do want a block there anyway, it's more terse in those cases.

</aside>

As usual, the parsing code hews closely to the grammar. It detects an else clause by looking for the preceding `else` keyword. If there isn't one, the else field in the syntax tree is `null`.

That seemingly innocuous piece of optional syntax has in fact opened up an ambiguity in our grammar. Consider:

```lox
if (first) if (second) whenTrue(); else whenFalse();
```

Here's the riddle: Which if statement does that else clause belong to? This isn't just a theoretical question about how we notate our grammar? It affects how the code executes.

*   If we attach it to the first if statement, then `whenFalse()` is called if `first` is falsey, regardless of what value `second` has.

*   If we attach it to the second if statement, then `whenFalse()` is only called if both `first` and `second` are falsey.

Since else clauses are optional, and there is no explicit delimiter marking the end of the if statement, the grammar ambiguous when you nest ifs in this way. This classic pitfall of syntax is called the "[dangling else][]" problem.

**todo: illustrate?**

[dangling else]: https://en.wikipedia.org/wiki/Dangling_else

It is possible to define a context-free grammar that's unambiguous, but it would require splitting most of the statement rules into pairs, one that allows an if with an else and one that doesn't. It's hairy. Instead, most languages and parsers avoid the problem in an ad hoc way.

Whatever hack they use to get themselves out of the trouble, they almost always choose the same interpretation -- an else is bound to the nearest if that precedes. That's the second bullet point there.

Our parser conveniently does that already. Since it eagerly looks for an `else` before returning from parsing the if, the innermost call to a nested series of `ifStatement()` calls will claim the else clause for itself before returning to the outer if statements.

Syntax in hand, we are ready to interpret:

^code visit-if

The code is barely more than a thin wrapper around the self-same Java code. It evaluates the condition. If it's truthy, it executes the then branch. Otherwise, if there is an else branch, it executes that.

If you compare this code to how the interpreter handles other syntax we've implemented, the part that makes control flow special is visible in that Java `if`. Function calls and most other operators and statements always evaluate their subexpressions. Here, we don't evaluate the then or else statement. If either of those has a side effect, the choice not to evaluate it becomes user-visible.

The fact that we used Java's `if` to implement Lox's is no coincidence. Being
able to conditionally execute code is a fundamental primitive of our language.
We can't <span name="closure">emulate</span> it in terms of the basic operators
and statements we have already.

<aside name="closure">

You *can* implement condition execution using either higher-order functions --
functions that take and return functions as arguments -- or dynamic dispatch.
The lambda calculus does the latter, and Smalltalk does the former.

But we don't have either of those yet, so for now, at least, conditional
execution must be a real primitive in the interpreter.

</aside>

## Logical Operators

Since we don't have the conditional operator, you might think we're done with branching, but no. Even without the ternary operator, there are two binary operators in C that are technically control flow constructs -- the logical operators and (`&&`) and or (`||`).

These aren't like other binary operators because they **short-circuit**. If, after evaluating the left operand, we know what the result of the expression is going to be, we don't evaluate the right operand at all. For example:

```lox
false && sideEffect();
```

For an and expression to evaluate to something truthy, both operands must be truthy. We can see as soon as we evaluate the left `false` operand that that isn't going to be the case, so there's no reason to worry about the call to `sideEffect()` and it gets skipped.

This is why we didn't add the logical operators when we did all of the other binary operators. We're going to add them now. The logical operators are pretty low in the precedence table. Only assignment is lower. As in C, they each have their <span name="logical">own</span> precedence with `||` lower than `&&`. We slot them right between `assignment` and `equality`:


<aside name="logical">

I've always wondered why they don't have the *same* precedence like the various
comparison or equality operators do.

</aside>

```lox
expression  = assignment ;
assignment  = identifier "=" assignment
            | or ;
or          = and ( "or" and )* ;
and         = equality ( "and" equality )* ;
```

Instead of falling back to `equality`, `assignment` now cascades to `or`.
The two new rules for `or` and `and` are structurally the <span name="binary">same</span> as the other binary operators. Then `and` calls out to `equality` for its operands, and we escape back to the rest of the expression rules.

<aside name="same">

The *grammar* doesn't care that they short-circuit. That's a semantic concern.

</aside>

We could reuse the existing Expr.Binary class for these two new expressions since they have the same fields. But then the visit method for the class would have to check to see if the operator is one of the logical operators and use a different code path to handle the short circuiting.

I think it's cleaner to use a distinct class for these operators so that they get their own visit method. Like so:

^code logical-ast (1 before, 1 after)

To weave the new expressions into the parser, we first change the parsing code for assignment to call `or()` instead of the old `equality()`:

^code or-in-assignment (1 before, 2 after)

The code for parsing a series of `or` expressions is the same as the parsing code for other binary operators:

^code or

Its operands are the next higher level of precedence, the new and expression:

^code and

That calls `equality()` for its operands and now it's all tied back together again. We're ready to interpret it:

^code visit-logical

If you compare this to the earlier chapter's `visitBinary()` method, you can see the different structure. Here, we evaluate the left operand first. Then we look at its value and operator to see if we can short circuit and return. Otherwise, and only then, do we evaluate the right operand.

The other interesting piece here is deciding what actual value to return. Since Lox is dynamically typed, we allow operands of any type and use their truthiness to determine what they mean in the context of the logic operation. We apply similar reasoning to the result. Instead of promising to literally return `true` or `false`, it merely guarantees it will return a value with appropriate truthiness.

Fortunately, we have values with right truthiness right at hand -- the results of the operands itself. So we use those. For example:

```lox
print "hi" or 2; // "hi".
print nil and "nope"; // nil.
```

On the first line, `"hi"` is truthy, so the `or` short-circuits and we return
that value. On the second line, `nil` is falsey, so we evaluate and return the
second operand, `"nope"`.

That covers all of the branching primitives in Lox. We're ready to jump ahead to loops. You see what I did there? *Jump. Ahead.* Get it? See, it's like a reference to... oh, forget it.

## While Loops

Lox features two looping control flow statements, while and for. While is the simpler one so we'll start there. Its grammar is the same as in C:

```lox
statement   = exprStmt
            | ifStmt
            | printStmt
            | whileStmt
            | block ;

whileStmt   = "while" "(" expression ")" statement ;
```

We add another clause to the statement rule that points to the new rule for
while. It takes a `while` keyword, followed by a parenthesized condition expression, then a statement for the <span name="semicolon">body</span>.

<aside name="semicolon">

Note that the semicolon there is part of the grammar metasyntax, not the while statement itself. That's why it isn't quoted. If the body of the while is a block, there won't be a semicolon after it.

</aside>

Our new grammar rule leads to a new syntax tree node:

^code while-ast (1 before, 1 after)

It stores the condition and body. Here you can see why it's nice to have separate base classes for expressions and statements. If you look at the declaration of this, it's explicit that the condition is an expression and the body is a statement.

Over in the parser, we follow the same process we did for if. First, we add another case in `statement()` to detect and match the leading keyword:

^code match-while (1 before, 1 after)

That delegates the real work to:

^code while-statement

The grammar is dead simple and this is a straight translation of it to Java. Speaking of translating straight to Java, here's how we interpret this new piece of syntax:

^code visit-while

Like the visit method for if, it uses the corresponding Java syntax. This isn't very complex, but we have made Lox much more powerful now. We can finally write programs whose running time isn't strictly bound by the length of the source code!

## For Loops

We're down to the last flow control construct, <span name="for">Ye Olde</span> C-style for loop. I probably don't need to remind you, but it looks like this:

<aside name="for">

Most modern languages have a higher-level looping statement for iterating over arbitrary user-defined sequences. C# has `foreach`, Java has "enhanced for", even C++ has range-based for statements now. Those offer cleaner syntax than C's for statement by implicitly calling out to some iteration protocol that the object being looped over is expected to support.

Those are great. For Lox, though, we're limited by building up the interpreter a chapter at a time. We don't have objects and methods yet, so we have no way of defining an iteration protocol that the for loop could call into. So we'll stick with the old school C for loop. Think of it as "vintage". The fixie of flow control statements.

</aside>

```lox
for (var i = 0; i < 10; i = i + 1) print i;
```

It has four components. Inside the parentheses, you have three clauses separated by semicolons:

1. The first clause is the *initializer*. It is executed exactly once, before anything else. It's usually an expression, but for convenience, we also allow a variable declaration there. In that case, the variable is scoped to the rest of the for loop (the other two clauses an the body). No other statements are allowed here.

2. Next is the *condition*. As in a while loop, this expression is used to tell when to exit the loop. It's evaluated once at the beginning of each iteration, including the first. If the result is truthy, it enters the body of the loop. Otherwise, it bails.

3. The last clause is the *increment*. It's an arbitrary expression that does some work at the end of each loop iteration. The result of the expression is discarded, so it must be some kind of expression with a side effect to be useful. In practice, it's usually incrementing and assigning a variable.

Any of these clauses can be omitted. After that, you have a statement for the body.

### Desugaring

That's quite a lot of machinery, but note that none of it doesn't anything you
can't already do with the statements we've already introduced. If for loops
didn't support initializer clauses, you could just put the initializer
expression before the for statement. If it didn't have an increment clause, you
could simply put the increment expression at the end of the body yourself.

In other words, Lox doesn't *need* for loops -- anything you can do with them
you could do without them -- they just make some common code patterns more
pleasant to write. These kinds of features are called <span
name="sugar">**syntactic sugar**</span>.

<aside name="sugar">

This delighful turn of phrase was coined by Peter J. Landin in 1964 to describe
how some of the nice expression forms supported by languages like ALGOL were a
sugaring over the more fundamental, yet presumably less palatable lambda
calculus underneath.

![Slightly more than a spoonful of sugar.](image/control-flow/sugar.png)

</aside>

For example, the previous for loop could be rewritten like so:

```lox
{
  var i = 0;
  while (i < 10) {
    print i;
    i = i + 1;
  }
}
```

That has the exact same semantics, though it's certainly not as easy on the
eyes. We'll use the fact that this is syntactic sugar to our advantage by <span
name="caramel">**desugaring**</span> it. Instead of adding direct support for
for loops in the interpreter, the parser will take the new syntax and translate
it to more primitive forms that the interpreter already knows how to execute.

<aside name="caramel">

Oh, how I wish the accepted term for this were "caramelization" instead of
desugaring. I mean, why introduce a metaphor if you aren't going to stick with
it?

</aside>

It's not exactly saving us a ton of work in this tiny interpreter, but desugaring is a really powerful tool to have in your toolbox and this gives me an excuse to show it to you. In a sophisticated implementation, the backend does lots of work optimizing each supported chunk of semantics. The fewer of those there are, the more mileage it gets out of each optimization.
 Meanwhile, a rich syntax makes the language more pleasant and productive to work in. Desugaring bridges that gap -- it lets you take a rich expressive syntax and cook it down to a small number of primitives that the back end can do its magic on.

So, unlike the previous statements, we *won't* add a new syntax tree node. Instead, we go straight to parsing. First, let's add an import we'll need in a bit:

^code import-arrays (1 before, 1 after)

Like every statement, we start parsing a for loop by matching its keyword:

^code match-for (1 before, 1 after)

Here is where it's going to get interesting:

^code for-statement

The desugaring is going to happen here, so we'll build this method a piece at a time, starting with the opening parenthesis before the clauses. The first clause following that is the initializer:

^code for-initializer (2 before, 1 after)

If the next token is a semicolon, then the initializer has been omitted and we leave it null. Otherwise, we look for a `var` keyword to handle the <span name="variable">special</span> case support for declaring a variable here. Otherwise, we parse an initializer expression and wrap it in an expression statement so that the initializer is always of type Stmt.

<aside name="variable">

In a previous chapter, I said we can split expression and statement syntax trees into two separate class hierarchies because there's no single place in the grammar that allows either an expression or a statement. That wasn't *entirely* true, I guess.

</aside>

Next up is the condition:

^code for-condition (2 before, 1 after)

Again, we look for a semicolon to see if the clause has been omitted. The last clause is the increment:

^code for-increment (1 before, 1 after)

It's a little different because now the ending delimiter the closing parenthesis. All that remains is the <span name="body">body</span>:

<aside name="body">

Is it just me or does that sound morbid? "All that remained... was the body."

</aside>

^code for-body (1 before, 1 after)

It parses the statement and returns it. OK, obviously we're missing something. We parsed all of the clauses and then dropped them on the floor. Now it's time to get desugaring.

We're going to take all of those clauses and the body and synthesize syntax tree nodes that express the same semantics as the for loop, like the hand-desugared example I showed you earlier. The code is a little simpler if we work backwards, so we start with the increment expression:

^code for-desugar (1 before, 1 after)

The increment -- if there is one -- executes right after the body statement in each iteration of the loop. We do that by replacing the current body with a little block that contains the original body followed by an expression statement that evaluates the increment.

Next, we take the condition and the body and build the loop itself using a while loop. If the condition is omitted, we jam in `true` to make an infinite loop.

Finally, if there is an initializer, it runs once before the entire loop. We do that by, again, replacing the whole statement with a block that runs the initializer and then executes the loop.

That's it. Our interpreter now supports C-style for loops and we didn't have to touch the Interpreter class at all. Since we desugared to nodes it already knows how to visit, there is no more work to do.

<div class="challenges">

## Challenges

1.  A few chapters from now, when Lox supports first-class functions and dynamic
    dispatch, then we technically won't *need* branching statements built into
    the language. Show how conditional executed can be implemented in terms of
    those. Name a language that uses this technique for its control flow.

2.  Likewise, looping can be implemented using those same tools, provided our
    interpreter supports an important optimization. What is it, and why is it
    necessary? Name a language that uses this technique for iteration.

3.  Unlike Lox, most other C-style languages also support break and continue
    statements inside loops. Add support for break statements.

    The syntax is a `break` keyword followed by a semicolon. It should be a
    syntax error to have a break statement appear outside of any enclosing loop.

    At runtime, a break statement causes execution to jump to the end of the
    nearest enclosing loop and proceed from there. Note that the break may be
    nested inside other blocks and if statements that also need to be exited.

</div>

<div class="design-note">

## Design Note: Spoonfuls of Syntactic Sugar

As you design your own language, you must decide how much syntactic sugar to
stuff into the grammar. Do you make an unsweetened health food where each
semantic maps directly to exactly one syntactic unit, or some decadent dessert
where every bit of behavior can be expressed ten different ways? There are
successful languages all along this continuum.

On the most extreme savory end are languages with ruthlessly minimal syntax like
Lisp, Forth, and Smalltalk. Lispers famously claim their language "has no
syntax", while Smalltalkers proudly show that you can fit the entire grammar on
an index card. This tribe has the philosophy that they don't need syntactic
sugar. Instead, the language is designed such that library code can be as
beautiful as any native grammatical features would be.

Near these are languages like C, Lua, and Go. They aim for simplicity and
clarity over minimalism. Some, like Go, deliberately eschew both syntactic sugar
and the kind of syntactic extensibility of the previous category. They want the
syntax to get out of the way of the semantics, so they focus on keeping both the
grammar and libraries simple. Code should be obvious more than beautiful.

Somewhere in the middle you have languages like Java, C# and Python, though over
time, the center tends to creep towards ever-sweeter grammars.

Then you get to Ruby, C++, Perl, and D, languages who have stuffed so much
syntax into their grammar, they are running out of punctuation characters on the
keyboard.

To some degree, location on the spectrum correlates to age. It's relatively easy
to add new bits of syntactic sugar in later versions. New syntax is a crowd
pleaser, and it's less likely to break existing programs than mucking with the
semantics is. Once added, you can never take it away, so languages tend to
accumulate syntactic sugar over time.

One of the main benefits of creating a new language from scratch is it gives you
an opportunity to scrape off some of those layers of accumulated frosting and
start over.

Syntactic sugar has a bad rep among the PL intelligentsia. There's a real fetish
for minimalism among that crowd. There is some justification for that. Poorly
designed unneeded syntax raises the cognitive load without adding enough
expressiveness to carry its weight. Since there is always pressure to cram new
features into the language, it takes discipline and a focus on simplicity to
avoid bloat. Once you add some syntax, you're stuck with it, so it's smart to be
cautious.

At the same time, most successful languages do have fairly complex grammars, at
least by the time they are widely used. Programmers spend a ton of time in their
language of choice, and a few niceties here and there really can improve the
comfort and efficiency of their work.

Striking the right balance -- choosing the right level of sweetness for your
language -- relies on your own sense of taste.

</div>
