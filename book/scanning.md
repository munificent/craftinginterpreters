^title Scanning
^part A Tree-Walk Interpreter

> Take big bites. Anything worth doing is worth overdoing.
>
> <cite>Robert A. Heinlein</cite>

The first step in any compiler or interpreter is <span name="lexing">scanning</span>. The scanner takes in the raw source code as a series characters and groups them into meaningful chunks -- the "words" and "punctuation" that make up the language's artificial grammar.

<aside name="lexing">

This task has been variously called "scanning" and "lexing" -- short for lexical analysis -- over the years and the two terms don't have very distinct definitions. Way back when computers were as big as Winebagos but had less memory than your watch, some people used "scanner" only to refer to the piece of code that deals with reading raw source code characters from disk and buffering them in memory. Then lexing was the phase after that that did useful stuff with the characters.

These days, reading a source file into memory is an easier task and the two terms are basically interchangeable.

</aside>

This is a good starting for us too because the code isn't very hard -- pretty much a switch statement with delusions of grandeur. It will let us get warmed up before we tackle some of the more interesting material later.

By the end of this chapter, we'll have a full-featured fast scanner that can handle any string of Lox source code and produce the tokens that we'll feed into the parser in the next chapter.

## The Interpreter Framework

Since this is our first real chapter, before we get to actually scanning some code, we need to tie together the basic application framework for our interpreter, jlox.

Because everything starts with a class in Java, we'll begin with like so:

^code lox-class

Stick that in a text file, and go get your IDE and Makefile or whatever set up. I'll be right here when you're ready. Good? OK, let's keep going.

Lox is a scripting language, which means it executes directly from source. There are actually two ways you can run some code. If you start jlox from the command line and give it a path to a file, it reads it and executes it:

^code run-file

If you like a closer emotional connection to your interpreter, you can also run it interactively. If you fire up jlox without any arguments, it drops you into a prompt where you can enter and execute code one line at a time.

<aside name="repl">

An interactive prompt is also called a "REPL" (pronounced like "ripple" but with
an "e"). The name comes from Lisp where providing one is as simple as
wrapping a loop around a few built-in functions:

```lisp
(print (eval (read))
```

Working outwards from the most nested function call, you **R**ead a line of input, **E**valuate it, **P**rint the result, then **L**oop and do it all over again.

</aside>

^code prompt

To escape that infinite loop in jlox, hit Control-C or yank the plug out of your machine if you have anger management problems.

Both the prompt and the file runner are thin wrappers around this core function:

^code run

It's not super useful yet since we haven't written the interpreter, but baby steps, you know? Right now, we'll just have it print out the tokens our forthcoming scanner spits out so that we can see if we're making progress. Feel free to run this periodically as you work you way through the chapter to see how it does.

### Error handling

While we're setting things up, another key piece of infrastructure is *error handling*. Textbooks sometimes gloss over this because it's more an engineering concern than a formal computer science-y problem. But if you care about making a language that's actually *usable*, then how your interpreter handles errors is a vital concern.

The tools your language provides for dealing with errors make up a large portion of your language's user interface. When their code is working, they aren't thinking about your language at all -- they're headspace is all about *their program*. It's only when things go sideways that they focus on your beautiful language and its implementation.

When that happens, it's up to us to give the user all of the information they need to understand what went wrong and guide them tactfully back to where they are trying to go. Doing that well means thinking about error handling all through the implementation of our interpreter, starting now.

Having said all that, for *this* interpreter, what we'll build is pretty bare bones. I'd love to talk about interactive debuggers, static analyzers and other fun stuff, but there's only so much time.

For now, we'll start with:

^code lox-error

This tells users some syntax error occurred on a given line. This is really the bare minimum to be able to claim you even *have* error reporting. Imagine you accidentally leave a dangling comma in some function call and the interpreter prints out:

```
Error: Unexpected "," somewhere in your program. Good luck finding it!
```

So we at least need to point them to the right line. Even better would be the beginning and end column so they know where in the line. Even better than *that* is to *show* the user the offending line, like:

```text
Error: Unexpected "," in argument list.

    15 | function(first, second,);
                               ^-- Here.
```

I'd love to implement something like that in this book but the honest truth is that it's a lot of grungy string munging code. Very useful for users, but not super fun to read in a book and not very technically interesting. So we'll stick with just a line number. In your interpreters, please do kick it up a notch.

