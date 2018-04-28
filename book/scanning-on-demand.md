^title Scanning on Demand
^part A Bytecode Virtual Machine

> Literature is idiosyncratic arrangements in horizontal lines in only twenty-six symbols, ten arabic numbers, and about eight punctuation marks.
>
> <cite>Kurt Vonnegut</cite>

**todo: more illustrations?**

Our second interpreter clox has roughly three phases -- the scanner, the compiler, and the virtual machine. Between each pair is a data structure. Tokens flow between the scanner and compiler, and chunks of bytecode between the compiler and VM. We started near the end with [chunks][] and the [VM][]. Now we're going to circle back to the beginning and build a scanner that makes tokens. In the [next chapter][], we'll tie the two ends together with our bytecode compiler.

[chunks]: chunks-of-bytecode.html
[vm]: a-virtual-machine.html
[next chapter]: compiling-expressions.html

**todo: illustrate phases scanner -> tokens -> compiler -> chunk -> vm**

I'll admit, this is not the most exciting chapter in the book. With two implementations of the same languages, there's bound to be *some* redundancy. The basic techniques we use here are similar to the scanner in jlox. I did sneak in a few interesting differences, though, mostly driven by our need to manage memory ourselves in C. Ready to get started?

## Spinning Up the Interpreter

Now that we're building the front end, we can finally get clox running like a
real interpreter. No more executing a hand-authored chunk of bytecode and then
exiting. Like jlox has, clox is going to get a REPL and the ability to run Lox
scripts stored in files.

It starts in `main()`:

^code args (2 before, 1 after)

You invoke clox from the command line. If you pass <span name="args">no
arguments</span> to the executable, you are dropped into the REPL. A single
command line argument is understood to be the path to a script to run. More
arguments than that is an error.

<aside name="args">

The code actually tests for one and two arguments respectively. That's because
the first argument in `argv` is always the name of the executable being run.
Sort of the "zeroth" argument to the command.

</aside>

We need to include a few headers, so let's get that out of the way now:

^code main-includes (1 after)

And now to get the REPL up and... uh... REPLing:

^code repl

It's pretty rudimentary. A quality REPL handles input that spans multiple lines
gracefully and doesn't have a hardcoded line length limit. This REPL here is a
little more spartan, but it gets the job done.

The real work happens in `interpret()`. We'll get to that soon, but first let's
take care of running scripts.

### Running Lox scripts

If you pass an argument to clox, it loads the file at that path and runs it:

^code run-file

<span name="owner">We</span> read the file and execute the resulting string of
Lox source code. Then, based on the result of that, we set the exit code
appropriately because we're scrupulous tool builders and care about little
details like that.

We also need to free the source code string because `readFile()` dynamically
allocates it and passes ownership to us. Here's what
that function looks like:

<aside name="owner">

The fact that we have to know and remember the memory policy is a key pain point
of C. The language is too low-level to handle memory policy on its own as Java
does, but nor does it give us tools to actually encode the policies like you
could in C++. Instead, it's up to us fallible humans. C's simplicity is nice,
but it has its cost.

</aside>

^code read-file

Like a lot of C code, this takes more effort than it seems like it should,
especially for a language expressly designed for operating systems. You'd think
file I/O would come more easily, but I digress.

The tricky part is that we want to allocate a big enough string to read the
whole file, but we don't know how big the file is. The code here is the classic
trick to solve that. We open the file, but before reading it, we seek to the
very end of the file using `fseek()`. Then we call `ftell()` which tells us how
many bytes we are from the start of the file. Since we seeked ("sought"?) to the
end, that's the size.

Next, we rewind back to the beginning of a file, allocate a string of that <span
name="one">size</span>, and read the whole file in a single batch. Then the
usual remaining formalities -- ensuring the string is null-terminated and
closing the file.

<aside name="one">

Well, that size *plus one*. Always gotta remember to make room for the null
byte.

</aside>

So we're done, right? Not quite. These function calls, like most calls in the C
standard library, can <span name="printf">fail</span>. If this were Java, the
failures would be thrown as exceptions and automatically unwind the stack so we
wouldn't *really* need to handle them. In C, if we don't check for them, they
silently get ignored.

