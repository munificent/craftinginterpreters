^title Scanning
^part A Tree-Walk Interpreter

> Take big bites. Anything worth doing is worth overdoing.
>
> <cite>Robert A. Heinlein</cite>

The first step in any compiler or interpreter is <span
name="lexing">scanning</span>. The scanner takes in raw source code as a series
characters and groups it into meaningful chunks -- the "words" and "punctuation"
that make up the language's grammar.

<aside name="lexing">

This task has been variously called "scanning" and "lexing" (short for "lexical
analysis") over the years. Way back when computers were as big as Winebagos but
had less memory than your watch, some people used "scanner" only to refer to the
piece of code that dealt with reading raw source code characters from disk and
buffering them in memory. Then "lexing" was the phase after that that did useful
stuff with the characters.

These days, reading a source file into memory is trivial, so it's rarely a
distinct phase in the compiler. Because of that two terms are basically
interchangeable.

</aside>

Scanning is a good starting point for us too because the code isn't very hard --
pretty much a switch statement with delusions of grandeur. It will help us warm
up before we tackle some of the more interesting material later. By the end of
this chapter, we'll have a full-featured, fast scanner that can take any string
of Lox source code and produce the tokens that we'll feed into the parser in the
next chapter.

## The Interpreter Framework

Since this is our first real chapter, before we get to actually scanning some
code, we need to sketch out the basic shape of our interpreter, jlox. Everything
starts with a class in Java:

^code lox-class

Stick that in a text file, and go get your IDE or Makefile or whatever set up.
I'll be right here when you're ready. Good? OK!

Lox is a scripting language, which means it executes directly from source. There
are actually two ways you can run some code. If you start jlox from the command
line and give it a path to a file, it reads the file and executes it:

^code run-file

If you want a more intimate conversation with your interpreter, you can also run
it interactively. Fire up jlox without any arguments, and it drops you into a
prompt where you can enter and execute code one line at a time.

<aside name="repl">

An interactive prompt is also called a "REPL" (pronounced like "rebel" but with
a "p"). The name comes from Lisp where implementing one is as simple as
wrapping a loop around a few built-in functions:

```lisp
(print (eval (read))
```

Working outwards from the most nested call, you **R**ead a line of input,
**E**valuate it, **P**rint the result, then **L**oop and do it all over again.

</aside>

^code prompt

(Escape that infinite loop by hitting Control-C or throwing your machine at the
wall if you have anger management problems.)

Both the prompt and the file runner are thin wrappers around this core function:

^code run

It's not super useful yet since we haven't written the interpreter, but baby
steps, you know? Right now, it prints out the tokens our forthcoming scanner
will emit so that we can see if we're making progress.

### Error handling

While we're setting things up, another key piece of infrastructure is *error
handling*. Textbooks sometimes gloss over this because it's more a practical
matter than a formal computer science-y problem. But if you care about making a
language that's actually *usable*, then handling errors gracefully is vital.

The tools our language provides for dealing with errors make up a large portion
of its user interface. When the user's code is working, they aren't thinking
about our language at all -- their headspace is all about *their program*. It's
usually only when things go wrong that they notice our implementation.

<span name="errors">When</span> that happens, it's up to us to give the user all
of the information they need to understand what went wrong and guide them gently
back to where they are trying to go. Doing that well means thinking about error
handling all through the implementation of our interpreter, starting now:

<aside name="errors">

Having said all that, for *this* interpreter, what we'll build is pretty bare
bones. I'd love to talk about interactive debuggers, static analyzers and other
fun stuff, but there's only so much ink in the pen.

</aside>

^code lox-error

This tells users some syntax error occurred on a given line. This is really the
bare minimum to be able to claim you even *have* error reporting. Imagine if you
accidentally left a dangling comma in some function call and the interpreter
printed out:

```text
Error: Unexpected "," *somewhere* in your program. Good luck finding it!
```

That's not very helpful. We need to at least point them to the right line. Even
better would be the beginning and end column so they know *where* in the line.
Even better than *that* is to *show* the user the offending line, like:

```text
Error: Unexpected "," in argument list.

    15 | function(first, second,);
                               ^-- Here.
```

I'd love to implement something like that in this book but the honest truth is
that it's a lot of grungy string munging code. Very useful for users, but not
super fun to read in a book and not very technically interesting. So we'll stick
with just a line number. In your own interpreters, please do as I say and not as
I do.

The primary reason we're sticking this error reporting function in the main Lox
class is because of that `hadError` field. It's defined here:

^code had-error (1 before)

We'll use this to ensure we don't try to execute code that has a known error.
Also, it lets us exit with a non-zero exit code like a good command line citizen
should:

^code exit-code (1 before, 1 after)

We need to reset this flag in the interactive loop. If the user makes a mistake,
it shouldn't kill their entire session:

^code reset-had-error (1 before, 1 after)

The other reason I pulled the error reporting out here instead of stuffing it
into the scanner and other phases where the error occurs is to remind you that
it's a good engineering practice to separate the code that *generates* the
errors from the code that *reports* them.

Various phases of the front end will detect errors, but it's not really their
job to know how to present that to a user. In a full-featured language
implementation, you will likely have multiple ways errors get displayed: on
stderr, in an IDE's error window, logged to a file, etc. You don't want that
code smeared all over your scanner and parser.

Ideally, we would have an actual abstraction, some kind of <span
name="reporter">"ErrorReporter"</span> interface that gets passed to the scanner
and parser so that we can swap out different reporting strategies. For our
simple interpreter here, I didn't do that, but I did at least move the code for
error reporting into a different class.

<aside name="reporter">

I had exactly that when I first implemented jlox. I ended up tearing it out
because it felt over-engineered for the minimal interpreter in this book.

</aside>

With that in place, our application shell is ready. Once we have a Scanner class
with a `scanTokens()` method, we can start running it. Before we get to that,
let's talk about these mysterious "tokens".

## Tokens and Lexemes

Here's a line of Lox code:

```lox
var language = "lox";
```

Here, `var` is the keyword for declaring a variable. That three-character
sequence 'v' 'a' 'r' *means* something. If we yank three letters out of the
middle of `language`, like `gua`, those don't mean anything on their own.

That's what lexical analysis is about. Our job is to scan through the list of
characters and group them together into the smallest sequences that still
represent something. Each of these blobs of characters is called a **lexeme**.
In that example line of code, the lexemes are:

<img src="image/scanning/lexemes.png" alt="'var', 'language', '=', 'lox', ';'" />

The lexemes are only the raw substrings of the source code. However, in the
process of recognizing them, we also stumble upon some other useful information.
Things like:

### Lexeme type

Keywords are part of the shape of the language's grammar, so the parser often
has code like, "If the next token is `while` then do..." That means the parser
wants to know not just that it has a lexeme for some identifier, but that it has
a *reserved* word, and *which* keyword it is.

The <span name="ugly">parser</span> could determine that from the raw lexeme by
comparing the strings, but that's slow and kind of ugly. Instead, at the point
that we recognize a lexeme, we also remember which *kind* of lexeme it
represents. We have a different type for each keyword, operator, bit of
punctuation, and literal type:

<aside name="ugly">

After all, string comparison ends up looking at individual characters, and isn't
that the *scanner's* job?

</aside>

^code token-type

### Literal value

There are lexemes for literal values -- numbers and strings and the like. Since
the scanner has to walk each character in the literal to correctly identify it,
it can also convert it to the real runtime value that will be used by the
interpreter later.

### Location information

Back when I was preaching the gospel about error handling, we saw that we need
to tell users *where* errors occurred. Tracking that starts here. In our simple
interpreter, we only note which line the token appears on, but more
sophisticated implementations include the column and length too.

<aside name="location">

Some token implementations store the location as two numbers: the offset from
the beginning of the source file to the beginning of the lexeme, and the length
of the lexeme. The scanner needs to know these anyway, so there's no overhead to
calculate them.

An offset can be converted to line and column positions later by looking back at
the source file and counting the preceding newlines. That sounds slow, and it
is. However *you only need to do it when you need to actually display a line and
column to the user.* Most tokens never appear in an error message. For those,
the less time you spend calculating position information ahead of time, the
better.

</aside>

We take all of this and wrap it up in a class:

^code token-class

That's a **token**: a bundle containing the raw lexeme along with the other
things the scanner learned about it.

## Regular Languages and Expressions

Now that we know what we're trying to produce, let's, well, produce it. The core
of the scanner is a loop. Starting at the first character of the source code, it
figures out what lexeme it belongs to, and consumes it and any following
characters that are part of that lexeme. When it reaches the end of that lexeme,
it emits a token.

Then it loops back and does it again, starting from the very next character in
the source code. It keeps doing that, eating characters and occasionally, uh,
excreting tokens, until it runs out of characters.

<span name="alligator"></span>

<img src="image/scanning/lexigator.png" alt="An alligator eating characters and, well, you don't want to know." />

<aside name="alligator">

Lexical analygator.

</aside>

The part of the loop where we look at a handful of characters to figure out
which kind of lexeme it "matches" may sound familiar. If you know regular
expressions, you might consider defining a regex for each kind of lexeme and use
those to match characters. For example, Lox has the same identifier rules as C.
This regex matches one:

```text
[a-zA-Z_][a-zA-Z_0-9]*
```

If you did think of regular expressions, your intuition is a deep one. The rules
that determine how characters are grouped into lexemes for some language are
called its <span name="theory">**lexical grammar**</span>. In Lox, as in most
programming languages, the rules of that grammar are simple enough to be
classified a **[regular language][]**. That's the same "regular" as in regular
expressions.

[regular language]: https://en.wikipedia.org/wiki/Regular_language

<aside name="theory">

It pains me to gloss over the theory so much, especially when it's as fun as I
think the [Chomsky hierarchy][] and [FSMs][] are. But the honest truth is other
books cover this better than I could. [Compilers: Principles, Techniques, and
Tools][dragon] (universally known as "the Dragon Book") is the canonical
reference.

[chomsky hierarchy]: https://en.wikipedia.org/wiki/Chomsky_hierarchy
[dragon]: https://en.wikipedia.org/wiki/Compilers:_Principles,_Techniques,_and_Tools
[fsms]: https://en.wikipedia.org/wiki/Finite-state_machine

</aside>

You very precisely *can* recognize all of the different lexemes for Lox using
regexes if you want to, and there's a pile of interesting theory underlying why
that is and what it means. Tools like [Lex][] or
[Flex][] are designed expressly to let you do this -- throw a handful of regexes
at them, and they give you a complete scanner <span name="lex">back</span>.

<aside name="lex">

Lex was created by Mike Lesk and Eric Schmidt. Yes, the same Eric Schmidt who is
executive chairman of Google as of this writing. I'm not saying programming
languages are a sure-fire path to wealth and fame, but we *can* count at least
one multi-billionaire among us.

</aside>

[lex]: http://dinosaur.compilertools.net/lex/
[flex]: https://github.com/westes/flex

Since our goal is to understand how a scanner does what it does, we won't be
delegating that task. We're about hand-crafted goods.

## The Scanner Class

Without further ado, let's make ourselves a scanner.

^code scanner-class

<aside name="static-import">

I know static imports are considered bad style by some, but they save me from
having to sprinkle `TokenType.` all over the scanner and parser. Forgive me, but
every character counts in a book.

</aside>

We store the raw source code as a simple string, and we have a list ready to
fill with tokens we're going to generate. The aforementioned loop that does that
looks like this:

^code scan-tokens

It works its way through the source code, adding tokens, until it runs out of
characters. When it's done, it appends one final "end of file" token. That isn't
strictly needed, but it makes our parser a little cleaner.

This loop depends on a couple of fields to keep track of where in the source
code we are:

^code scan-state (1 before, 2 after)

The `start` and `current` fields are offsets in the string -- the first
character in the current lexeme being scanned, and the character we're currently
considering. The other field tracks what source line `current` is on so we can
produce tokens that know their location.

Then we have one little helper function that tells us if we've consumed all the
characters:

^code is-at-end

## Recognizing Lexemes

Each turn of the loop, we scan a single token. This is the real heart of the
scanner. We'll start simple. Imagine if every lexeme was only a single character
long. All you need to do is consume the next character and pick a token type for
it. Several lexemes *are* only a single character in Lox, so let's start with
those:

^code scan-token

Again, we need a couple of helper methods:

^code advance-and-add-token

The `advance()` method consumes the next character in the source file and
returns it. Where `advance()` is for input, `addToken()` is for output. It grabs
the text of the current lexeme and creates a new token for it. (We'll use the
other overload to handle tokens with literal values later.)

### Lexical errors

Before we get too far in, let's take a moment to think about errors at the
lexical level. What happens if a user throws a source file containing some
characters Lox doesn't use, like `@#^` at our interpreter? Right now, those
characters get silently added to the next token. That ain't right.

Let's fix that:

^code char-error (1 before, 1 after)

Note that the erroneous character is still *consumed* by the earlier call to
`advance()`. That's important so that we don't get stuck in an infinite loop.

Note also that we <span name="shotgun">*keep scanning*</span>. There may be
other errors later in the program. It gives our users a better experience if we
detect as many of those as possible in one go. Otherwise, they see one tiny
error and fix it, only to have the next error appear, and so on. Syntax error
whack-a-mole is no fun.

(Don't worry. Since `hadError` gets set, we'll never try to *execute* any of the
code, even though we keep going and scan the rest of it.)

<aside name="shotgun">

The code reports each invalid character separately, so this shotguns the user
with a blast of errors if they accidentally paste a big blob of weird text.
Coalescing a run of invalid characters into a single error would give a nicer
user experience.

</aside>

### Operators

We have single-character lexemes covered, but that doesn't cover all of Lox's
operators. What about `!`? It's a single character, right? Sometimes, yes, but
not when it's followed by a `=`. In that case, it should be a `!=` lexeme.
Likewise, `<`, `>`, and `=` can all be followed by `=`.

For those, we need to look at the second character:

^code two-char-tokens (1 before, 2 after)

Those use this new method:

^code match

It's like a conditional `advance()`. It only consumes the current character if
it's what we're looking for.

Using that, we recognize these lexemes in two stages. When we hit, say `!`, we
jump to its switch case. That case means "OK, we know the lexeme *starts* with
`!`". Then we look at the next character to determine if we're on a `!` or a
`!=`.

## Longer Lexemes

We're still missing one operator, `/`. That one needs a little special handling
because comments begin with a slash too.

^code slash (1 before, 2 after)

This is similar to the other two-character operators, except that when we find a
second `/`, we don't end the token yet. Instead, we keep consuming characters
until we reach the end of the line.

This is our general strategy for handling longer lexemes. After we detect the
beginning of one, we shunt off to some code specific to that kind of lexeme that
keeps eating characters until it sees the end.

We've got another helper:

^code peek

It's sort of like `advance()`, but doesn't consume the character. This is called
<span name="match">**lookahead**</span>. Since it only looks at the current
unconsumed character, we have *one character of lookahead*. The smaller this
number is, generally, the faster the scanner runs. The lexical grammar dictates
how much lookahead we need. Fortunately, most languages in wide use only need
one or two characters of lookahead.

<aside name="match">

Technically, `match()` is doing lookahead too. `advance()` and `peek()` are the
fundamental operators and `match()` combines them.

</aside>

Comments are lexemes, but they aren't meaningful, and the parser doesn't want
to deal with them. So when we reach the end of the comment, we *don't* call
`addToken()`. When we loop back around to start the next lexeme, `start` gets
reset and the comment's lexeme disappears in a puff of smoke.

Now's a good time to skip over those other meaningless characters, newlines and
whitespace, too:

^code whitespace (1 before, 3 after)

When encountering whitespace, we simply go back to the beginning of the scan
loop. That starts a new lexeme *after* the whitespace character. For newlines,
we do the same thing, but we also increment the line counter. (This is why we
used `peek()` to find the newline ending a comment instead of `match()`. We want
that newline to get here and update `line`.)

Our scanner is starting to feel more real now. It can handle fairly free-form
code like:

```lox
// this is a comment
(( )){} // grouping stuff
!*+-/=<> <= == // operators
```

### String literals

Now that we're comfortable with longer lexemes, we're ready to tackle literals.
We'll do strings first, since they always begin with a specific character, `"`:

^code string-start (1 before, 2 after)

That calls:

^code string

Like with comments, it consumes characters until it hits the `"` that ends the
string. It also gracefully handles running out of input before the string is
closed and reports an error for that.

For no particular reason, Lox supports multi-line strings. There are pros and
cons to that, but prohibiting them was a little more complex than allowing them,
so I left them in. That does mean we also need to update `line` when we hit a
newline inside a string.

Finally, the last interesting bit is that when we create the token, we also
produce the actual string *value* that will be used later by the interpreter.
Here, that conversion only requires a `substring()` to strip off the surrounding
quotes. If Lox supported escapes sequences like `\n`, we'd unescape those here.

### Number literals

All numbers in Lox are floating point at runtime, but it supports both integer
and decimal literals. A number literal is a series of <span
name="minus">digits</span> optionally followed by a `.` and one or more digits:

<aside name="minus">

Since we only look for a digit to start a number, that means `-123` is not a
number *literal*. Instead, `-123`, is an *expression* that applies `-` to the
number literal `123`. In practice, the result is the same, though it has one
interesting edge case if we were to add method calls on numbers. Consider:

```lox
print -123.abs();
```

This prints `-123` because `-` has lower precedence than `.`. We could fix that
by making `-` part of the number literal. But then consider:

```lox
var n = 123;
print -n.abs();
```

This still produces `-123`, so now the language seems inconsistent. No matter
what you do, some case ends up weird.

</aside>

```lox
1234
12.34
```

We don't allow a leading or trailing decimal point, so these are both invalid:

```lox
.1234
1234.
```

We could easily support the former, but I left it out to keep things simple. The
latter gets weird if we ever want to allow methods on numbers like `123.sqrt()`.

To recognize the beginning of a number lexeme, we look for any digit. It's kind
of tedious to add cases for every decimal digit, so we'll stuff it in the
default case instead:

^code digit-start (1 before, 1 after)

This relies on:

^code is-digit

<aside name="is-digit">

The Java standard library provides [`Character.isDigit()`][is-digit] which seems
like a good fit. Alas, that method allows things like Devangari digits,
fullwidth numbers, and other funny stuff we don't want.

[is-digit]: http://docs.oracle.com/javase/7/docs/api/java/lang/Character.html#isDigit(char)

</aside>

Once we know we are in a number, we branch to a separate method to consume the
rest of the literal, like we do with strings:

^code number

It consumes as many digits as it finds for the integer part of the literal. Then
it looks for a fractional part, which is a decimal point (`.`) followed by at
least one digit. This requires another character of lookahead since we don't
want to consume the `.` until we're sure there is a digit *after* it. So we
add:

^code peek-next

<aside name="peek-next">

I could have made `peek()` take a parameter for the number of characters ahead
to look instead of defining two functions, but that would allow *arbitrarily*
far lookahead. Only providing these two functions makes it clearer to a reader
of the code that our scanner only looks ahead at most two characters.

</aside>

If we do have a fractional part, again, we consume as many digits as we can
find.

Finally, we convert the lexeme to its numeric value. Our interpreter uses Java's
`Double` type to represent numbers, so we produce a value of that type. We're
using Java's own parsing method to convert the lexeme to a real Java double. We
could implement it ourselves, but really, unless you're trying to cram for an
upcoming programming interview, it's not worth your time.

The remaining literals are Booleans and `null`, but we handle those as keywords,
which gets us to...

## Reserved Words and Identifiers

Our scanner is almost done. The only remaining pieces of the lexical grammar to
implement are identifiers and their close cousins the reserved words. You might
think we could match keywords like `or` in the same way we handle
multiple-character operators like `<=`:

```java
case 'o':
  if (peek() == 'r') {
    addToken(OR);
  }
  break;
```

Consider what would happen if a user named a variable `orchid`. The scanner
would see the first two letters, `or`, and immediately emit an `or` keyword
token. This gets us to an important principle called <span
name="maximal">**maximal munch**</span>. When two lexical grammar rules can both
match a chunk of code that the scanner is looking at, *whichever one matches the
most characters wins*.

That rule states that if we can match `orchid` as an identifier and `or` as a
keyword, then the former wins. This is also why we tacitly assumed above that
`<=` should be scanned as a single `<=` token and not `<` followed by `=`.

<aside name="maximal">

Consider this nasty bit of C code:

```c
---a;
```

Is it valid? That depends on how the scanner splits the lexemes. If the scanner
sees it like:

```c
- --a;
```

Then it could be parsed. But that would require the scanner to know about the
grammatical structure of the surrounding code, which entangles things more than
we want. Instead, the maximal munch rule says that it is *always* scanned like:

```c
-- -a;
```

It scans it that way even though doing so leads to a syntax error later in the
parser.

</aside>

Maximal munch means we can't easily detect a reserved word until we've reached
the end of what might instead be an identifier. After all, a reserved word *is*
an identifier, it's just one that has been claimed by the language for its own
use. That's where the term **"reserved word"** comes from.

Instead, we assume any lexeme starting with a letter or underscore is an
identifier:

^code identifier-start (3 before, 3 after)

That calls:

^code identifier

Those use these helpers:

^code is-alpha

Now identifiers are working. To handle keywords, we see if the identifier's
lexeme is one of the reserved words. If so, we use a token type specific to that
keyword. We define this set of reserved words in a map:

^code keyword-map

Then, after we scan an identifier, we check to see if it matches one of these
keywords:

^code keyword-type (2 before, 1 after)

If so, we use that keyword's token type. Otherwise, it's a regular user-defined
identifier.

And with that, we now have a complete scanner for the entire Lox lexical
grammar. Fire up the REPL and type in some valid and invalid code. Does it
produce the tokens you expect? Try to come up with some interesting edge cases
and see if it handles them as it should.

<div class="challenges">

## Challenges

1.  The lexical grammars of Python and Haskell are not *regular*. What does that
    mean, and why aren't they?

1.  Aside from separating tokens -- distinguishing `print foo` from `printfoo`
    -- spaces aren't used for much in most languages. However, in a couple of
    dark corners, a space *does* affect how code is parsed in CoffeeScript,
    Ruby, and the C preprocessor. Where and what effect does it have in each of
    those languages?

1.  Our scanner here, like most, discards comments and whitespace since those
    aren't needed by the parser. Why might you want to write a scanner that does
    *not* discard those? What would it be useful for?

1.  Add support to Lox's scanner for C-style `/* ... */` block comments. Make
    sure to handle newlines in them. Consider allowing them to nest. Is adding
    support for nesting more work than you expected? Why?

</div>

<div class="design-note">

## Design Note: Implicit Semicolons

Programmers today are spoiled for choice in languages and have gotten picky
about syntax. They want their language to look clean and modern. One bit of
syntactic lichen that almost every new language scrapes off (and some ancient
ones like BASIC never had) is `;` as an explicit statement terminator.

Instead, they treat a newline as a statement terminator where it makes sense to
do so. The "where it makes sense" part is the interesting bit. While *most*
statements are on their own line, sometimes you need to spread a single
statement across a couple of lines. Those intermingled newlines should not be
treated as terminators.

Most of the obvious cases where the newline should be ignored are easy to
detect, but there are a handful of nasty ones:

* A return value on the next line:

        :::js
        return
        "value"

    Is "value" the value being returned, or do we have a return statement with
    no value followed by an expression statement containing a string literal?

* A parenthesized expression on the next line:

        :::js
        func
        (parenthsized)

    Is this a call to `func(parenthesized)`, or two expression statements, one
    for `func` and one for a parenthesized expression?

* A `-` on the next line:

        :::js
        first
        -second

    Is this `first - second` -- an infix subtraction -- or two expression
    statements, one for `first` and one to negate `second`?

In all of these, either treating the newline as a separator or not would both
produce valid code, but possibly not the code the user wants. Across languages,
there is an unsettling variety in the rules they use to decide which newlines
are separators. Here are a couple:

*   [Lua][] completely ignores newlines, but carefully controls its grammar such
    that no separator between statements is needed at all in most cases. This is
    perfectly legit:

        :::lua
        a = 1 b = 2

    Lua avoids the `return` problem above by requiring a `return` statement to
    be the very last statement in a block. If there is a value after `return`
    before the keyword `end`, it *must* be for the return. For the other two
    cases, they allow an explicit `;` and expect users to use that. In practice,
    that almost never happens because there's no point in a parenthesized or
    unary negation expression statement.

*   [Go][] handles newlines in the scanner. If a newline appears following one
    of a handful of token types that are known to potentially end a statement,
    the newline is treated like a semicolon, otherwise it is ignored. The Go
    team provides a canonical code formatter, [gofmt][], and the ecosystem is
    fervent about its use, which ensures that idiomatic styled code works well
    with this simple rule.

*   [Python][] treats all newlines as significent unless an explicit backslash
    is used at the end of a line to continue it to the next line. Also, newlines
    anywhere inside a pair of brackets (`()`, `[]`, or `{}`) are ignored.
    Idiomatic style strongly prefers the latter.

    This rule works well for Python because it is a deeply statement-oriented
    language. In particular, Python's grammar ensures a statement never appears
    inside an expression. C does the same, but many other languages which have a
    "lambda" or function literal syntax do not.

    For example, in JavaScript:

        :::js
        console.log(function() {
          statement();
        });

    Here, the `console.log()` *expression* contains a function literal which
    in turn contains the *statement* `statement();`.

    Python would need a different set of rules for implicitly joining lines if
    you could get back *into* a <span name="lambda">statement</span> where
    newlines should become meaningful while still nested inside brackets.

<aside name="lambda">

And now you know why Python's `lambda` only allows a single expression body.

</aside>

*   JavaScript's "[automatic semicolon insertion][asi]" rule is the odd one.
    Where other languages assume most newlines *are* meaningful and only a few
    should be ignored in multi-line statements, JS assumes the opposite. It
    treats all of your newlines as meaningless whitespace *unless* it
    encounters a parse error. If it does, it goes back and figures out the
    minimal set of newlines to turn into semicolons to get to something
    grammatically valid.

    This design note would turn into a design diatribe if I went into complete
    detail about how that even *works*, much less all the various ways that that
    is a bad idea. It's a mess. JavaScript is the only language I know where
    many style guides demand explicit semicolons after every statement even
    though the language theoretically lets you elide them.

[lua]: https://www.lua.org/pil/1.1.html
[go]: https://golang.org/ref/spec#Semicolons
[gofmt]: https://golang.org/cmd/gofmt/
[python]: https://docs.python.org/3.5/reference/lexical_analysis.html#implicit-line-joining
[asi]: https://www.ecma-international.org/ecma-262/5.1/#sec-7.9

If you're designing a new language, you almost surely *should* avoid an explicit
statement terminator. Programmers are creatures of fashion like other humans and
semicolons are as passé as ALL CAPS KEYWORDS. Just make sure you pick a set of
rules that make sense for your language's particular grammar and idioms. And
don't do what JavaScript did.

</div>