The primary reason we're sticking this error reporting function in the main Lox class is because of that `hadError` field. It's defined here:

^code had-error (1 before)

If a syntax error occurs when running a script, we want to exit with a non-zero exit code like a good command line citizen should:

^code exit-code (1 before, 1 after)

We also need to reset it in the prompt. If the user makes a mistake, they should be able to keep going:

^code reset-had-error (1 before, 1 after)

The other reason I pulled the error reporting out here instead of stuffing it into the scanner and other phases where the error occurs is to remind you that it's a good engineering practice to separate the code that *generates* the errors from the code that *reports* them.

The various phases of the front end will detect errors, but it's not really their job to know how to present that to a user. In a full-featured language implementation, you will likely have multiple ways errors can get displayed: on stderr, in an IDE's error window, logged to a file, etc. You don't want that code jammed in your parser.

Ideally, we would have an actual abstraction, some kind of <span name="reporter">"ErrorReporter"</span> interface that gets passed to the scanner and parser so that we can swap out different reporting strategies. For our simple interpreter here, I didn't do that, but I did at least separate out the code for error reporting.

<aside name="reporter">

I had exactly that when I first implemented jlox. I ended up tearing it out
because it felt like pointless boilerplate in such a minimal implementation.

</aside>

And with that, our basic shell is in place. Once we have a Scanner class with some `scanTokens()` method, we can start running it. Before we get to that, les talk about these mysterious "tokens" and the prizes they may or may not be redeemed for.

## Tokens and Lexemes

Here's a line of Lox code:

```lox
var language = "lox";
```

Here, `var` is the keyword for declaring a variable. That three-character sequence *means* something. If we yank, say, `gua` out of the middle of `language`, those three characters don't mean anything on their own.

That's what lexical analysis is really about. Our job is to scan through the list of characters and group them together into the smallest possible sequences that still have a well-defined meaning. Each of these is called a **lexeme**.

In that line of code, the lexemes are:

```
var
language
=
"lox"
;
```

**TODO: illustrate**

The lexemes are just the raw substrings of the source code. However, in the process of recognizing those and drawing boundaries between each one, we also stumble upon some other useful information. Things like:

### Lexeme type

If the lexeme is an identifier whose name matches one of the language's reserved words, like `while` or `if`, we can recognize that now. Since keywords are part of the grammatical structure of the language, the parser often has logic like, "If the next token is `while` then parse a while statement."

Technically, the parser can determine that right from the lexeme by comparing the strings. But that's slow and kind of ugly. Instead, at the point that we recognize a lexeme, we'll also remember which *kind* of lexeme it represents. We'll have a different type for each keyword, operator, bit of punctuation, and literal value.

It's a simple enum:

^code token-type

### Literal value

Some lexemes represent literal values -- numbers and strings and the like.
Since the scanner has to walk each character in the literal to correctly identify it, it can also convert it to its actual runtime value as used by the interpreter later.

For example, after the scanner walks over the characters `123` in a number literal, we can convert it the actual numeric value 123.

### Location information

Back when I was on my soapbox about error handling, we saw that we need to tell users *where* errors occurred. We have to keep track of that information through every phase of the interpreter, starting here.

In our simple interpreter, we just track which line the token appears on, but more sophisticated implementations would track the column and length too.

<aside name="location">

Many token implementations store the location as two numbers: the number of characters from the beginning of the source file to the beginning of the lexeme, and the number of characters in the lexeme. The scanner needs to know these anyway, so there's no overhead to track them.

Those can be later converted to line and column positions later by looking back at the source file and counting the preceding newlines. That sounds slow, and it kind of is. However *you only need to do it when you need to actually display a line and column to the user.* The vast majority of tokens never appear in an error message. For those the less time you spend calculating position information ahead of time, the better.

</aside>

We take all of this and wrap it up in a class:

^code token-class

And *that's* what a **token** is -- a bundle containing the raw lexeme along with the other things the scanner knows about it.

## Regular Languages and Expressions

Now that we know what we're trying to produce, let's, well, produce it. The code of the scanner is a loop. Starting at the beginning of the source code, it figures out what lexeme the first character belongs to. Then it consumes any following characters that belong to that same lexeme.

When it hits the end of that lexeme, it emits a token. Then it loops back and does it again, starting from the very next character in the source code. It keeps doing that, eating characters and occasionally, uh, excreting tokens, until it runs out of characters.

**TODO: illustrate**

