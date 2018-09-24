^title Scanning on Demand
^part A Bytecode Virtual Machine

> Literature is idiosyncratic arrangements in horizontal lines in only twenty-six symbols, ten arabic numbers, and about eight punctuation marks.
>
> <cite>Kurt Vonnegut</cite>

Our second interpreter, clox, has three phases -- scanner, compiler, and virtual
machine. A data structure joins each pair of phases. Tokens flow from scanner to
compiler, and chunks of bytecode from compiler to VM. We began our
implementation near the end with [chunks][] and the [VM][]. Now, we're going to
hop back to the beginning and build a scanner that makes tokens. In the [next
chapter][], we'll tie the two ends together with our bytecode compiler.

[chunks]: chunks-of-bytecode.html
[vm]: a-virtual-machine.html
[next chapter]: compiling-expressions.html

<img src="image/scanning-on-demand/pipeline.png" alt="Source code &rarr; scanner &rarr; tokens &rarr; compiler &rarr; bytecode chunk &rarr; VM." />

I'll admit, this is not the most exciting chapter in the book. With two
implementations of the same language, there's bound to be some redundancy. I did
sneak in a few interesting differences compared to jlox's scanner. Read on to
see what they are.

## Spinning Up the Interpreter

Now that we're building the front end, we can get clox running like a real
interpreter. No more hand-authored chunks of bytecode. It's time for a REPL and
script loading. Tear out most of the code in `main()` and replace it with:

^code args (3 before, 2 after)

If you pass <span name="args">no arguments</span> to the executable, you are
dropped into the REPL. A single command line argument is understood to be the
path to a script to run.

<aside name="args">

The code tests for one and two arguments, not zero and one, because the first
argument in `argv` is always the name of the executable being run.

</aside>

We'll need a few system headers, so let's get them all out of the way:

^code main-includes (1 after)

And now to get the REPL up and REPL-ing:

^code repl

A quality REPL handles input that spans multiple lines gracefully and doesn't
have a hardcoded line length limit. This REPL here is a little more, ahem,
austere, but it's fine for our purposes.

The real work happens in `interpret()`. We'll get to that soon, but first let's
take care of loading scripts:

^code run-file

We read the file and execute the resulting string of Lox source code. Then,
based on the result of that, we set the exit code appropriately because we're
scrupulous tool builders and care about little details like that.

We also need to free the source code string because `readFile()` dynamically
allocates it and passes ownership to its caller. That function looks like this:

<aside name="owner">

C asks us not just to manage memory explicitly, but *mentally*. We programmers
have to remember the ownership rules and hand-implement them throughout the
program. Java just does it for us. C++ gives us tools to encode the policy
directly so that the compiler validates it for us.

I like C's simplicity, but we pay a real price for it -- the language requires
us to be more conscientious.

</aside>

^code read-file

Like a lot of C code, it takes more effort than it seems like it should,
especially for a language expressly designed for operating systems. The
difficult part is that we want to allocate a big enough string to read the whole
file, but we don't know how big the file is until we've read it.

The code here is the classic trick to solve that. We open the file, but before
reading it, we seek to the very end using `fseek()`. Then we call `ftell()`
which tells us how many bytes we are from the start of the file. Since we seeked
("sought"?) to the end, that's the size. We rewind back to the beginning,
allocate a string of that <span name="one">size</span>, and read the whole file
in a single batch.

<aside name="one">

Well, that size *plus one*. Always gotta remember to make room for the null
byte.

</aside>

So we're done, right? Not quite. These function calls, like most calls in the C
standard library, can fail. If this were Java, the failures would be thrown as
exceptions and automatically unwind the stack so we wouldn't *really* need to
handle them. In C, if we don't check for them, they silently get ignored.

This isn't really a book on good C programming practice, but I hate to encourage
bad style, so lets go ahead and handle the errors. It's good for us, like eating
our vegetables or flossing.