<aside name="printf">

Even good old `printf()` can fail. Yup. How many times have you checked for
*that* error?

</aside>

This isn't really a good on good C programming practices, but I don't want to
encourage sloppy programming either, so lets go ahead and handle the errors.
It's good for us, like eating our vegetables or flossing.

Fortunately, we don't need to do anything particulary clever if a failure occurs
here. If we can't even correctly read the user's script, all we can really do is
let the user know and exit the interpreter gracefully.

First up, we might fail to open the file:

^code no-file (1 before, 2 after)

This can happen if the file doesn't exist or the user doesn't have access to it.
This is actually pretty common -- people mis-type paths all the time.

This failure is much less common:

^code no-buffer (1 before, 1 after)

If you can't even allocate enough memory to read a little Lox script, you've
probably got bigger problems to worry about, but we should do our best to at
least let you know.

Finally, the read itself may fail:

^code no-read (1 before, 1 after)

This is also pretty unlikely. Actually, the calls to `fseek()`, `ftell()`, and
`rewind()` could theoretically fail too, but let's not go too far off in the
weeds, shall we?

## Opening the Compilation Pipeline

We've got ourselves a string of Lox source code, so now we're ready to set up a
pipeline to scan it, compile it, and execute it. That's all going to be wrapped
up in `interpret()`. Right now, that function is a hack to run our old
hard-coded test chunk. We're ready to change it to something closer to its final
incarnation:

^code vm-interpret-h (1 before, 1 after)

Where before we passed in a Chunk, now we pass in the string of source code.
Here's the new implementation:

^code vm-interpret-c (1 after)

The first step is to compile that source code. We don't have the actual
*compiler* yet in this chapter, but we can start laying out that code. It lives
in a new module:

^code vm-include-compiler (1 before, 1 after)

For now, the one function in it is declared like so:

^code compiler-h

That will change, but it's fine for now. Here's a first pass at implementing it:

^code compiler-c

The first phase of compilation in scanning -- you know, the thing we're doing
in this chapter -- so right now all the compiler does is set up that.

## The Scanner Scans

This is a lot of scaffolding code to set up but now we're starting to get to
something that does something:

^code scanner-h

This initial function sets up the scanner to get ready to tokenize the given
string of source code. The implementation is:

^code scanner-c

Our scanner has some state to keep track of where it is as it chews its way
through the user's source code. Like with did with the VM, we wrap that state up
in a single struct and then create a top-level module variable for it so we
don't have to thread it through all of the various functions.

It only needs a couple of little first. The `start` pointer marks the beginning
of the current lexeme being scanned, and `current` points to the current
character being looked at. It's the next character to be consumed, not the
most-recently consumed one.

Also, we have a `line` field to track of what line `current` is on in the user's
program. When we create tokens, we'll store that so that we can tell the user
what line errors occurred on.

That's all we need. Note that we don't even keep a pointer to the whole source
code string. The scanners works its way through the source string once and never
returns to the beginning.

Since we have some state, we need to initialize it:

^code init-scanner (1 before)

We start at the very first character on the very first line, like a runner
crouching down at the starting line, ready to sprint.

## A Token at a Time

We're almost ready to get started on the real code, but the starting gun hasn't
fired just yet. We don't have a compiler that will consume the output of our
scanner, so we need some temporary code to see what we're doing:

^code dump-tokens (1 before, 1 after)

This loops indefinitely. Each turn through the loop, it scans one token and
prints it. When it reaches a special "end of file" token, it stops. For example,
if we invoke the interpreter and throw this at it:

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

The first column is the line numbers, the second is the numeric value of the
token type (not exactly human-readable, I know), and then finally the lexeme.
That last empty token on line 2 here is the EOF one.

The goal for the rest of the chapter is to make that blob of code work. The key
function is this one:

^code scan-token-h (1 before, 2 after)