The first step inside the loop where we look at the first couple of characters and figure out which kind of lexeme it *matches* might sound familiar. If you're familiar with regular expressions, you might consider defining a regex for each kind of lexeme and use those to match characters. For example, Lox has the same identifier rules as C, and the regex `[a-zA-Z_][a-zA-Z_0-9]*` matches one.

If you did think of regular expressions by now, your <span
name="theory">intuition</span> is a deep one. The rules that determine how characters are associated with different lexemes for a language are called its **lexical grammar**. In Lox, as in most languages, the rules of that grammar are simple enough to within a boundary called a **[regular language][]**. That's the same "regular" as in regular expressions.

[regular language]: https://en.wikipedia.org/wiki/Regular_language

You very precisely *can* recognize all of the different lexemes for Lox using
regexes if you want to, and there's a pile of interesting theory underlying why that is and what that means. For
jlox, we could even use Java's regex library for our scanner. Or we could break
out a tool like <span name="lex">[Lex][]</span> or [Flex][] to take all of the regular expressions for
Lox's lexical grammar and spit out an entire scanner for us.

<aside name="lex">

Lex was created by Mike Lesk and Eric Schmidt. Yes, the same Eric Schmidt who ran Sun's software division, was a CEO of Novell, and is executive chairman of Google as of this writing. I'm not saying programming languages are a sure-fire path to wealth and fame, but we can count at least one multi-billionaire among us.

</aside>

[lex]: http://dinosaur.compilertools.net/lex/
[flex]: https://github.com/westes/flex

<aside name="theory">

It pains me to gloss over the theory so much, especially when it's as fun as I think the [Chomsky hierarchy][] and [FSMs][] are. But the honest truth is other books cover this better than I could. [Compilers: Principles, Techniques, and Tools][dragon] (universally known as "the Dragon Book") is a great place to slake your thirst for knowledge.

[chomsky hierarchy]: https://en.wikipedia.org/wiki/Chomsky_hierarchy
[dragon]: https://en.wikipedia.org/wiki/Compilers:_Principles,_Techniques,_and_Tools
[fsms]: https://en.wikipedia.org/wiki/Finite-state_machine

</aside>

But our goal is to understand how a scanner works inside, so we won't be
outsourcing that task. We're about hand-crafted goods here.

## The Scanner Class

Without further ado, let's make ourselves a scanner.

^code scanner-class

<aside name="files">

I know it seems like we're cranking out an awful lot of files. Java does tend to
be verbose, but I promise it gets less so in later chapters. Since we're just
getting started now, there's a lot of framework to put in place.

</aside>

We store the raw source code as a simple string, and we have an empty list that we will fill with tokens as we generate them. The aforementioned loop that does that looks like this:

^code scan-tokens

It works its way through the source code until it runs out of characters. When it's done, it adds one final special "end of file" token to the end. That isn't strictly needed, but it will make our parser a little cleaner.

This loop depends on a couple of fields to keep track of where in the source code we are:

^code scan-state (1 before, 2 after)

The `start` and `current` fields are indexes into the string -- the first character in the current lexeme being scanned, and the character we're currently considering. The other field tracks the line that contains `current`. We'll keep that updated as we go so we can produce tokens that know what line they occur on.

Then we have one little helper function:

^code is-at-end

## Recognizing Lexemes

Each turn of the loop, we scan the next token. This is the real heart of the scanner. We'll start simple. Imagine if every lexeme was only a single character long. To implement that, you can just consume the next character and pick a token type for it.

This works fine for several of Lox's real lexemes, so lets start there:

^code scan-token

Again, we need a couple of helper methods:

^code advance-and-add-token

The `advance()` method adds the next character to the lexeme we're currently building and then returns it. We can call this even before we know what kind of lexeme we're building since we know the character is going into *some* lexeme.

If `advance()` is the input then `addToken()` is the output. It grabs the text of the current lexeme and creates a new token for it. Later, we'll use the other overloaded version here to handle tokens with literal values.

### Lexical errors

Before we get too far in, let's take a moment to think about errors at the lexical level. Lox doesn't use the `@` character. What happens if a user throws a source file containing that at our interpreter? Right now, it just gets silently added to the next token. That ain't right.

Let's fix that:

^code char-error (1 before, 1 after)