Fortunately, we don't need to do anything particulary clever if a failure
occurs. If we can't correctly read the user's script, all we can really do is
tell the user and exit the interpreter gracefully. First up, we might fail to
open the file:

^code no-file (1 before, 2 after)

This can happen if the file doesn't exist or the user doesn't have access to it.
It's pretty common -- people mistype paths all the time.

This failure is much rarer:

^code no-buffer (1 before, 1 after)

If you can't even allocate enough memory to read the Lox script, you've probably
got bigger problems to worry about, but we should do our best to at least let
you know.

Finally, the read itself may fail:

^code no-read (1 before, 1 after)

This is also unlikely. Actually, the <span name="printf"> calls</span> to
`fseek()`, `ftell()`, and `rewind()` could theoretically fail too, but let's not
go too far off in the weeds, shall we?

<aside name="printf">

Even good old `printf()` can fail. Yup. How many times have you handled *that*
error?

</aside>

### Opening the compilation pipeline

We've got ourselves a string of Lox source code, so now we're ready to set up a
pipeline to scan, compile, and execute it. It's driven by `interpret()`. Right
now, that function runs our old hard-coded test chunk. Let's change it to
something closer to its final incarnation:

^code vm-interpret-h (1 before, 1 after)

Where before we passed in a Chunk, now we pass in the string of source code.
Here's the new implementation:

^code vm-interpret-c (1 after)

We don't have the actual *compiler* yet in this chapter, but we can start laying
out its structure. It lives in a new module:

^code vm-include-compiler (1 before, 1 after)

For now, the one function in it is declared like so:

^code compiler-h

That will change, but it gets us going:

^code compiler-c

The first phase of compilation is scanning -- the thing we're doing in this
chapter -- so right now all the compiler does is set that up.

### The scanner scans

There's still a few more feet of scaffolding to stand up before we can start
writing useful code. First, a new header:

^code scanner-h

And its corresponding implementation:

^code scanner-c

Our scanner tracks where it is as it chews its way through the user's source
code. Like we did with the VM, we wrap that state in a struct and then create a
single top-level module variable of that type so we don't have to pass it around
all of the various functions.

There are surprisingly few fields. The `start` pointer marks the beginning of
the current lexeme being scanned, and `current` points to the current character
being looked at.

<span name="fields"></span>

<img src="image/scanning-on-demand/fields.png" alt="The start and current fields pointing at 'print bacon;'. Start points at 'b' and current points at 'o'." />

<aside name="fields">

Here, we are in the middle of scanning the identifier `bacon`. The current
character is `o` and the character we most recently consumed is `c`.

</aside>

We have a `line` field to track what line the current lexeme is on for error
reporting. That's it! We don't even keep a pointer to the beginning of the
source code string. The scanner works its way through the code once and is done
after that.

Since we have some state, we should initialize it:

^code init-scanner (1 before)

We start at the very first character on the very first line, like a runner
crouched and ready to run.

## A Token at a Time

In jlox, when the starting gun went off, the scanner raced ahead and eagerly
scanned the whole program, returning a list of tokens. This would be a challenge
in clox. We'd need some sort of growable array or list to store the tokens. We'd
need to manage allocating and freeing the tokens and the collection itself.
That's a lot of code, and a lot of memory churn.

At any point in time, the compiler only needs one or two tokens -- remember our
grammar only requires a single token of lookahead -- so we don't need to keep
them *all* around at the same time. Instead, the simplest solution is to not
scan a token until the compiler needs one. When the scanner provides one, it
returns the token by value. It doesn't need to dynamically allocate anything --
it can just pass tokens around on the C stack.

Unfortunately, we don't have a compiler yet that can ask the scanner for tokens,
so the scanner will just sit there doing nothing. To kick it into action, we'll
write some temporary code to drive it:

^code dump-tokens (1 before, 1 after)

<aside name="format">

That `%.*s` in the format string is a neat feature. Usually, you set the output
precision -- the number of characters to show -- by placing a number inside the
format string. Using `*` instead lets you pass the precision as an argument. So
that `printf()` call prints the first `token.length` characters of the string at
`token.start`. We need to limit the length like that because the lexeme points
into the original source string and doesn't have a terminator at the end.

