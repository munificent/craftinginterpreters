^title Parsing Expressions
^part A Tree-Walk Interpreter

> Grammar, which knows how to control even kings.
> <cite>Molière</cite>

This chapter marks the first major milestone of the book. <span
name="parse">Parsing</span> has a fearsome reputation. Many of us have cobbled
together a mishmash of regular expressions and substring operations in order to
extract some sense out of a pile of text. It was probably riddled with bugs
and a beast to maintain.

<aside name="parse">

"Parse" comes to English from the Old French "pars" which relates to "part of
speech". It means to take a text and figure out how each word maps to the
grammar of the language. We use it here in the same sense, except that our
language is a little more modern than Old French.

</aside>

Writing a *real* parser -- one with decent error-handling, a coherent internal structure, and the ability to robustly chew through a sophisticated syntax -- is considered a rare, impressive skill. In this chapter, you will <span name="attain">attain</span> it.

<aside name="attain">

Like many rites of passage, you'll probably find it looks a little smaller, a little less daunting when it's behind you than when it looms ahead.

</aside>

It's easier than you think, partially because we front-loaded a lot of the hard
work in the [last chapter][]. You already know your way around a formal grammar.
You're familiar with syntax trees, and we have some Java classes to represent
them. The only remaining piece is parsing -- transmogrifying a sequence of
tokens into one of those syntax trees.

[last chapter]: representing-code.html

Some computer science textbooks make a big deal out of parsing. In the 60s,
computer scientists -- reasonably fed up with programming in assembly
language -- started designing more sophisticated, <span
name="human">human</span>-friendly languages like FORTRAN and ALGOL. Alas, they
weren't very *machine*-friendly, for the primitive machines at the time.

<aside name="human">

Yeah, I called FORTRAN "human-friendly". Consider how bad assembly programming on those old machines must have been for *FORTRAN* to have been a major improvement. We are awash in luxury today.

</aside>

They designed languages that they honestly weren't even sure how to write compilers for, and then did ground-breaking work inventing parsing and compiling techniques that could handle these new big languages on those old tiny machines.

Classic compiler books read like fawning hagiographies of these pioneers and their tools. The cover of "Compilers: Principles, Techniques, and Tools" literally has a dragon labeled "complexity of compiler design" being slain by a knight bearing a sword and shield branded "LALR parser generator" and "syntax directed translation". They laid it on thick.

A little self-congratulation is well-deserved, but the truth is you don't need to know most of that stuff to bang out a high quality parser for a modern machine. As always, I encourage you to broaden your education and take it in later, but for now, we'll skip past their trophy case.

## Ambiguity and the Parsing Game

In the last chapter, I said you can "play" a context free grammar like a game
in order to generate strings. Now, we are playing that game in reverse. Given a
string -- a series of tokens -- we map those tokens to terminals in rules and
figure out which set of rules could have been used to generate that string.

The "could have been" part is interesting. It's entirely possible to create a
grammar that is *ambiguous*, where different choices of productions can lead to
the same string. When you're using the grammar to *generate* strings, that
doesn't matter much. Once you have the string, who cares how you got to it?

When parsing, ambiguity means the parser may misunderstand the user's code.
As a refresher, here's the Lox expression grammar we're working with:

```lox
expression → literal
           | unary
           | binary
           | grouping

literal    → NUMBER | STRING | "true" | "false" | "nil"
grouping   → "(" expression ")"
unary      → ( "-" | "!" ) expression
binary     → expression operator expression
operator   → "==" | "!=" | "<" | "<=" | ">" | ">="
           | "+"  | "-"  | "*" | "/"
```

This is a valid string in that grammar:

**TODO: illustrate**

    6 - 2 / 2

In order to generate it, we first pick the `binary` rule. Then, we need to
But there are two ways we could have generated it. One way is:

1. Starting at `expression`, pick `binary`.
2. For the left-hand `expression`, pick `NUMBER`, and use `6`.
3. For the operator, pick `"-"`.
4. For the right-hand `expression`, pick `binary` again.
5. In that nested `binary` expression, pick `2 / 2`.

Another is:

1. Starting at `expression`, pick `binary`.
2. For the left-hand `expression`, pick `binary` again.
3. In that nested `binary` expression, pick `6 - 2`.
4. Back at the outer `binary`, for the operator, pick `"-"`.
4. For the right-hand `expression`, pick `NUMBER`, and use `2`.

Those produce the same strings, but not the same *syntax trees*:

**TODO: illustrate**

That in turn affects the result of evaluating the expression. In the first form, the `2 / 2` is a subexpression of the outer `-`, so we evaluate that first, giving us `6 - 1` and a final result of `5`. In the other, the `6 - 2` is evaluated first to `4` and then that is divided by `2` giving `2`.

The way mathematicians have solved this since blackboards were first invented is by defining rules for precedence and <span name="nonassociative">associativity</span>.

*   **Precedence** determines which operator is evaluated first in an expression
    containing a mixture of different operators. Precedence rules tell us that
    we evaluate the `/` before the `-` in the above example. Operators with
    *higher* precedence are evaluated before operators with lower precedence.
    Equivalently, higher precedence operators are said to "bind tighter".

*   **Associativity** determines which operator is evaluated first in a series
    of the *same* operator. When an operator is **left-associative**, operators
    on the left evaluate before ones of the right. Since `-` is
    left-associative, this expression:

        :::lox
        5 - 3 - 1

    is equivalent to:

        :::lox
        (5 - 3) - 1)

    Assignment, on the other hand, is **right-associative**. This:

        :::lox
        a = b = c

    is equivalent to:

        :::lox
        a = (b = c)

<aside name="nonassociative">

While not common these days, some languages specify that certain pairs of operators have *no* relative precedence. That makes it a syntax error to mix those operators in an expression without using explicit grouping.

Likewise, some operators are **non-associative**. That means it's an error to use that operator more than once in a sequence.

</aside>

Without well-defined precedence and associativity, an expression that uses multiple operators is ambiguous -- it can be parsed into different syntax trees, which could in turn evaluate to different results. For Lox, we'll follow the same precedence rules as C, going from highest to lowest:

<table>
<thead>
<tr>
  <td>Name</td>
  <td>Operators</td>
  <td>Associates</td>
</tr>
</thead>
<tbody>
<tr>
  <td>Unary</td>
  <td><code>!</code> <code>-</code></td>
  <td>Right</td>
</tr>
<tr>
  <td>Factor</td>
  <td><code>/</code> <code>*</code></td>
  <td>Left</td>
</tr>
<tr>
  <td>Term</td>
  <td><code>-</code> <code>+</code></td>
  <td>Left</td>
</tr>
<tr>
  <td>Comparison</td>
  <td><code>&gt;</code> <code>&gt;=</code>
      <code>&lt;</code> <code>&lt;=</code></td>
  <td>Left</td>
</tr>
<tr>
  <td>Equality</td>
  <td><code>==</code> <code>!=</code></td>
  <td>Left</td>
</tr>
</tbody>
</table>

Let's take that and <span name="massage">massage</span> it into our context-free grammar. Right now, when we have an expression that contains subexpressions, like a binary operator, we allow *any* expression in there:

<aside name="massage">

Instead of baking precedence right into the grammar rules, some parser
generators let you keep the same ambiguous-but-simple grammar and then add in a
little explicit operator precedence metadata on the side in order to
disambiguate.

</aside>

```lox
binary → expression operator expression
```

The `expression` nonterminal allows us to pick any kind of expression as an operand, regardless of the operator we picked. The rules of precedence limit that. For example, an operand of a `*` expression *cannot* be `+` <span name="paren">expression</span>, since the latter has lower precedence.

<aside name="paren">

Of course, it could be a *parenthesized* addition expression, but that's because the parentheses let you sidestep the precedence relations.

</aside>

What we need is a nonterminal that means "any kind of expression of higher precedence than `*`". Something like:

```lox
addition → higherThanAddition "*" higherThanAddition
```

Since `*` and `/` ("factors" in math lingo) have the same precedence and the level above them is unary operators, a better approximation is:

```lox
factor → unary ( "*" | "/" ) unary
```