Note that the erroneous character was still consumed by the call to `advance(). That's important so that we don't get stuck in an infinite loop.

Note also that we <span name="shotgun">keep scanning</span>. There may be other errors later in the program. It gives our users a better experience if we can detect as many of those as possible in one go. It's no fun if they see one tiny error and fix it, only to have the next error appear, and so on. Syntax error whack-a-mole is a drag.

<aside name="shotgun">

Because we report each invalid character as its own error, this will shotgun the user with a blast of errors if they accidentally paste a big blob of weird characters. If we wanted to be a little more nicer, we should coalesce a run of invalid characters into a single error.

</aside>

Even though we keep scanning, though, we won't try to *execute* this code. As soon as the first error is hit, `hadError` gets set so we know not to try to run it.

### Operators

We have single-character lexemes covered, but that doesn't cover all of Lox's operators. What about `!`? It's a single character, right? Sometimes, yes, but not if it's followed by a `=`. In that case, it should be a `!=` lexeme. Likewise, `<`, `>`, and `=` can all be followed by `=`.

For those, we need to look at the second character:

^code two-char-tokens (1 before, 2 after)

Those use this new method:

^code match

It's like a conditional `advance()`. It only consumes the current character if it's what we're looking for.

## Longer Lexemes

We're still missing one operator, `/`. That one needs a little special handling because we use `//` to begin a comment.

^code slash (1 before, 2 after)

This is roughly similar to the other two-character operators. However, when we match a second `/`, then we know we're in a comment. At that point, we keep consuming characters until we hit the end of the line.

This is our general strategy for handling longer lexemes. Once we've detected the beginning of a lexeme, we shunt off to some code specific to that kind of lexeme that keeps eating characters until it sees the end.

We've got another helper:

^code peek

It's sort of like `advance()`, but doesn't consume the character. This is called <span name="match">**lookahead**</span>. Since it only looks at the current unconsumed character, we have *one character of lookahead*. The smaller this number is, generally, the faster our scanner will run. The lexical grammar dictates how much lookahead we need. Fortunately, most languages in wide use are designed to be scanned with one or two characters of lookahead.

<aside name="match">

Technically, `match()` is doing lookahead too. `advance()` and `peek()` are the fundamental operators and `match()` combines them.

</aside>

After we consume all of the characters in the comment, we don't called `addToken()`. The lexeme just gets discarded the next time we loop around and start a new one. This is deliberate. Comments, by design, aren't interpreted by the language. By discarding them now, we simplify the parser since it won't have to worry about them.

Newlines and other whitespace are also ignored, and now is a good time to do that:

^code whitespace (1 before, 3 after)

For spaces and tabs, we simply go back to the beginning of the scan loop. That will start a new lexeme after the whitespace character. For newlines we do the same thing, but we also increment the line counter. (This is why we used `peek()` to find the newline after a comment instead of `match()`. We want that newline to get here and update `line`.)

Our scanner is starting to feel more real now. It can handle fairly free-form code like:

```lox
// this is a comment
(( )){} // grouping stuff
!*+-/=<> <= == // operators
```

### String literals

Now that we know how to handle multiple-character lexemes, we can add support for literals. We'll do strings first, since that always begin with a specific character, `"`:

^code string-start (1 before, 2 after)

That calls:

^code string

Like with comments, we keep consuming characters until we hit the `"` that ends the string. We also need to gracefully handle running out of input before the string is closed and report an error for that.

For no particular reason, Lox supports multi-line strings. There are pros and cons to that, but prohibiting them was a little more complex that allowing them, so I left them in. This does mean we also need to update `line` when we hit a newline inside a string.

Finally, the last interesting bit is that when we create the token, we also produce the actual *value* of the string literal that will be used later by the interpreter. Here, that conversion just requires a `substring()` to strip off the quotes. That's because we don't support escape sequences in Lox. If we did, we'd process those here.

### Number literals

All numbers in Lox are floating point at runtime, but it supports both integer and decimal literals. That means a number literal is a series of <span name="minus">digits</span> optionally followed by a `.` and one or more digits:

<aside name="minus">

Since we only look for a digit to start a number, that means `-123` is not a
number *literal*. Instead, `-123`, is an *expression* that applies `-` to the
number literal `123`. In practice, the result is the same, though it has one
interesting edge case if we were to add method calls on numbers. Consider:

```lox
print -123.abs();
```

This prints `-123` because `-` has lower precedence than `.`. We could fix that by making `-` part of the number literal. But then consider:

```lox
var n = 123;
print -n.abs();
```

Now this produces a different result than you'd get by repacing `n` with its value. No matter what you do, some case ends up weird.

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