It tells the scanner to scan and return the next token in the source code. This
is the main difference between clox's scanner and the one we wrote for jlox. In
jlox, our scanner eagerly tokenized the entire input and produced a list of all
of the tokens at once. If we did that for clox, we'd need to allocate and grow
the list and then free it when the compiler no longer needed it. That's a lot of
effort, and a lot of memory churn.

At any point in time, the compiler only needs one or two tokens -- remember that
our grammar only requires a single token of lookahead after all -- so we don't
need to keep them *all* around at the same time. Instead, the simplest solution
is to not create a token until the compiler requests one.

Where our old interpreter pipeline ran the scanner and then *pushed* the
resulting tokens over to the parser, our new one inverts it. The parser will
*pull* tokens out of the scanner as it needs them. When the scanner returns one,
it returns the token by value. We don't need to dynamically allocate them at all
-- we can just pass them around on the C stack.

Here's what a token looks like:

^code token-struct (1 before, 2 after)

It's pretty similar to our Java Token class in jlox. There's an enum identifying
what type of token it is -- number, identifier, `+` operator, etc. There's a
line number. The interesting bit is how we represent the lexeme.

In jlox, each Token stored the lexeme as its own separate little Java string. If
we did that for clox, we'd have to figure out how to manage the memory for that
string. That's especially hard since we pass around Tokens by value -- multiple
tokens could point to the same lexeme string. Ownership gets weird.

Instead, we let the characters for the lexeme reside in the original source
string. We represent a lexeme by a pointer to its first character, and the
number of characters it takes up. This means we don't need to worry about
managing memory for lexemes at all and we can freely copy tokens around. As long
as the one string for the source code <span name="outlive">outlives</span> all
of the tokens, everything works out fine.

<aside name="outlive">

I don't mean to sound flippant. We really do need to actually think about and
ensure that the source string, which is created far away over in the main
module, has the right lifetime.

</aside>

That struct relies on an enum for the token's type. It's virtually identical to
the one in jlox, so let's just hammer out the whole thing:

^code token-type (2 before, 2 after)

Aside from prefixing them all with `TOKEN_` since C doesn't namespace enum
names, the only difference is that extra `TOKEN_ERROR` type. What's that about?
There are only a couple of errors that get detected during scanning -- things
like unterminated strings and unrecognized characters. In jlox, the scanner
reported those directly.

In clox, the scanner produces a synthetic "error" token for that error and
passes it over to the compiler. This way, the compiler knows an error occurred
and can kick off error recovery before reporting it.

That's the data representation of a token. Now let's start producing them. We'll
work our way up to the complete implementation, starting with this:

^code scan-token

Each call to this scans a complete token, so we know at the beginning of the
function that we are at the beginning of the next token. So we set
`scanner.start` to point to the current character so we can remember where the
token starts.

Then we check to see if we've reached the end of the source code. If so, we
return an EOF token and stop. This is a signal to the compiler to stop looking
for more tokens.

Otherwise, we do some... stuff... to scan the next token. But we haven't written
that code yet. We'll get to that soon. If that code
doesn't successfully scan and return a token, then we reach the end of the
function. That must mean we're at a character that the scanner can't recognize,
so we return an error token for that.

To get this working, we need a couple of helpers. Most of these will seem
familiar from jlow. First up:

^code is-at-end

We require the source string to be a good null-terminated C string. If the
current character is the null byte, then we have reached the end. For this check
to work, we do need to be careful to never advance current *past* that null
byte. Otherwise, we'll end up in other random memory doing God knows what.

In order to create a token, we have this constructor-like function:

^code make-token

It uses the scanner's `start` and `current` pointers to define the range of
source code containing the token's lexeme. It sets a couple of other obvious
fields, then returns the token.

We have a similar function for returning error tokens:

^code error-token

The key difference is that the lexeme fields point to the error message instead
of pointing into the user's source code string. We need to be careful then to
ensure that the error message string also has a long enough lifetime that it is
still around when the compiler reads it. In practice, we only ever call this
function with C string literals. Those are constant and eternal, so we're fine.

What we have now is basically a working scanner for a language with an empty
lexical grammar. Since the grammar has no productions, every character is an
error. Now we can start filling in the rules.

### Simple tokens