Except that's not *quite* right. We broke associativity. The above rule `factor` doesn't allow `1 * 2 * 3`. To support associativity, we can make one side permit expressions at the *same* level. Which side we choose determines if the grammar is left- or right-associative. Since multiplication and <span name="div">division</span> are left-associative, it would be:

<aside name="div">

Ignoring issues around floating-point roundoff and overflow, it doesn't really matter whether you treat multiplication as left- or right-associative -- you'll get the same result either way. Division definitely does matter.

</aside>

```lox
factor → factor ( "*" | "/" ) unary
```

This is technically correct, but the fact that the first non-terminal in the body of the rule is the same as the head of the rule means this production is *left recursive*. Some parsing techniques, including the one we're going to use have trouble with left recursion, so instead, we'll use this other formulation:

```lox
factor → unary ( ( "/" | "*" ) unary )*
```

At the grammar level, this sidesteps left recursion by saying a factor is a flat
*sequence* of multiplications and divisions. This mirrors the code we'll use to parse a sequence of factors. When we apply this transformation to every precedence level of binary operators, we get:

```lox
expression → equality
equality   → comparison ( ( "!=" | "==" ) comparison )*
comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )*
term       → factor ( ( "-" | "+" ) factor )*
factor     → unary ( ( "/" | "*" ) unary )*
unary      → ( "!" | "-" ) unary
           | primary
primary    → NUMBER | STRING | "true" | "false" | "nil"
           | "(" expression ")"
```

Instead of a single `binary` rule, there are now four separate rules for each binary operator precedence level. The main `expression` rule is no longer a single series of `|` branches for each kind of expression. Instead, it is simply an alias for the lowest-precedence expression form, <span name="equality">equality</span>, because that includes all higher-precedence expressions too.

<aside name="equality">

In later chapters, when we expand the grammar to include assignment and logical
operators, this will change, but equality is the lowest for now.

</aside>

Each binary operator's operands use the next-higher precedence level. After the binary operators, we go to `unary`, the rule for unary operator expressions, since those bind tighter than binary ones. For `unary`, we *do* use a recursive rule because unary operators are right-associative, which means instead of left recursion, we have right recursion. The `unary` nonterminal is at the end of the body for `unary`, not the beginning, and our parser won't have any trouble with that.

Finally, the `unary` rule alternately bubbles up to `primary` in cases where it doesn't match an unary operator. That's the established name for the highest level of precedence -- the atomic expressions like literals that all other expressions are ultimately composed of.

Now we have a solid unambiguous grammar. We're ready to make a parser for it.

## Recursive Descent Parsing

There are a whole pack of parsing techniques whose names mostly seem to be combinations of "L" and "R" -- [LL(k)][], [LR(1)][lr], [LALR][] -- along with more exotic beasts like [parser combinators][], [Earley parsers][], [the shunting yard algorithm][], and [packrat parsing][]. For our first interpreter, one technique is more than sufficient: **recursive descent**.

[ll(k)]: https://en.wikipedia.org/wiki/LL_parser
[lr]: https://en.wikipedia.org/wiki/LR_parser
[lalr]: https://en.wikipedia.org/wiki/LALR_parser
[parser combinators]: https://en.wikipedia.org/wiki/Parser_combinator
[earley parsers]: https://en.wikipedia.org/wiki/Earley_parser
[the shunting yard algorithm]: https://en.wikipedia.org/wiki/Shunting-yard_algorithm
[packrat parsing]: https://en.wikipedia.org/wiki/Parsing_expression_grammar

Recursive descent is about the simplest way to build a parser, and doesn't require using complex parser generator tools like Yacc, Bison and ANTLR. It's just straightforward hand-written code. Don't be fooled by its simplicity, though. Recursive descent parsers are fast, robust, support sophisticated error-handling, scale to large language grammars, and are easy to maintain. In fact, GCC, V8 (The JavaScript VM in Chrome), Roslyn (the C# compiler written in C#) and many other heavyweight production language implementations use recursive descent. It kicks ass.