We could easily support the former, but I left it out to keep things simple. The latter gets weird if we ever want to allow methods on numbers like `123.sqrt()`.

To recognize the beginning of a number lexeme, we look for any digit. It's kind of tedious to add cases for every decimal digit, so we'll stuff it in the default instead:

^code digit-start (1 before, 1 after)

This relies on:

^code is-digit

<aside name="is-digit">

The Java standard library provides [`Character.isDigit()`][is-digit] which seems like a good fit. Alas, that method allows things like Devangari digits, fullwidth numbers, and other funny stuff we don't want.

[is-digit]: http://docs.oracle.com/javase/7/docs/api/java/lang/Character.html#isDigit(char)

</aside>

Once we know we are in a number, we branch to a separate method to consume the rest of the literal, like we do with strings:

^code number

It consumes as many digits as it finds for the integer part of the literal.

Then it looks for a fractional part, which is a decimal point (`.`) followed by at least one digit. This requires another character of lookahead since we don't want to consume the `.` until we're sure there is a digit *after* it. So we'll add:

^code peek-next

If we have a fractional part, again, we consume as many digits as we can find.

Finally, we convert the lexeme to its numeric value. Our interpreter will use Java's `double` type to represent numbers, so we produce a value of that type. We're using Java's own parsing method to convert the lexeme to a real Java double. We could implement this ourselves, but really, unless you're trying to cram for an upcoming programming interview, it's not worth our time.

The remaining literals are Booleans and `null`, but we'll handle those as keywords, which gets us to...

## Reserved Words and Identifiers

Our scanner is almost done. The only things left in the lexical grammar to implement is identifiers and their close cousins the reserved words. You might think we could match keywords like we handle multiple-character operators like `<=`. Something like:

```java
case 'o':
  if (peek() == 'r') {
    addToken(OR);
  }
  break;
```

But now consider if a user names a variable `orchid`. We don't want the scanner to see the first to letters `or` and immediately emit an `or` keyword token. This gets us to an important principle in lexical grammars called <span name="maximal">**maximal munch**</span>. When the scanner is matching lexemes if two lexical grammar rules match some chunk of characters, *the longest one wins*.

That rule states that if we match `orchid` as an identifier and `or` as a keyword, the latter wins. It's also why we tacitly assume above that `<=` should be scanned as a single `<=` token and not `<` followed by `=`.

<aside name="maximal">

Consider this nasty bit of C code:

```c
---a;
```

Is this valid? It depends on how it's scanned. If the scanner sees it as:

```c
- --a;
```

Then it would work. But that would require the scanner to know about the grammatical structure of the surrounding code, which entangles things more than we want. Instead, the maximal munch rule says that it is *always* scanned like:

```c
-- -a;
```

Even though that becomes a syntax error later in the parser.

</aside>

This means we can't easily detect a reserved word until we've reached the end of what might instead be an identifier. After all, a reserved word *is* an identifier, it's just one that has been claimed by the language for its own use. That's where the term **"reserved word"** comes from.

Instead, we'll assume any lexeme starting with a letter or underscore is an identifier:

^code identifier-start (3 before, 3 after)

That calls:

^code identifier

Those use these helpers:

^code is-alpha

Now identifiers are working. To handle keywords, we just see if the identifier's lexeme is one of the reserved words. If so, we'll use a token type specific to that keyword. (That will make parsing easier later.) We'll define this set of reserved words in a map:

^code keyword-map

Then, after we scan an identifier, we check to see if it matches one of these keywords:

^code keyword-type (2 before, 1 after)

If so, we use that keyword's token type. Otherwise, it's a regular user-defined identifier.

And with that, we now have a complete scanner for the entire Lox lexical grammar. Fire up the REPL and type some valid and invalid code in. Does it produce the tokens you expect? Try to come up with some interesting edge cases and see if it handles them as it should.

<div class="challenges">

## Challenges

1.  The lexical grammars of Python and Haskell are not *regular*. What does that
    mean, and why aren't they?

1.  Aside from separating tokens -- distinguishing `print foo` from
    `printfoo` -- spaces aren't used for much in most languages. However, they
    do affect how code is parsed in CoffeeScript, Ruby, and the C preprocessor.
    Where and what effect do they have in each language?

1.  Our scanner here, like most, discards comments and whitespace since those
    aren't needed by the parser. Why might you want to write a scanner that does
    *not* discard those? What would it be useful for?