The simplest tokens are only a single character. We can recognize those like so:

^code scan-char (1 before, 2 after)

We read the next character from the source code, and then do a straightforward
switch to see if it's any of Lox's one-character lexemes. If it is, we make a
token of the right type.

To read the next character, we use this helper which consumes the current
character and returns it:

^code advance

Next up are the punctuation tokens that can be one or two characters, like `!`
and `!=`, or `>` and `>=`. When we see a character like `!`, we don't know if
we're in a `!` token or a `!=` until we look at the next character too. We
handle those like so:

^code two-char (1 before, 2 after)

After consuming the first character, we look for an `=`. If found, we consume it
and return the corresponding two-character token. Otherwise, we leave the
current character alone (so it can be part of the next token) and return the
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

This advances the scanner past any leading whitespace. After a call to this
returns, we know the very next character is a meaningful one (or we're at the
end of the source code).

^code skip-whitespace

It's sort of a separate mini-scanner. It loops, consuming every whitespace
character it encounters. However, unlike the main scanner loop, this needs to
*not* consume the current character if it isn't whitespace. To support that, it
uses:

^code peek

<span name="peek">This</span> simply returns the next character waiting to be
consumed, but doesn't consume it.

<aside name="peek">

Another approach some scanners take is to have a function to "unconsume" a
character. Then you would speculatively consume it and undo if it turns out to
not be a whitespace character.

</aside>

We handle most whitespace characters now, but newlines are a little different:

^code newline (1 before, 2 after)

When we consume one of those, we also need to bump the current line number.

### Comments

There's one last kind of program text that isn't meaningful to the interpreter
-- comments. They aren't technically "whitespace", if you want to get all
precise with your terminology, but as far as Lox is concerned, they may as well
be:

^code comment (1 before, 2 after)

Comments start with `//` in Lox, so as with `!=` and friends, we need a second
character of lookahead. However, with `!=`, we still wanted to consume the `!`
even if the `=` wasn't found. Comments are different, if we don't find a second
`/`, we need to not consume the first slash either.

To handle that, we add:

^code peek-next

This is like `peek()` but for one character past the current one. If the current
character and the next one are both `/`, we consume them and then any other
characters until the next newline or the end of the source code.

We use `peek()` to check for the newline but not consume it. That way, the
newline will be the current character on the next turn of the outer loop in
`skipWhitespace()` and we'll recognize it and increment `scanner.line`.

### Literal tokens

Number and string tokens are special because they have an associated runtime
value with them. We'll start with strings because they are easy to recognize --
they always start with a double quote:

^code scan-string (1 before, 2 after)

That calls:

^code string

This is similar to how we did it in jlox. We keep consuming characters until we
reach the closing quote. We also need to track newlines inside the string
literal -- Lox supports multi-line strings. And, of course, we need to
gracefully handle running out of source code before we find the end quote.

The main change here in clox is something that's *not* present. Again, it
relates to memory management. In jlox, the Token class had an Object field to
store the runtime value converted from the literal token's lexeme.

Supporting that in C requires a lot of work. We need some sort of union and type
tag to track where the token contains a string or double value. If it's a
string, we need to manage the memory for the string's character array somehow.

Instead of adding that complexity to the scanner, we will defer <span
name="convert">converting</span> the literal lexeme to a runtime value until
later. In clox, Token will just store the lexeme -- the character sequence
exactly as it appears in the user's source code. Then, in the compiler, we'll
convert that lexeme to a runtime value right when we are ready to store it in
the chunk's constant table.

<aside name="convert">

Doing the lexeme to value conversion in the compiler does introduce a little
redundancy. The work to, say, scan a number literal is *awfully* similar to the
work required to convert a sequence of digit characters to a number. If we had
things like string escape sequences, there would be even more duplicate effort.

But, for our purposes, there isn't that much redundancy, it isn't in anything
performance-critical, and it keeps our scanner simpler.

</aside>

Next up, numbers. Instead of adding a case for each of the ten digits that can
start a number, we handle them separately:

^code scan-number (1 before, 2 after)

That uses this obvious utility function:

^code is-digit

We finish scanning the number using this:

^code number

It's virtually identical to jlox's version except, again, we don't convert the
lexeme to a double yet.

---

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

As usual, after the first letter, we allow digits too, and we keep consuming
alphanumerics until we run out of them. Then we produce a token with the proper
type. "Proper type" is where the fun happens:

^code identifier-type

Here's what it looks like if we have no reserved words at all. Now how should we
go about recognizing keywords? In jlox, we stuff them all in a Java Map and
looked them up by name. We don't have any sort of hash table structure in clox,
at least not yet.

Even if we did, it's overkill if you think about it. To look up a string in a
hash <span name="hash">table</span>, we need to walk the string to calculate
it's hash code, look up the corresponding bucket in the hash table, and then do
a character-by-character equality comparison on any string it happens to find
there.

<aside name="hash">

Don't worry if this is unfamiliar to you. When we get to [building our own hash
table from scratch][hash], we'll learn all about it in minute detail.

[hash]: hash-tables.html

</aside>

Let's say we've scanned the identifier "gorgonzola". How much work should we
need to do to tell if that's a reserved word? Well, no Lox keyword starts with
"g", so looking at the first character is enough to definitively answer "no".
That's a lot simpler than a hash table lookup.

What about "cardigan"? We do have a keyword in Lox that starts with "c":
"class". But the second character in "cardigan", "a", rules that out. What about
"forest"? Since "for" is a keyword, we have to go farther in the string before
we can establish that we don't have a reserved word. But, in most cases, only a
character or two is enough to tell we've got a user-defined name in our hands.

It seems like there should be some way to traverse through the characters in the
lexeme and bail out quickly when we realize the identifier isn't a keyword. At
each character in the lexeme, we make a choice: either we realize it's not a
keyword, realize it is a specific one, or need to look further.

Imagine a tree where each node (except the root) is a character. Each child of
the root is one character that starts a Lox keyword. A child of one of those
nodes in turn has children for second characters that can follow that first one.
If a node is the last letter in a keyword, it's marked with that keyword's token
type.

To recognize a keyword, we take the identifier's lexeme and use it as a path to
guide a traversal through the tree. At each node, we choose the branch that
leads to the next character in the lexeme. If there is no branch for the next
character, it must not be a keyword. If we reach a node marked with a keyword's
token type, and we're at the last character of the lexeme, then we've exactly
matched a keyword.

Lox's keyword tree looks like this:

**todo: illustrate example traversal**

This kind of tree may look familiar. It's part of a family of related classic,
fundamental data structures in computer science. The first is a <span
name="trie">[**trie**][trie]</span>. Like Java's HashSet, a binary search tree,
or even the humble array, a trie stores a collection of items. Specifically, a
trie stores a set of strings. The brilliant part is that it breaks them down to
their atomic components -- individual characters. Nowhere in the trie will you
find a whole string.

[trie]: https://en.wikipedia.org/wiki/Trie

<aside name="trie">

"Trie" is one of the most confusing names in CS. Edward Fredkin yanked it out of
the middle of the word "retrieval", which means it should be pronounced like
"tree". But, uh, there is already a pretty important data structure pronounced
"tree" *which tries are a special case of*, so unless you never speak of these
things out loud, no one can tell which one you're talking about. Thus, people
these days often pronounce it like "try" to try to avoid the headache.

</aside>

Instead, each string in the trie exists as a *path* through a tree of character
nodes. Each child of the root node is a letter that starts one of the strings in
the set. Off those children are nodes for letters that follow those, and so on.
Nodes that represent the last character in a string have a special marker. That
way, if your trie contains, say, "banquet" and "ban", you are able to tell that
it does *not* contain "banque" -- the "e" node won't have that marker, while the
"n" and "t" nodes will.

To see if the trie contains some string, you use the characters in the string to
traverse through the tree. If you successfully reach a marked node, the string
is present. Sounds a lot like the keyword tree I sketched out above, right?