</aside>

This loops indefinitely. Each turn through the loop, it scans one token and
prints it. When it reaches a special "end of file" token, it stops. For example,
if we run the interpreter on this program:

```
print 1 + 2;
```

It prints out:

```
   1 31 'print'
   | 21 '1'
   |  7 '+'
   | 21 '2'
   |  8 ';'
   2 39 ''
```

The first column is the line number, the second is the numeric value of the
token <span name="token">type</span>, and then finally the lexeme. That last
empty lexeme on line 2 is the EOF token.

<aside name="token">

Yeah, the raw index of the token type isn't exactly human readable, but it's all
C gives us.

</aside>

The goal for the rest of the chapter is to make that blob of code work. The key
function is this one:

^code scan-token-h (1 before, 2 after)

Each call to this scans and returns the next token in the source code. A token
looks like:

^code token-struct (1 before, 2 after)

It's pretty similar to jlox's Token class. We have an enum identifying what type
of token it is -- number, identifier, `+` operator, etc. The enum is virtually
identical to the one in jlox, so let's just hammer out the whole thing:

^code token-type (2 before, 2 after)

Aside from prefixing all the names with `TOKEN_` (since C tosses enum names in
the top level namespace) the only difference is that extra `TOKEN_ERROR` type.
What's that about?

There are only a couple of errors that get detected during scanning:
unterminated strings and unrecognized characters. In jlox, the scanner reports
those itself. In clox, the scanner produces a synthetic "error" token for that
error and passes it over to the compiler. This way, the compiler knows an error
occurred and can kick off error recovery before reporting it.

The novel part in clox's Token type is how it represents the lexeme. In jlox,
each Token stored the lexeme as its own separate little Java string. If we did
that for clox, we'd have to figure out how to manage the memory for those
strings. That's especially hard since we pass tokens by value
-- multiple tokens could point to the same lexeme string. Ownership gets weird.

Instead, we use the original source string as our character store. We represent
a lexeme by a pointer to its first character and the number of characters it
contains. This means we don't need to worry about managing memory for lexemes at
all and we can freely copy tokens around. As long as the main source code string
<span name="outlive">outlives</span> all of the tokens, everything works fine.

<aside name="outlive">

I don't mean to sound flippant. We really do need to think about and ensure that
the source string, which is created far away over in the "main" module, has a
long enough lifetime. That's why `runFile()` doesn't free the string until
`interpret()` finishes executing the code and returns.

</aside>

### Scanning tokens

We're ready to scan some tokens. We'll work our way up to the complete
implementation, starting with this:

^code scan-token

Since each call to this scans a complete token, we know we are at the beginning
of a new token when we enter the function. Thus, we set `scanner.start` to point
to the current character so we remember where the lexeme we're about to scan
starts.

Then we check to see if we've reached the end of the source code. If so, we
return an EOF token and stop. This is a sentinel value that signals to the
compiler to stop asking for more tokens.

If we aren't at the end, we do some... stuff... to scan the next token. But we
haven't written that code yet. We'll get to that soon. If that code doesn't
successfully scan and return a token, then we reach the end of the function.
That must mean we're at a character that the scanner can't recognize, so we
return an error token for that.

This function relies on a couple of helpers, most of which are familiar from
jlox. First up:

^code is-at-end

We require the source string to be a good null-terminated C string. If the
current character is the null byte, then we've reached the end.

To create a token, we have this constructor-like function:

^code make-token

It uses the scanner's `start` and `current` pointers to capture the token's
lexeme. It sets a couple of other obvious fields then returns the token. It has
a sister function for returning error tokens:

^code error-token

<span name="axolotl"></span>

<aside name="axolotl">
This part of the chapter is pretty dry, so here's a picture of an axolotl.