It is considered a *top-down* parser because it starts from the top or outermost grammar rule (here `expression`) and works its way <span name="descent">down</span> into the nested subexpressions before finally reaching the leaves of the syntax tree. This is in contrast with bottom-up parsers like LR that start with primary expressions and compose them into larger and larger chunks of syntax.

<aside name="descent">

It's "recursive *descent*" because it walks *down* the grammar.

</aside>

A recursive descent parser is a straight transliteration of the grammar's rules directly into imperative code. Each rule becomes a function. The body of the is translated to the imperative code roughly like:

<table>
<thead>
<tr>
  <td>Grammar notation</td>
  <td>Code representation</td>
</tr>
</thead>
<tbody>
  <tr><td>Production rule</td><td>Function declaration</td></tr>
  <tr><td>Rule name</td><td>Function name</td></tr>
  <tr><td>Rule body</td><td>Function body</td></tr>
  <tr><td>Terminal</td><td>Code to match and consume a token</td></tr>
  <tr><td>Nonterminal</td><td>Call to that rule&rsquo;s function</td></tr>
  <tr><td><code>|</code></td><td>if or switch statement</td></tr>
  <tr><td><code>*</code> or <code>+</code></td><td>while or for loop</td></tr>
  <tr><td><code>?</code></td><td>if statement</td></tr>
</tbody>
</table>

It's called "*recursive* descent" because when a grammar rules refers to itself -- directly or indirectly -- that maps that to code using recursive method calls.

### The parser class

These parsing methods live inside a class:

^code parser

Like the scanner, it consumes a sequence, only now we're working at the level of entire tokens. It takes in a list of tokens and uses `current` to point to the next token to be consumed.

We're going to run straight through the expression grammar now and translate each rule to Java code. The first rule, `expression`, simply expands to the `equality` rule, so that's straightforward:

^code expression

Each method for parsing a grammar rule produces a syntax tree for that rule and returns it to the caller. When the body of the rule contains a nonterminal -- a reference to another rule -- we <span name="left">call</span> its method.

<aside name="left">

This is why left recursion is problematic for recursive descent. The function for a left recursive rule would immediately call itself, which would call itself again, and so on, until the parser hit a stack overflow and died.

</aside>

The rule for equality is a little more complex:

```lox
equality → comparison ( ( "!=" | "==" ) comparison )*
```

In Java, that becomes:

^code equality

Let's step through it. The left `comparison` nonterminal in the body is translated to the first call to `comparison()` and we store that in a local variable.

Then, the `( ... )*` loop in the rule is mapped to a while loop. We need to know when to exit that loop. We can see that inside the rule, we must first find either a `!=` or `==` token. So, if *don't* see one of those, we must be done with the sequence of equality operators. We express that check using a handy `match()` method:

^code match

It checks to see if the current token is any of the given types. If so, it consumes it and returns `true`. Otherwise, it returns `false` and leaves the token where it is.

The `match()` method is defined in terms of two more fundamental operations:

^code check

This returns `true` if the current token is of the given type. Unlike `match()`, it doesn't consume the token, it only looks at it.

^code advance

This consumes the current token and returns it, similar to how our scanner's `advance()` did with characters.

These bottom out on the last handful of tiny helper methods:

^code utils

`isAtEnd()` checks if we've run out of tokens to parse. `peek()` returns the current token we have yet to consume and `previous()` returns the most recently consumed token. The latter makes it easier to use `match()` and then access the just-matched token.

That's most of the parsing infrastructure we need. Where were we? Right, so if
we are inside the while loop in `equality()` now then we know we have found a `!=` or `==` operator and we must be parsing an equality expression.

We grab the token that we matched for the operator so we can track which kind of binary expression this is. Then we call `comparison()` again to parse the right-hand operand. We combine the operator and the two operands into a new `Expr.Binary` syntax tree node, and then loop around. Each time, we store the expression back in the same `expr` local variable. As we zip through a sequence of equality expressions, that creates a left-associative nested tree of binary operator syntax trees.

**TODO: illustrate**

We fall out of the loop once we hit something that's not an equality operator and we return the expression. Note how in cases where we don't encounter a single equality operator, we never enter the loop. In that case, the `equality()` method effectively calls and returns `comparison()`. In that way, this method matches an equality operator *or anything of higher precedence*.