1.  Add support to Lox's scanner for C-style `/* ... */` block comments. Make
    sure the handle newlines in them. Consider allowing them to nest. Is adding
    support for nesting more work than you expected? Why?

</div>

<div class="design-note">

## Design Note: Implicit Semicolons

Programmers today are spoiled for choice in languages and have gotten picky about the look and feel of its syntax. They want their language to look clean and modern. One bit of syntactic lichen that almost every new language eliminates (and some ancient ones like BASIC never had) is `;` as an explicit statement separator.

Instead, they treat a newline as a statement separator where it makes sense to do so. The "where it makes sense" part is the interesting bit. While *most* statements are on their own line, sometimes you need to split it in the middle and break it across a couple of lines. Those newlines should not be treated as separators.

Most of the obvious cases are easy to detect, but there are a handful of nasty cases you run into:

* A return value on the next line:

        :::js
        return
        "value"

    Is "value" the value being returned, or do we have a return statement with no value followed by an expression statement containing a string literal?

* A parenthesized expression on the next line:

        :::js
        func
        (parenthsized)

    Is this a call to `func(parenthesized)`, or two expression statements, one for `func` and one for a parenthesized expression?

* A `-` on the next line:

        :::js
        first
        -second

    Is this `first - second` -- an infix subtraction -- or two expression statements, one for `first` and one to negate `second`?

In all of these, treating the newline as a separator not would both produce valid code, but possibly not the code the user wants. Across languages, there is an unsettling variety in the rules they use to decide which newlines are separators. Here are a couple:

* [Lua][] completely ignores newlines, but carefully controls its grammar such that no separator between statements is needed at all in most cases. This is perfectly valid:

        :::lua
        a = 1 b = 2

    It avoids the `return` problem above by requiring a `return` statement to be the very last statement in a block. If there is a value after `return` before the keyword `end`, it *must* be for the return. For the other two cases, they allow an explicit `;` and expect users to use that. In practice, that almost never happens because there's no point in a parenthesized or unary negation expression statement.

* [Go][] handles it in the scanner. If a newline appears following one of a handful of token types that are known to potentially end a statement, the newline is treated like a semicolon. The Go team provides a canonical code formatter, [gofmt][], and the ecosystem is fervent about its use, which ensures that idiomatic styled code works well with this simple rule.

* [Python][] treats all newlines as significent unless an explicit backslash is used at the end of a line to continue it to the next line. Also, newlines anywhere inside a pair of brackets (`()`, `[]`, or `{}`) are ignored. Idiomatic style strongly prefers the latter.

    This rule works well for Python because it is a strongly statement-oriented language. In particular, Python's grammar disallows a statement ever appearing inside an expression. This is also true of C, but not true of many other languages which have a "lambda" or function literal syntax.

    For example, in JavaScript, you can have:

        :::js
        console.log(function() {
          statementInAnExpression();
        });

    Python would need a different set of rules for implicitly joining lines if you could get back *into* a <span name="lambda">statement</span> where newlines should become meaningful while still nested inside brackets.

<aside name="lambda">

And now you know why Python's `lambda` only allows a single expression body.

</aside>

* JavaScript's "[automatic semicolon insertion][asi]" rules are the outliers. Where other languages assume most newlines *are* meaningful and there are just a few in multi-line statements that should be ignored, JS assumes the opposite. It treats all of your semicolons as meaningless whitespace unless that generates a parse error. If it does, it goes back and figures out the minimal set of newlines to turn into semicolons to get to something grammatically valid.

    This design note would turn into a design essay it I went into complete detail about how that even works, much less all the various ways that that is a bad idea. It's a mess. JavaScript is the only language I know where many style guides demand explicit semicolons after every statement even though the language theoretically lets you elide them.

[lua]: https://www.lua.org/pil/1.1.html
[go]: https://golang.org/ref/spec#Semicolons
[gofmt]: https://golang.org/cmd/gofmt/
[python]: https://docs.python.org/3.5/reference/lexical_analysis.html#implicit-line-joining
[asi]: https://www.ecma-international.org/ecma-262/5.1/#sec-7.9

If you're designing a new language, you almost surely *should* avoid an explicit statement separator. Programmers are creatures of fashion like other humans and semicolons are as passé as ALL CAPS KEYWORDS. Just make sure you pick a set of rules that make sense for your language's particular grammar and idioms. And, uh, don't do what JavaScript did.

</div>