<img src="image/scanning-on-demand/axolotl.png" alt="A drawing of an axolotl." />
</aside>

The only difference is that the "lexeme" points to the error message string
instead of pointing into the user's source code. Again, we need to ensure that
the error message sticks around long enough for the compiler to read it. In
practice, we only ever call this function with C string literals. Those are
constant and eternal, so we're fine.

What we have now is basically a working scanner for a language with an empty
lexical grammar. Since the grammar has no productions, every character is an
error. That's not exactly a fun language to program in, so let's fill in the
rules.

## A Lexical Grammar for Lox

The simplest tokens are only a single character. We recognize those like so:

^code scan-char (1 before, 2 after)

We read the next character from the source code, and then do a straightforward
switch to see if it matches any of Lox's one-character lexemes. To read the next
character, we use this helper which consumes the current character and returns
it:

^code advance

Next up are the two-character punctuation tokens like `!=` and `>=`. Each of
these also has a corresponding single-character token. That means that when we
see a character like `!`, we don't know if we're in a `!` token or a `!=` until
we look at the next character too. We handle those like so:

^code two-char (1 before, 2 after)

After consuming the first character, we look for an `=`. If found, we consume it
and return the corresponding two-character token. Otherwise, we leave the
current character alone (so it can be part of the *next* token) and return the
appropriate one-character token.

That logic for conditionally consuming the second character lives here:

^code match

If the current character is the desired one, we advance and return `true`.
Otherwise, we return `false` to indicate it wasn't matched.

Now, we can handle all of the punctuation-like tokens. Before we get to the
longer ones, let's take a little side trip to handle characters that aren't part
of a token at all.

### Whitespace

Our scanner needs to handle spaces, tabs, and newlines, but those characters
don't become part of any token's lexeme. We could check for those inside the
main character switch in `scanToken()` but it gets a little tricky to ensure
that the function still correctly finds the next token *after* the whitespace
when you call it. We'd have to wrap the whole body of the function in a loop or
something.

Instead, before starting the token, we shunt off to a separate function:

^code call-skip-whitespace (1 before, 2 after)

This advances the scanner past any leading whitespace. After this call returns,
we know the very next character is a meaningful one (or we're at the end of the
source code).

^code skip-whitespace

It's sort of a separate mini-scanner. It loops, consuming every whitespace
character it encounters. We need to be careful that it does *not* consume any
*non*-whitespace characters. To support that, we use:

^code peek

This simply returns the current character, but doesn't consume it. The previous
code handles all the whitespace characters except for newlines:

^code newline (1 before, 2 after)

When we consume one of those, we also bump the current line number.

### Comments

Comments aren't technically "whitespace", if you want to get all precise with
your terminology, but as far as Lox is concerned, they may as well be, so we
skip those too:

^code comment (1 before, 2 after)

Comments start with `//` in Lox, so as with `!=` and friends, we need a second
character of lookahead. However, with `!=`, we still wanted to consume the `!`
even if the `=` wasn't found. Comments are different, if we don't find a second
`/`, then `skipWhitespace()` needs to not consume the *first* slash either.

To handle that, we add:

^code peek-next

This is like `peek()` but for one character past the current one. If the current
character and the next one are both `/`, we consume them and then any other
characters until the next newline or the end of the source code.

We use `peek()` to check for the newline but not consume it. That way, the
newline will be the current character on the next turn of the outer loop in
`skipWhitespace()` and we'll recognize it and increment `scanner.line`.

### Literal tokens

Number and string tokens are special because they have a runtime value
associated with them. We'll start with strings because they are easy to
recognize -- they always begin with a double quote:

^code scan-string (1 before, 2 after)

That calls:

^code string

Similar to jlox, we consume characters until we reach the closing quote. We also
track newlines inside the string literal. (Lox supports multi-line strings.)
And, as ever, we gracefully handle running out of source code before we find the
end quote.

The main change here in clox is something that's *not* present. Again, it
relates to memory management. In jlox, the Token class had a field of type
Object to store the runtime value converted from the literal token's lexeme.