Moving on to the next rule...

```lox
comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )*
```

Translated to Java:

^code comparison

The grammar rule is virtually identical and so is the code. The only <span name="handle">differences</span> are the token types for the operators we match, and the method we call for the operands.

<aside name="handle">

If you wanted to do some clever Java 8, you could create a helper method for parsing a left-associative series of binary operators given a list of token types and an operand method handle.

</aside>

The remaining two binary operator rules are follow the same pattern:

^code term-and-factor

That's all of the binary operators, parsed with the correct precedence and associativity. We're crawling up the precedence hierarchy and now we've reached the unary operators:

```lox
unary → ( "!" | "-" ) unary
      | primary
```

The code for this is a little different:

^code unary

Again, we look at the next token to see how to parse. If it's a `!` or `-`, we must have an unary expression. In that case, we grab the token, and then recursively call `unary()` again to the parse the operand, wrap it in an unary expression syntax tree and we're done.

Otherwise, we must have reached the highest level of precedence, primary expressions.

```lox
primary → NUMBER | STRING | "true" | "false" | "nil"
        | "(" expression ")"
```

Most of the cases for the rule are single terminals, so it's pretty straightforward:

^code primary

The interesting one is the code for handling parentheses. After we match an opening `(` and parse the expression after it, we *must* find a `)` token. If we don't, that's an error.

## Syntax Errors

A parser really has two jobs:

1.  Given a valid sequence of tokens, produce a corresponding syntax tree.
2.  Given an *invalid* sequence of tokens, detect the the error and tell the
    user about their mistake.

The second job is equally important as the first. In modern IDEs and editors, the parser is constantly reparsing code, often while the user is still editing it, in order to syntax highlight and support things like auto-complete. That means it will encounter code in incomplete half wrong states all the time.

When the user doesn't realize the syntax is wrong, it is up to the parser to help guide the user back onto the right path. The way it reports errors is a large part of your language's user interface.

Good syntax error handling is hard. By definition, the code isn't in a well-defined state, so there's no infallible way to know what the user *meant* to write. The parser can't read your <span name="telepathy">mind</span>.

<aside name="telepathy">

Not yet at least. With the way things are going in machine learning these days, it could be right around the corner.

</aside>

There are a couple of hard requirements the parser has to meet when it runs into a syntax error:

*   **It must detect and report the error.** If it doesn't detect the <span
    name="error">error</span> and passes the resulting malformed syntax tree on
    to the interpreter, who knows what's going to happen.

    <aside name="error">

    Philosophically speaking, if an error isn't detected and the interpreter runs the code, *is* it really an error?

    </aside>

*   **It must not crash or hang.** Syntax errors are a fact of life and language
    tools have to be robust in the face of them. Segfaulting or getting stuck in
    an infinite loop isn't allowed. While the code may be syntactically invalid,
    it's still a valid *input to the parser* because users use the parser to
    learn what syntax is allowed.

Those are the table stakes if you want to get in the parser game at all, but you really want to raise the ante beyond that. A decent parser should:

*   **Be fast.** Computers are thousands of times faster than when parser
    technology was first invented. The days of needing to optimize your parser
    so that it could get through an entire source file during a coffee break are
    over. But programmer expectations have risen just as quickly, if not faster.
    They expect their editors to reparse files in milliseconds after every keystroke.

*   **Accurately tell users *where* an error occurred.** They expect the parser
    to tell them as early in the token stream as possible when things go off
    kilter. We don't want to send them off on a wild goose chase to figure out
    where the mistake is hiding.

*   **Report as many distinct errors as there are.** Aborting after the first
    error is easy to implement, but it's annoying for users if every time they
    fix what they think is the one error in a file, the next one appears. They
    want to see them all.

*   **Minimize *cascaded* errors.** Once a single error is found, the parser
    no longer really knows what's going on. It tries to get itself back on
    track and keep going, but if it gets confused, it may report a slew of
    ghost errors that don't indicate other real problems in the code. When the
    first error is fixed, they disappear, because they really represent the
    parser's own confusion.

    These are annoying too because they can scare the user into thinking their
    code is in a worst state than it is.