Tries are actually a special case of an even more fundamental data structure, a
[**deterministic finite automaton**][dfa] (DFA). You might also know these by
another name: **"finite state machine"**, or just **"state machine"**. State
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
long strings. For example, here's a DFA that recognizes integers:

**todo: illustrate**

In fact, we could one big giant DFA that does *all* of the lexical analysis for
Lox. A single state machine that recognizes and spits out all of the tokens we
need. However, crafting one of those by hand would be pretty challenging. That's
why Lex was created. If you give it a nice textual description of your lexical
grammar -- a bunch of <span name="regex">regular</span> expressions -- it
automatically generates a DFA for you and produces a pile of C code that
implements it.

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
that tree to code?

The absolute simplest <span name="v8">solution</span> is to use a switch
statement for each node with cases for each branch. We'll start with the root
node and handle the easy keywords:

<aside name="v8">

Don't look down on my rudimentary code here. The same approach is [essentially
what V8 does][v8], and that's literally one of the world's most sophisticated,
fastest language implementations.

[v8]: https://github.com/v8/v8/blob/e77eebfe3b747fb315bd3baad09bec0953e53e68/src/parsing/scanner.cc#L1643

</aside>

^code keywords (1 before, 1 after)

These are the letters that only start a single keyword. If we see an "s", the
only keyword the identifier could possibly be is "super". It might not be,
though, so we still need to check the rest of the letters too. In the tree
diagram, this is basically that unbranching path leading off the "s".

We won't roll a switch for each of those nodes. Instead, here's a utility
function to test the rest of a potential keyword's lexeme:

^code check-keyword

Once we've found a prefix that could only be one possible reserved word, we
still need to verify two things. The lexeme must be exactly as long as the
keyword. If the first letter is "s", the lexeme could still be "sup" or
"superb". And the remaining characters must match exactly -- "supar" isn't good
enough.

If we do have the right number of characters, and they're the ones we want, then
it's a keyword, and we return the associated token type. Otherwise, it must be a
regular identifier.

We have a couple of keywords where the tree branches again after the first
letter. If the lexeme starts with "f", it could still be "false", "for", or
"fun". So we need another switch for the node reached from "f":

^code keyword-f (1 before, 1 after)

We also need to check that there even *is* a second letter. "f" by itself is a
valid identifier too, after all. The other letter like this is "t":

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
writing the simplest code I can is more than sufficient to accomplish that.

</aside>

<div class="challenges">

## Challenges

1.  Many newer languages support [*string interpolation*][interp]. Inside a
    string literal, you have some sort of special delimiters -- most commonly
    `${` at the beginning and `}` at the end. Inside those, any expression can
    appear. When the string literal is executed, the inner expressions are
    evaluated, converted to strings, and then merged with the surrounding string
    literal.

    For example, if Lox supported string interpolation, then this:

        :::lox
        var steep = 4;
        var cool = 2;
        print "Tea will be ready in ${steep + cool} minutes.";

    Would print:

        Tea will be ready in 6 minutes.

    What token types would you define to support scanning strings containing
    interpolation? What sequence of tokens would you emit for the above string
    literal?

    What tokens would you emit for:

        :::text
        "Nested ${"interpolation?! Are you ${"mad?!"}"}"

    Consider looking at other language implementations that support this to see
    how they handle it.

2.  Several languages use angle brackets for generics and also have a `>>` right
    shift operator. This lead to a classic problem in earlier versions of C++:

        :::c++
        vector<vector<string>> nestedVectors;

    This would produce a compile error because the `>>` was lexed to a single
    right shift token, not two `>` tokens. Instead, you needed to put a space
    between the closing angle brackets.. Later versions of C++ fix this. Java
    and C# never had the problem.

    How do those languages specify and implement this?

3.  Many language, especially later in their evolution, define "contextual
    keywords". These are identifiers that act like reserved words in some
    contexts but can be normal user-defined identifiers in others.

    For example, `await` is a keyword inside an `async` method in C#, but
    in other methods, you can use `await` as your own identifier.

    What are the pros and cons of having contextual keywords? How would you
    implement them in your language's front end if you needed to?

[interp]: https://en.wikipedia.org/wiki/String_interpolation

</div>