Supporting that in C requires a lot of work. We need some sort of union and type
tag to tell whether the token contains a string or double value. If it's a
string, we need to manage the memory for the string's character array somehow.

Instead of adding that complexity to the scanner, we defer <span
name="convert">converting</span> the literal lexeme to a runtime value until
later. In clox, tokens only store the lexeme -- the character sequence exactly
as it appears in the user's source code. Later in the compiler, we'll convert
that lexeme to a runtime value right when we are ready to store it in the
chunk's constant table.

<aside name="convert">

Doing the lexeme to value conversion in the compiler does introduce some
redundancy. The work to scan a number literal is awfully similar to the work
required to convert a sequence of digit characters to a number value. But there
isn't *that* much redundancy, it isn't in anything performance-critical, and it
keeps our scanner simpler.

</aside>

Next up, numbers. Instead of adding a switch case for each of the ten digits
that can start a number, we handle them here:

^code scan-number (1 before, 2 after)

That uses this obvious utility function:

^code is-digit

We finish scanning the number using this:

^code number

It's virtually identical to jlox's version except, again, we don't convert the
lexeme to a double yet.

## Identifiers and Keywords

The last batch of tokens are identifiers, both user-defined and reserved. This
section should be fun -- the way we recognize keywords in clox is quite
different from how we did it in jlox, and touches on some important data
structures.

First, though, we have to scan the lexeme. Names start with a letter:

^code scan-identifier (1 before, 2 after)

Which can be any of:

^code is-alpha

Once we've found an identifier, we scan the rest of it using:

^code identifier

After the first letter, we allow digits too, and we keep consuming alphanumerics
until we run out of them. Then we produce a token with the proper type. "Proper
type" is where the excitement happens:

^code identifier-type

Okay, I guess that's not quite exciting yet. That's what it looks like if we
have no reserved words at all. How should we go about recognizing keywords? In
jlox, we stuffed them all in a Java Map and looked them up by name. We don't
have any sort of hash table structure in clox, at least not yet.

A hash table would be overkill anyway. To look up a string in a hash <span
name="hash">table</span>, we need to walk the string to calculate its hash code,
find the corresponding bucket in the hash table, and then do a
character-by-character equality comparison on any string it happens to find
there.

<aside name="hash">

Don't worry if this is unfamiliar to you. When we get to [building our own hash
table from scratch][hash], we'll learn all about it in exquisite detail.

[hash]: hash-tables.html

</aside>

Let's say we've scanned the identifier "gorgonzola". How much work *should* we
need to do to tell if that's a reserved word? Well, no Lox keyword starts with
"g", so looking at the first character is enough to definitively answer "no".
That's a lot simpler than a hash table lookup.

What about "cardigan"? We do have a keyword in Lox that starts with "c":
"class". But the second character in "cardigan", "a", rules that out. What about
"forest"? Since "for" is a keyword, we have to go farther in the string before
we can establish that we don't have a reserved word. But, in most cases, only a
character or two is enough to tell we've got a user-defined name in our hands.
We should be able to recognize that and fail fast.

Here's a visual representation of that branching character-inspection logic:

<span name="down"></span>

<img src="image/scanning-on-demand/keywords.png" alt="A trie that contains all of Lox's keywords." />

<aside name="down">

Read down each chain of nodes and you'll see Lox's keywords emerge.

</aside>

We start at the root node. If there is a child node whose letter matches the
first character in the lexeme, we move to that node. Then repeat for the next
letter in the lexeme and so on. If at any point the next letter in the lexeme
doesn't match a child node, then the identifier must not be a keyword and we
stop. If we reach a double-lined box, and we're at the last character of the
lexeme, then we found a keyword.

This tree diagram is an example of a thing called a <span
name="trie">[**trie**][trie]</span>. A trie stores a set of strings. Most other
data structures for storing strings contain the raw character arrays and then
wrap them inside some larger construct that helps you search faster. A trie is
different. Nowhere in the trie will you find a whole string.