The last two points are in tension. We want to report as many real separate errors as we can, but we don't want to report ones that are merely side effects of an earlier one.

The way a parser responds to an error and keeps going to look for later errors
is called **"error recovery"**. It was a hot research topic in the 60s. Back
then, you'd hand a stack of punch cards to the secretary and come back the next
day to see if the compiler succeeded. With an iteration loop that slow, you
*really* wanted to find every single error in your code in one pass.

Today, with the parser completing before you've even finished typing, it's less of an issue. Simple, fast error recovery is perfectly fine.

### Panic mode error recovery

Out of the all of the recovery techniques invented back then, the main one that's stood the test of time is called -- somewhat alarmingly -- "panic mode". As soon as the parser detects an error, it enters "panic mode". At this point, it knows it has some stream of tokens, and it knows it's in some place in the middle of some stack of productions in the grammar, and it knows the two do *not* agree with each other.

Before it can get back to parsing, it needs to **synchronize** the two. We select some well-defined point in the grammar that will be the target. The parser synchronizes its own parsing state by jumping out of any nested productions until it gets back to that rule. Then it synchronizes the token stream by discarding tokens until it reaches one that it knows appears at that boundary.

Any additional syntax errors hiding in those discarded tokens won't be reported. But it also means that any mistaken cascaded errors that are side effects of the initial error aren't falsely reported either, which is a decent trade-off.

The classic place in the grammar to synchronize to is a statement boundary. We don't have statements yet, so we won't actually synchronize there, but we'll get the machinery in place now for when we do.

### Synchronizing the parser

Back before we went on this side trek about error recovery, we were looking at the code for parsing a parenthesized expression. After parsing the expression, it looks for the closing `)` by calling `consume()`. Here it is:

^code consume

It's sort of like `match()` where it checks to see if the next token is of the expected type. If so, it consumes it and everything is groovy. If some other token is there, then we've hit an error. We report it by calling this:

^code error

First, it shows the error to the user by calling another helper error-reporting method on the main Lox class:

^code token-error

That's a convenience method for reporting an error at given token. It automatically shows the token's location and the token itself. This will come in handy later since we use tokens throughout the interpreter to track locations in code.

After this is called, the user knows about the syntax error, but what does the *parser* do next? Back in `error()`, it creates and returns a ParseError:

^code parse-error (1 before, 1 after)

This is a simple sentinel class we can use to unwind the parser. The `error()` method *returns* it instead of *throwing* because we want to let the caller decide if it should unwind or not.

Some parse errors occur in places where the parser isn't likely to get into a weird state and we don't need to <span name="production">synchronize</span>. In those places, we simply report the error and keep on truckin'. For example, Lox limits the number of arguments you can pass to a function. If you pass to many, the parser needs to report that error, but it can keep on parsing the extra arguments and ignore them.

<aside name="production">

A more graceful way to handle a common syntax error is with an **error production**. This is added rule in the grammar that the parser "correctly" parses, even though it represents a syntax error. After parsing, the parser reports the error and keeps going instead of creating a syntax tree for the erroneous code.

For example, some languages have an unary `+` operator, like `+123`, but Lox does not. Instead of getting confused when the parser stumbles onto a `+` in prefix position, we could extend the unary rule to allow `+`:

```lox
unary → ( "!" | "-" | "+" ) unary
        | primary
```

The parser will then parse it instead of going into panic mode. When it parses an unary `+`, it reports an error instead of creating an unary expression for it.

The nice thing about error productions is you how the code went wrong and what the user was most likely trying to do. That means you can give a more helpful message to get the user back on track, like "unary '+' expressions are not supported." For simple parsers, you can usually live without error productions, but mature parsers tend to accumulate them since they help users fix common mistakes.

</aside>

In our case, though, the syntax error is nasty enough that we want to panic and synchronize. To synchronize the token stream, we can simply call `advance()` to drop tokens. How do we synchronize the parser's own state.

In a recursive descent parser, the "state" that the parser is in is not stored explicitly in field. Instead, we use Java's own callstack to track what the parser is doing. Each rule that is in the middle of being parsed is a callframe on stack. In order to reset that state, we need to clear out those callframes.

The natural way to do that in Java is exceptions. When we want to synchronize, we *throw* that ParseError object. Higher up in the method for the grammar rule we are synchronizing to, we'll catch it. All of the nested grammar rules under that disappear in a puff of smoke and we're right where we need to be.

Since we are synchronizing on statement boundaries, we'll catch the exception there. Once the exception is caught, the parser is in the right state. All that's left is to synchronize the tokens.

We want to discard tokens until we're right at the beginning of the next statement. That boundary is pretty easy to spot -- that's one of the main reasons we picked it. *After* a semicolon, we're probably finished with a statement. Most statements start with a keyword -- `for`, `if`, `return`, `var`, etc. If the *next* token is any of those, we're <span name="semicolon">probably</span> about to start a statement.

<aside name="semicolon">

I say "probably" because we could hitting a semicolon that separates clauses in a for loop. Our synchronization isn't perfect, but that's OK. Remember, we already know the code is broken and won't be run, and we've already reported the first error precisely. Everything after this is kind of "best effort".

</aside>

We wrap that logic up into this method:

^code synchronize

It discards tokens until it thinks it found a statement boundary. After catching a ParseError, we'll call this and then we are hopefully back in sync. When it works well, we have discarded tokens that would have likely caused cascaded errors anyway and now we can parse the rest of the file.

Alas, we don't get to see this method in action yet, since we don't have
statements. We'll get to that in a couple of chapters. For now, if an error
occurs, we'll panic and unwind all the way to the top and stop parsing. Since we
can only parse a single expression anyway, that's no big loss.

## Wiring up the Parser

We are mostly done parsing expressions now. There is one other place where we need to add a little error handling. As the parser descends through the parsing methods for each grammar rule, it eventually hits `primary()`. What happens if none of the cases in there match? When that happens, it means we are sitting on a token that can't be used in an expression. That's an error too, so we report that:

^code primary-error (5 before, 1 after)

Now, we can hook up our brand new parser to the main Lox class and try it out.
We still don't have an interpreter, so for now, we'll parse to a syntax tree and
then use the AstPrinter from the [last chapter][ast-printer] to display it.

[ast-printer]: representing-code.html#a-(not-very)-pretty-printer

Delete the old temporary code to print the scanned tokens and replace it with
this:

^code print-ast (1 before, 1 after)

You can start to see the phases of the implementation take shape now. We read the source, then scan it to a list of tokens. Now we take those and pass them to the parser to get a syntax tree. We're getting there!

The entrypoint method into the parser is called, naturally enough, `parse()`:

^code parse

We'll redo this later when we add statements to the language. For now it parses a single expression and returns it. We also have some temporary code to safely handle going into panic mode. Syntax error recovery is the parser's job, so we don't want that exception to escape into the rest of the interpreter.

If a syntax error does occur, this method returns `null`. That's OK. The parser promises not to crash or hang on invalid syntax, but it doesn't promise to return a usable syntax tree if that happens. As soon as it reports an error, `hadError` gets set, and the later phases are reponsible for checking that before they try to run.

Congratulations, you have crossed the threshold. That really is all there is to hand-writing a parser. We'll extend the grammar in later chapters with assignment, statements, and other stuff, but none of that is any more complex than the binary operators we tackled here.

Fire up the interpreter and type in some expressions. See how it handles precedence and associativity correctly? Neat, huh?

<div class="challenges">

## Challenges

1.  Add prefix and postfix `++` and `--` operators. (You will have to define
    token types for them and add them to the scanner too.) Given them the same
    precedence as C. Add them to the grammar, and then implement parser support
    for them.

2.  Likewise, add support for the C-style conditional or "ternary" operator
    `?:`.

3.  Add error productions to handle the various binary operators appearing
    without a left-hand operand. In other words, detect a binary operator
    appearing in prefix position. Report that as an error, but also parse and
    discard a right-hand operand with the appropriate precedence.

</div>

<div class="design-note">

## Design Note: Novelty Budget

</div>

**TODO: design note on familiarity versus idealism. should fix c grammar mistake for "|" or not?**