[trie]: https://en.wikipedia.org/wiki/Trie

<aside name="trie">

"Trie" is one of the most confusing names in CS. Edward Fredkin yanked it out of
the middle of the word "retrieval", which means it should be pronounced like
"tree". But, uh, there is already a pretty important data structure pronounced
"tree" *which tries are a special case of*, so unless you never speak of these
things out loud, no one can tell which one you're talking about. Thus, people
these days often pronounce it like "try" to avoid the headache.

</aside>

Instead, each string the trie "contains" is represented as a *path* through the
tree of character nodes, as in our traversal above. Nodes that match the last
character in a string have a special marker -- the double lined boxes in the
illustration. That way, if your trie contains, say, "banquet" and "ban", you are
able to tell that it does *not* contain "banque" -- the "e" node won't have that
marker, while the "n" and "t" nodes will.

Tries are a special case of an even more fundamental data structure: a
[**deterministic finite automaton**][dfa] (DFA). You might also know these by
other names: **"finite state machine"**, or just **"state machine"**. State
machines are rad. They end up useful in everything from [game
programming][state] to implementing networking protocols.

[dfa]: https://en.wikipedia.org/wiki/Deterministic_finite_automaton
[state]: http://gameprogrammingpatterns.com/state.html

In a DFA, you have a set of *states* with *transitions* between them, forming a
graph. At any point in time, the machine is "in" exactly one state. It gets to
other states by following transitions. When you use a DFA for lexical analysis,
each transition is a character that gets matched from the string. Each state
respresents a set of allowed characters.

Our keyword tree is exactly a DFA that recognizes Lox keywords. But DFAs are
more powerful than simple trees because they can be arbitrary *graphs*.
Transitions can form cycles between states. That lets you recognize arbitrarily
long strings. For example, here's a DFA that recognizes number literals:

<span name="railroad"></span>

<img src="image/scanning-on-demand/numbers.png" alt="A syntax diagram that recognizes integer and floating point literals." />

<aside name="railroad">

This style of diagram is called a [**"syntax diagram"**][syntax diagram] or the
more charming **"railroad diagram"**. The latter name is because it looks
something like a switching yard for trains.

Back before Backus-Naur Form was a thing, this was one of the predominant ways
of documenting a language's grammar. These days, we mostly use text, but there's
something delightful about the official specification for a *textual language*
relying on an *image*.

[syntax diagram]: https://en.wikipedia.org/wiki/Syntax_diagram

</aside>

I've collapsed the nodes for the ten digits together to keep it more readable,
but the basic process works the same -- you work through the path, entering
nodes whenever you consume a corresponding character in the lexeme. If we were
so inclined, we could construct one big giant DFA that does *all* of the lexical
analysis for Lox, a single state machine that recognizes and spits out all of
the tokens we need.

However, crafting that mega-DFA by <span name="regex">hand</span> would be
challenging. That's why [Lex][] was created. You give it a simple textual
description of your lexical grammar -- a bunch of regular expressions -- and it
automatically generates a DFA for you and produces a pile of C code that
implements it.

[lex]: https://en.wikipedia.org/wiki/Lex_(software)

<aside name="regex">

This is also how most regular expression engines in programming languages and
text editors work under the hood. They take your regex string and convert it to
a DFA, which they then use to match strings.

If you want to learn the algorithm to convert a regular expression into a DFA,
[the Dragon Book][dragon] has you covered.

[dragon]: https://en.wikipedia.org/wiki/Compilers:_Principles,_Techniques,_and_Tools

</aside>

We won't go down that road. We already have a perfectly serviceable hand-rolled
scanner. We just need a tiny trie for recognizing keywords. How should we map
that to code?

The absolute simplest <span name="v8">solution</span> is to use a switch
statement for each node with cases for each branch. We'll start with the root
node and handle the easy keywords:

<aside name="v8">

Simple doesn't mean dumb. The same approach is [essentially what V8 does][v8],
and that's currently one of the world's most sophisticated, fastest language
implementations.

[v8]: https://github.com/v8/v8/blob/e77eebfe3b747fb315bd3baad09bec0953e53e68/src/parsing/scanner.cc#L1643

</aside>

^code keywords (1 before, 1 after)

These are the letters that correspond to a single keyword. If we see an "s", the
only keyword the identifier could possibly be is `super`. It might not be,
though, so we still need to check the rest of the letters too. In the tree
diagram, this is basically that straight path hanging off the "s".

We won't roll a switch for each of those nodes. Instead, we have a utility
function that tests the rest of a potential keyword's lexeme:

^code check-keyword

We use this for all of the unbranching paths in the tree. Once we've found a
prefix that could only be one possible reserved word, we need to verify two
things. The lexeme must be exactly as long as the keyword. If the first letter
is "s", the lexeme could still be "sup" or "superb". And the remaining
characters must match exactly -- "supar" isn't good enough.

If we do have the right number of characters, and they're the ones we want, then
it's a keyword, and we return the associated token type. Otherwise, it must be a
regular identifier.

We have a couple of keywords where the tree branches again after the first
letter. If the lexeme starts with "f", it could be `false`, `for`, or `fun`. So
we add another switch for the branches coming off the "f" node:

^code keyword-f (1 before, 1 after)

Before we switch, we need to check that there even *is* a second letter. "f" by
itself is a valid identifier too, after all. The other letter that branches is
"t":

^code keyword-t (1 before, 1 after)

That's it. A couple of nested switch statements. Not only is this code <span
name="short">short</span>, but it's very very fast. It does the minimum amount
of work required to detect a keyword, and bails out as soon as it can tell the
identifier will not be a reserved one.

And with that, our scanner is complete.

<aside name="short">

We sometimes fall into the trap of thinking that performance comes from
complicated data structures, layers of caching, and other fancy optimizations.
But, many times, all that's required is to do less work, and I often find that
writing the simplest code I can is sufficient to accomplish that.

</aside>

<div class="challenges">

## Challenges

1.  Many newer languages support [*string interpolation*][interp]. Inside a
    string literal, you have some sort of special delimiters -- most commonly
    `${` at the beginning and `}` at the end. Between those delimiters, any
    expression can appear. When the string literal is executed, the inner
    expression is evaluated, converted to a string, and then merged with the
    surrounding string literal.

    For example, if Lox supported string interpolation, then this:

        :::lox
        var drink = "Tea";
        var steep = 4;
        var cool = 2;
        print "${drink} will be ready in ${steep + cool} minutes.";

    Would print:

        Tea will be ready in 6 minutes.

    What token types would you define to implement a scanner for string
    interpolation? What sequence of tokens would you emit for the above string
    literal?

    What tokens would you emit for:

        :::text
        "Nested ${"interpolation?! Are you ${"mad?!"}"}"

    Consider looking at other language implementations that support
    interpolation to see how they handle it.

2.  Several languages use angle brackets for generics and also have a `>>` right
    shift operator. This led to a classic problem in early versions of C++:

        :::c++
        vector<vector<string>> nestedVectors;

    This would produce a compile error because the `>>` was lexed to a single
    right shift token, not two `>` tokens. Users were forced to avoid this by
    putting a space between the closing angle brackets.

    Later versions of C++ are smarter and can handle the above code. Java and C#
    never had the problem. How do those languages specify and implement this?

3.  Many languages, especially later in their evolution, define "contextual
    keywords". These are identifiers that act like reserved words in some
    contexts but can be normal user-defined identifiers in others.

    For example, `await` is a keyword inside an `async` method in C#, but
    in other methods, you can use `await` as your own identifier.

    Name a few contextual keywords from other languages, and the context where
    they are meaningful. What are the pros and cons of having contextual
    keywords? How would you implement them in your language's front end if you
    needed to?

[interp]: https://en.wikipedia.org/wiki/String_interpolation

</div>
