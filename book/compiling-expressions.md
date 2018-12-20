^title Compiling Expressions
^part A Bytecode Virtual Machine

> In the middle of the journey of our life I found myself within a dark woods
> where the straight way was lost.
> <cite>Dante Alighieri</cite>

This chapter is exciting for not one, not two, but *three* reasons. First, it
provides the final segment of our VM's execution pipeline. Once in place, we can
plumb the user's source code from scanning all the way through to executing it.

<img src="image/compiling-expressions/pipeline.png" alt="Lowering the 'compiler' section of pipe between 'scanner' and 'VM'."/>

Second, we get to write an actual, honest-to-God *compiler*. It parses source
code and outputs a low-level series of binary instructions. Sure, it's <span
name="wirth">bytecode</span> and not some chip's native instruction set, but
it's way closer to the metal than jlox was. We're about to be real language
hackers.

<aside name="wirth">

Bytecode was good enough for Niklaus Wirth, and no one questions his street
cred.

</aside>

<span name="pratt">Third</span> and finally, I get to show you one of my
absolute favorite algorithms: Vaughan Pratt's "top-down operator precedence
parsing". It's the most elegant way I know to parse expressions. It gracefully
handles prefix operators, postfix, infix, *mixfix*, any kind of *-fix* you got.
It deals with precedence and associativity without breaking a sweat. I love it.

<aside name="pratt">

Pratt parsers are a sort of oral tradition in industry. No compiler or language
book I've read teaches them. Academia is very focused on generated parsers, and
Pratt's technique is for hand-written ones, so it gets overlooked.

But in production compilers, where hand-rolled parsers are common, you'd be
surprised how many people know it. Ask where they learned it, and it's always,
"Oh, I worked on this compiler years ago and my coworker said they took it from
this old front end..."

</aside>

As usual, before we get to the fun stuff, we've got some preliminaries to work
through. You have to eat your vegetables before you get dessert. First, let's
ditch that temporary scaffolding we wrote for testing the scanner and replace it
with something more useful:

^code interpret-chunk (1 before, 1 after)

We create a new empty chunk and pass it over to the compiler. The compiler will
take the user's program and fill up the chunk with bytecode. At least, that's
what it will do if the program doesn't have any compile errors. If it does
encounter an error, `compile()` returns `false` and we discard the unusable
chunk.

Otherwise, we send the completed chunk over to the VM to be executed. When the
VM finishes, we free the chunk and we're done. As you can see, the signature to
`compile()` is different now:

^code compile-h (2 before, 2 after)

We pass in the chunk where the compiler will write the code, and then
`compile()` returns whether or not compilation succeeded. Over in the
implementation...

^code compile-signature (1 before, 1 after)

That call to `initScanner()` is the only line that survives this chapter. Rip
out the temporary code we wrote to test the scanner and replace it with these
three lines:

^code compile-chunk (1 before, 1 after)

The call to `advance()` "primes the pump" on the scanner. We'll see what it does
soon. Then we parse a single expression. We aren't going to do statements yet,
so that's the only subset of the grammar we support. We'll revisit this when we
[add statements in a few chapters][globals]. After we compile the expression, we
should be at the end of the source code, so we check for the sentinel EOF token.

[globals]: global-variables.html

We're going to spend the rest of the chapter making this function work,
especially that little `expression()` call. Normally, we'd dive right into that
function definition and work our way through the implementation from top to
bottom.

This chapter is <span name="blog">different</span>. Pratt's parsing technique is
remarkably simple once you have it all loaded in your head, but it's a little
tricky to break into bite-sized pieces. It's recursive, of course, which is part
of the problem. But it also relies on a big table of data. As we build up the
algorithm, that table grows additional columns.

<aside name="blog">

If this chapter isn't clicking with you and you'd like another take on the
concepts, I wrote an article that teaches the same algorithm but using Java and
an object-oriented style: [Pratt Parsing: Expression Parsing Made Easy][blog].

[blog]: http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/

</aside>

I don't want to revisit 40-something lines of code each time we extend the
table. So we're going to work our way into the core of the parser from the
outside and cover all of the surrounding bits before we get to the juicy center.
This will require a little more patience and mental scratch space than most
chapters, but it's the best I could do.

## Single-Pass Compilation

A compiler has roughly two jobs. It parses the user's source code to understand
what it means. Then it takes that knowledge and outputs low-level instructions
that produce the same semantics. Many languages split those two roles into two
separate <span name="passes">passes</span> in the implementation. A parser
produces an AST -- just like jlox does -- and then a "code generator" traverses
the AST and outputs target code.

<aside name="passes">

In fact, most sophisticated optimizing compilers have a heck of a lot more than
two passes. Determining not just *what* optimization passes to have, but how to
order them to squeeze the most performance out of the compiler -- since the
optimizations often interact in complex ways -- is somewhere between an "open
area of research" and a "dark art".

</aside>

In clox, we're taking an old school approach and merging these two passes into
one. Back in the day, language hackers did this because computers literally
didn't have enough memory to store an entire source file's AST. We're doing it
because it keeps our compiler simpler, which is a real asset when programming in
C.

"Single-pass compilers" like we're going to build don't work well for all
languages. Since the compiler only has a peephole view into the user's program
while generating code, the language must be designed such that you don't need
much surrounding context to understand a piece of syntax. Fortunately, tiny,
dynamically-typed Lox is <span name="lox">well-suited</span> to that.

<aside name="lox">

Not that this should come as much of a surprise. I did design the language
specifically for this book after all.

<img src="image/compiling-expressions/keyhole.png" alt="Peering through a keyhole at 'var x;'"/>

</aside>

What this means in practical terms is that our "compiler" C module has
functionality you'll recognize from jlox for parsing -- consuming tokens,
matching expected token types, etc. And it also has functions for code gen --
emitting bytecode and adding constants to the destination chunk. (And it means
I'll use "parsing" and "compiling" interchangeably throughout this and later
chapters.)

We'll build those two ends up first. Then we'll stitch them together with the
code in the middle that uses Pratt's technique to parse Lox's particular grammar
and output the right bytecode.

## Parsing Tokens

First up, the front half of the compiler. This function's name should sound
familiar:

^code advance

Just like in jlox, it steps forward through the token stream. It asks the
scanner for the next token and stores it for later use. Before doing that, it
takes the old current token and stashes that in another field. That will come in
handy later so that we can get at the lexeme after we match a token.

The code to read the next token is wrapped in a loop. Remember, clox's scanner
doesn't report lexical errors. Instead, it creates special "error tokens" and
leaves it up to the parser to report them. We do that here.

We keep looping, reading tokens and reporting the errors, until we hit a
non-error one or reach the end. That way, the rest of the parser only sees
"real" tokens. The current and previous token are stored in this struct:

^code parser (1 before, 2 after)

Like we did in other modules, we have a single global variable of this struct
type so we don't need to pass the state around from function to function in the
compiler.

### Handling syntax errors

If the scanner hands us an error token, we need to actually tell the user. That
happens using this:

^code error-at-current

It pulls the location out of the current token in order to tell the user where
the error occurred and forwards to `errorAt()`. More often, we'll report an
error at the location of the token we just consumed, so we give the shorter name
to this other function:

^code error

The actual work happens here:

^code error-at

First, we print where the error occurred. We try to show the lexeme if it's
human-readable. Then we print the error message itself. After that, we set this
`hadError` flag. That records whether any errors occurred during compilation.
It also lives in the parser struct:

^code had-error-field (1 before, 1 after)

Earlier I said that `compile()` should return `false` if an error occurred. Now
we can make it do that:

^code return-had-error (1 before, 1 after)

I've got another flag for error handling to introduce. We want to avoid error
cascades. If the user has a mistake in their code and the parser gets confused
about where it is in the grammar, we don't want it to spew out a whole pile of
meaningless knock-on errors after the first one.

We fixed that in jlox using panic mode error recovery. In the Java interpreter,
we threw an exception to unwind out of all of the parser code to a point where
we could skip tokens and resynchronize. We don't have <span
name="setjmp">exceptions</span> in C. Instead, we'll do a little smoke and
mirrors. We add a flag to track whether we're currently in panic mode:

<aside name="setjmp">

There is `setjmp()` and `longjmp()`, but I'd rather not go there. Those make it
too easy to leak memory, forget to maintain invariants, or otherwise have a Very
Bad Day.

</aside>

^code panic-mode-field (1 before, 1 after)

When an error occurs, we set it:

^code set-panic-mode (1 before, 1 after)

After that, we go ahead and keep compiling as normal as if the error never
occurred. The bytecode will never get executed, so it's harmless to keep on
trucking. The trick is that while the panic mode flag is set, we simply suppress
any other errors that get detected:

^code check-panic-mode (1 before, 1 after)

There's a good chance the parser will go off in the weeds, but the user won't
know because the errors all get swallowed. Panic mode ends when the parser
reaches a synchronization point. For Lox, we chose statement boundaries, so when
we later add those to our compiler, we'll clear the flag.

These new fields need to be initialized:

^code init-parser-error (1 before, 1 after)

And to display the errors, we need a standard header:

^code compiler-include-stdlib (1 before, 2 after)

There's one last parsing function, another old friend from jlox:

^code consume

It's similar to `advance()` in that it reads the next token. But it also
validates that the token has an expected type. If not, it reports an error. This
function is the foundation of most syntax errors in the compiler.

OK, that's enough on the front end for now.

## Emitting Bytecode

After we parse and understand a piece of the user's program, the next step is to
translate that to a series of bytecode instructions. It doesn't get more
fundamental than appending a single byte to the chunk:

^code emit-byte

It's hard to believe great things will flow through such a simple function. It
writes the given byte, which may be an opcode or an operand to an instruction.
It sends in the previous token's line information so that runtime errors are
associated with that line.

The chunk that we're writing gets passed into `compile()`, but it needs to make
its way to `emitByte()`. To do that, we rely on this intermediary function:

^code compiling-chunk (2 before, 1 after)

Right now, the chunk pointer is stored in a module level variable like we store
other global state. Later, when we start compiling user-defined functions, the
notion of "current chunk" gets more complicated. To avoid having to go back and
change a lot of code, I encapsulate that logic in this function.

We initialize this new module variable before we write any bytecode:

^code init-compile-chunk (2 before, 2 after)

Then, at the very end, when we're done compiling the chunk, we wrap things up:

^code finish-compile (1 before, 1 after)

That calls this:

^code end-compiler

In this chapter, our VM only deals with expressions. When you run clox, it
parses, compiles, and executes a single expression, then prints the result. To
print that value, we are temporarily using the `OP_RETURN` instruction. So we
have the compiler add one of those to the end of the chunk:

^code emit-return

While we're over here in the back end:

^code emit-bytes

Over time, we'll have enough cases where we need to write an opcode followed by
a one-byte operand that it's worth defining this convenience function.

## Parsing Prefix Expressions

We've assembled our parsing and code generation functions. The missing piece is
the code in the middle that connects those together.

<img src="image/compiling-expressions/mystery.png" alt="Parsing functions on the left, bytecode emitting functions on the right. What goes in the middle?"/>

The only step in `compile()` that we have left to implement is this function:

^code expression

We aren't ready to implement every kind of expression in Lox yet. Heck, we don't
even have Booleans. For this chapter, we're only going to worry about:

* Number literals: `123`.
* Parentheses for grouping: `(123)`.
* Unary negation: `-123`.
* The Four Horsemen of the Arithmetic: `+`, `-`, `*`, `/`.

As we work through the functions to compile each of those kinds of expressions,
we'll also assemble the requirements for the table-driven parser that calls
them.

### Parsers for tokens

Imagine that every expression in Lox is only a single token. Each token type
maps to a different kind of expression. We define a function for each that
outputs the appropriate bytecode for that expression. Then we build an array of
function pointers. The indexes in the array correspond to the `TokenType` enum
values, and the function at that index is the code to compile an expression of
that token type.

To add support for number literals, we store a pointer to the following function
at the `TOKEN_NUMBER` index in the array:

^code number

We assume the token for the number literal has already been consumed and is
stored in `previous`. We take that lexeme and use the C standard library to
convert it to a double value. Then we generate the code to load that value using
this function:

^code emit-constant

First, we add the value to the constant table, then we emit an `OP_CONSTANT`
instruction that pushes it onto the stack at runtime. To insert an entry in the
constant table, we rely on:

^code make-constant

Most of the work happens in `addConstant()`, which we defined back in an
[earlier chapter][bytecode]. That adds the given value to the end of the chunk's
constant table and returns its index. The new function's job is mostly to make
sure we don't have too many constants. Since the `OP_CONSTANT` instruction uses
a single byte for the index operand, we can only store and load up to <span
name="256">256</span> constants in a chunk.

[bytecode]: chunks-of-bytecode.html

<aside name="256">

Yes, that limit is pretty low. If this were a full-sized language
implementation, we'd want to add another instruction like `OP_CONSTANT_16` that
stores the index as a two-byte operand so we can handle more constants when
needed.

The code to support that isn't particularly illuminating, so I omitted it from
clox, but you'll want to scale to larger programs in your own VMs.

</aside>

That's basically all it takes. Provided there is some suitable code that
consumes a `TOKEN_NUMBER` token and then calls `number()`, we can now compile
number literals to bytecode.

### Parentheses for grouping

Our array of parsing function pointers would be great if every expression was
only a single token long. Alas, most are longer. However, many expressions
*start* with a particular token. We call these *prefix* expressions. For
example, when we're parsing an expression and the current token is `(`, we know
we must be looking at a parenthesized grouping expression.

It turns out our function pointer array handles those too. The parsing function
for an expression type can consume any additional tokens that it wants to, just
like in a regular recursive descent parser. Here's how parentheses work:

^code grouping

Again, it assumes the initial `(` has already been consumed. It <span
name="recursive">recursively</span> calls back into `expression()` to compile
the expression between the parentheses, then it parses the closing `)` at the
end.

<aside name="recursive">

A Pratt parser isn't a recursive *descent* parser, but it's still recursive.
That's to be expected since the grammar itself is recursive.

</aside>

As far as the back end is concerned, there's literally nothing to a grouping
expression. Its sole function is syntactic -- it lets you insert a lower
precedence expression where a higher precedence is expected. Thus, it has no
runtime semantics on its own and therefore doesn't emit any bytecode. The inner
call to `expression()` takes care of generating bytecode for the expression
inside the parentheses.

### Unary negation

Unary minus is also a prefix expression, so it works with our model too:

^code unary

The leading `-` token has been consumed and is sitting in `parser.previous`. We
grab the token type from that to note which unary operator we're dealing with.
It's unnecessary right now, but this will make more sense when we use this same
function to compile the `!` operator in [the next chapter][next].

[next]: types-of-values.html

As in `grouping()`, we recursively call `expression()` to compile the operand.
After that, we emit the bytecode to perform the negation. It might seem a little
weird to write the negate instruction *after* its operand's bytecode since the
`-` appears on the left, but think about in terms of order of execution:

1. We evaluate the operand first which leaves its value on the stack.

2. Then we pop that value, negate it, and push the result.

So the `OP_NEGATE` instruction should be emitted <span name="line">last</span>.
This is part of the compiler's job -- parsing the program in the order it
appears in the source code and rearranging it into the order that execution
happens.

<aside name="line">

Emitting the `OP_NEGATE` instruction after the operands does mean that the
current token when the bytecode is written is *not* the `-` token. That mostly
doesn't matter, except that we use that token for the line number to associate
with that instruction.

This means if you have a multi-line negation expression, like:

```lox
print -
  true;
```

Then the runtime error will be reported on the wrong line. Here, it would show
the error on line 2, even though the `-` is on line 1. A more robust approach
would be to store the token's line before compiling the operand and then pass
that into `emitByte()`, but I wanted to keep things simple for the book.

</aside>

There is one problem with this code, though. By calling `expression()`, we allow
any expression for the operand, regardless of precedence. Once we add binary
operators and other syntax, that will do the wrong thing. Consider:

```lox
-a.b + c;
```

Here, the operand to `-` should be just the `a.b` expression, not the entire
`a.b + c`. But if `unary()` calls `expression()`, the latter will happily chew
through all of the remaining code including the `+`.

The operand to `-` is an expression, but it needs to only allow expressions at a
certain precedence level or higher. In jlox's recursive descent parser, each
method for parsing a specific expression also parsed any expressions of higher
precedence too. By calling the parsing method for the expression type of the
lowest allowed precedence, we'd roll in the others as well.

The parsing functions here are different. Each only parses exactly one type of
expression. They don't cascade to include higher precedence expression types
too. We need a different solution. For now, let's assume that we have a
function that parses any expression of a given precedence level or higher:

^code parse-precedence

We will circle back and see *how* that function does what it does. Right now,
it's magic. In order to take the "precedence" as a parameter, we define it
numerically:

^code precedence (1 before, 2 after)

These are all of Lox's precedence levels in order from lowest to highest. Since
C implicitly gives successively larger numbers for enums, this means that
`PREC_CALL` is numerically larger than `PREC_UNARY`. With this function in hand,
it's a snap to fill in the missing body for `expression()`:

^code expression-body (1 before, 1 after)

It simply parses the lowest precedence level, which subsumes all of the higher
precedence expressions too.

Now, to compile the operand for a unary expression, we call this new function
and limit it to the appropriate level:

^code unary-operand (1 before, 2 after)

We use the unary operator's own `PREC_UNARY` precedence to permit <span
name="useful">nested</span> unary expressions like `!!doubleNegative`. Since
unary operators have pretty high precedence, that correctly excludes things like
binary operators. When we call `unary()` to parse:

<aside name="useful">

Not that nesting unary expressions is particularly useful in Lox. But other
languages let you do it, so we do too.

</aside>

```lox
-a.b + c;
```

It consumes the `a.b` and then stops there. Then `unary()` returns and relies on
some other code to handle parsing the surrounding `+` expression that contains
the negation on its left. Which brings us to...

## Parsing Infix Expressions

We're down to the binary operators. These are different from the previous
expressions because they are *infix*. With the other expressions, we know what
we are parsing from its very first token. With infix expressions, we don't even
know we're in the middle of a binary operator until *after* we've parsed its
left operand and then stumbled onto the operator token in the middle.

Here's an example:

```lox
-1 + 3
```

Let's walk through trying to compile it with what we have so far:

1.  We call `expression()`. It sees the first token is `-`. The associated parse
    function is `unary()`, so it calls that.

3.  `unary()` calls `parsePrecedence()` to parse the operand. That compiles the
    `1` literal. It does not compile the `+`, because that's too low precedence
    for the call to `parsePrecedence()`.

4.  `unary()` returns back to `expression()`.

Now what? It turns out we are right where we need to be. Now that we've compiled
the prefix expression, the next token is `+`. That's the token we need to detect
that we're in the middle of an infix expression and to realize that the prefix
expression we already compiled is actually an operand to that.

So we turn that array of function pointers into a *table*. One column associates
*prefix* parser functions with token types. The new column associates *infix*
parser functions with token types. In the rows for `TOKEN_PLUS`, `TOKEN_MINUS`,
`TOKEN_STAR`, and `TOKEN_SLASH`, we point to this function:

^code binary

When a prefix parser function is called, the leading token has already been
consumed. An infix parser function is even more *in media res* -- the left-hand
operand has already been compiled and the subsequent infix operator consumed.

The fact that the left operand gets compiled first works out fine. It means at
runtime, that code gets executed first. When it runs, the value it produces will
end up on the stack. That's right where the infix operator is going to need it.

Then we come here to `binary()` to handle the rest of the arithmetic operator.
It compiles the right operand, much like how `unary()` compiles its own trailing
operand. Finally, it emits the bytecode instruction that performs the binary
operation.

When run, the VM will execute the left and right operand code, in that order,
leaving their values on the stack. Then it executes the instruction for the
operator. That pops the two values, computes the operation, and pushes the
result.

The code that probably caught your eye here is that `getRule()` line. When we
parse the right-hand operand, we again need to worry about precedence. Take an
expression like:

```lox
2 * 3 + 4
```

When we parse the right operand of the `*` expression, we need to just capture
`3`, and not `3 + 4`, because `+` is lower precedence than `*`. We could define
a separate function for each binary operator. Each would call
`parsePrecedence()` and pass in the correct precedence level for its operand.

But that's kind of tedious. Each binary operator's right-hand operand precedence
is one level <span name="higher">higher</span> than its own. We can look that up
dynamically with this `getRule()` thing we'll get to soon. Using that, we call
`parsePrecedence()` with one level higher than this operator's level.

<aside name="higher">

We use one *higher* level of precedence for the right operand because the binary
operators are left-associative. Given a series of the *same* operator, like:

```lox
1 + 2 + 3 + 4
```

We want to parse it like:

```lox
((1 + 2) + 3) + 4
```

Thus, when parsing the right-hand operand to the first `+`, we want to consume
the `2`, but not the rest, so we use one level above `+`&rsquo;s precedence. But
if our operator was *right*-associative, this would be wrong. Given:

```lox
a = b = c = d
```

Since assignment is right-associative, we want to parse it as:

```lox
a = (b = (c = d))
```

To enable that, we would call `parsePrecedence()` with the *same* precedence as
the current operator.

</aside>

This way, we can use a single `binary()` function for all binary arithmetic
operators even though they have different precedences.

## A Pratt Parser

We now have all of the pieces and parts of the compiler laid out. We have a
function for each grammar production: `number()`, `grouping()`, `unary()`, and
`binary()`. We still need to implement `parsePrecedence()`, and `getRule()`. We
also know we need some table that, given a token type, lets us find:

*   The function to compile a prefix expression starting with a token of that
    type.

*   The function to compile an infix expression whose left operand is followed
    by a token of that type.

*   The precedence of an <span name="prefix">infix</span> expression that uses
    that token as an operator.

<aside name="prefix">

We don't need to track the precedence of the *prefix* expression starting with a
given token because all prefix operators in Lox have the same precedence.

</aside>

We wrap these three properties in a little struct which represents a single row
in the parser table:

^code parse-rule (1 before, 2 after)

That ParseFn type is a simple <span name="typedef">typedef</span> for a function
type that takes no arguments and returns nothing:

<aside name="typedef">

C's syntax for function pointer types is so bad that I always hide it behind a
typedef. I understand the intent behind the syntax -- the whole "declaration
reflects use" thing -- but I think it was a failed syntactic experiment.

</aside>

^code parse-fn-type (1 before, 2 after)

The table that drives our whole parser is an array of ParseRules. We've been
talking about it forever, and finally you get to see it:

^code rules

<aside name="big">

See what I mean about not wanting to revisit the table each time we needed a new
column? It's a beast.

</aside>

There are a lot of `NULL` and `PREC_NONE` values in here. Most of those are
because there is no expression associated with those tokens. You can't start an
expression with, say, `else`, and `}` would make for a pretty confusing infix
operator.

But, also, we haven't filled in the entire grammar yet. In later chapters, as we
add new expression types, some of these slots will get functions in them. One of
the things I like about this approach to parsing is that it makes it very easy
to see which tokens are in use by the grammar and which are available.

Now that we have the table, we are finally ready to write the code that uses it.
This is where our Pratt parser comes to life. The easiest function to define is
`getRule()`:

^code get-rule

It simply returns the rule at the given index. It's called by `binary()` to look
up the precedence of the current operator. This function exists solely to handle
a declaration cycle in the C code. `binary()` is defined *before* the rules
table so that the table can store a pointer to it. That means the body of
`binary()` cannot access the table directly.

Instead, we wrap the lookup in a function. That lets us forward declare
`getRule()` before the definition of `binary()`, and <span
name="forward">then</span> *define* `getRule()` after the table. We'll need a
couple of other forward declarations to handle the fact that our grammar is
recursive, so let's get them all out of the way:

<aside name="forward">

This is what happens when you write your VM in a language that was designed to
be compiled on a PDP-11.

</aside>

^code forward-declarations (2 before, 1 after)

If you're following along and implementing clox yourself, pay close attention to
the little annotations that tell you where to put these code snippets. Don't
worry, though, if you get it wrong, the C compiler will be happy to tell you.

### Parsing with precedence

Now we're getting to the fun stuff. The maestro that orchestrates all of the
parsing functions we've defined is `parsePrecedence()`. Let's start with parsing
prefix expressions:

^code precedence-body (1 before, 1 after)

It reads the next token and looks up the corresponding ParseRule. If there is no
prefix parser then the token must be a syntax error. We report that and return
to the caller.

Otherwise, we call that prefix parse function and let it do its thing. That
prefix parser compiles the rest of the prefix expression, consumes any other
tokens it needs, and returns back here. Infix expressions are where it gets
challenging, since precedence comes into play. The implementation is remarkably
simple:

^code infix (1 before, 1 after)

That's the whole thing. Really. Here's how the entire function works: At the
beginning of `parsePrecedence()`, we look up a prefix parser for the current
token. The first token is *always* going to belong to some kind of prefix
expression, by definition. It may turn out to be nested as an operand inside one
or more infix expressions, but as you read the code from the left to right, the
first thing you hit is always some prefix expression.

After parsing that, which may consume more tokens, the prefix expression is
done. Now we look for an infix parser for the next token. If we find one, it
means the prefix expression we already compiled might be an operand for it. But
only if the call to `parsePrecedence()` has a `precedence` that is low enough to
permit that infix operator.

If the next token is too low precedence, or isn't an infix operator at all,
we're done. We've parsed as much expression as we can. Otherwise, we consume the
operator and hand off control to the infix parser we found. It consumes whatever
other tokens it needs (usually the right operand) and returns back to
`parsePrecedence()`. Then we loop back around and see if the *next* token is
also a valid infix operator that can take the entire preceding expression as its
operand.

We keep looping like that, crunching through infix operators and their operands
until we hit a token that isn't an infix operator or is too low precedence. This
function is fairly short, but kind of tricky since each of those prefix and
infix parsers often calls back into `parsePrecedence()` for its operands.

That's a lot of prose, but if you really want to mind meld with Vaughan Pratt
and fully understand the algorithm, step through the parser in your debugger as
it works through some expressions. Maybe a picture will help. There's only a
handful of functions, but they are marvelously intertwined:

<span name="connections"></span>

<img src="image/compiling-expressions/connections.png" alt="The various parsing functions and how they call each other."/>

<aside name="connections">

The <img src="image/compiling-expressions/calls.png" class="arrow" /> arrow
connects a function to another function it directly calls. The <img
src="image/compiling-expressions/points-to.png" class="arrow" /> arrow shows the
table's pointers to the parsing functions.

</aside>

We'll need to tweak the code in this chapter later to handle assignment. But,
otherwise, what we wrote covers all of our expression compiling needs for the
rest of the book. We'll plug additional parsing functions into the table when we
add new kinds of expressions, but `parsePrecedence()` is complete.

## Dumping Chunks

While we're here in the core of our compiler, we should put in some
instrumentation. To help debug the generated bytecode, we'll add support for
dumping the chunk once the compiler finishes. We had some temporary logging
earlier when we hand-authored the chunk. Now we'll put in some real code so that
we can enable it whenever we want.

Since this isn't for end users, we hide it behind a flag:

^code define-debug-print-code (2 before, 1 after)

When that flag is defined, we use our existing "debug" module to print out the
chunk's bytecode:

^code dump-chunk (1 before, 1 after)

We only do this if the code was free of errors. After a syntax error, the
compiler keeps on going but it's in kind of a weird state and might produce
broken code. That's harmless because it won't get executed, but we'll just
confuse ourselves if we try to read it.

Finally, to access `disassembleChunk()`, we need to include its header:

^code include-debug (1 before, 2 after)

We made it! This was the last major section to install in our VM's compilation
and execution pipeline. Our interpreter doesn't *look* like much, but inside it
is scanning, parsing, compiling to bytecode, and executing it.

Fire up the VM and type in an expression. If we did everything right, it should
calculate and print the result. We now have a very over-engineered arithmetic
calculator. We have a lot of language features to add in the coming chapters,
but the foundation is in place.

<div class="challenges">

## Challenges

1.  To really understand the parser, you need to see how execution threads
    through the interesting parsing functions -- `parsePrecedence()` and the
    parser functions stored in the table. Take this (strange) expression:

        :::lox
        (-1 + 2) * 3 - -4

    Write a trace of how those functions are called. Show the order they are
    called, which calls which, and the arguments passed to them.

2.  The ParseRule row for `TOKEN_MINUS` has both prefix and infix function
    pointers. That's because `-` is both a prefix operator (unary negation) and
    an infix one (subtraction).

    In the full Lox language, what other tokens can be used in both prefix and
    infix positions? What about in C or another language of your choice?

3.  You might be wondering about more complex "mixfix" expressions that have
    more than two operands separated by tokens. C's conditional or "ternary"
    operator, `? :` is a widely-known one.

    Add support for that operator to the compiler. You don't have to generate
    any bytecode, just show how you would hook it up to the parser and handle
    the operands.

</div>

<div class="design-note">

## Design Note: It's Just Parsing

I'm going to make a claim here that will be unpopular with some compiler and
language people. It's OK if you don't agree. Personally, I learn more from
strongly-stated opinions that I disagree with than I do from several pages of
qualifiers and equivocation. My claim is that *parsing doesn't matter.*

Over the years, many programming language people, especially in academia, have
gotten *really* into parsers and taken them very seriously. Initially, it was
the compiler folks who got into <span name="yacc">compiler-compilers</span>,
LALR and other stuff like that. The first half of the Dragon book is a long love
letter to the wonders of parser generators.

<aside name="yacc">

All of us suffer from the vice of "when all you have is a hammer, everything
looks like a nail", but perhaps none so visibly as compiler people. You wouldn't
believe the breadth of software problems that miraculously seem to require a new
little language in their solution as soon as you ask a compiler hacker for help.

Yacc and other compiler-compilers are the most delightfully recursive example.
"Wow, writing compilers is a chore. I know, let's write a compiler to write our
compiler for us."

For the record, I don't claim immunity to this affliction.

</aside>

Later, the functional programming folks got into parser combinators, packrat
parsers and other sorts of things. Because, obviously, if you give a functional
programmer a problem, the first thing they'll do is whip out a pocketful of
higher-order functions.

Over in math and algorithm analysis land, there is a long legacy of research
into proving time and memory usage for various parsing techniques, transforming
parsing problems into other problems and back, and assigning complexity classes
to different grammars.

At one level, this stuff is important. If you're implementing a language, you
want some assurance that your parser won't go exponential and take 7,000 years
to parse a weird edge case in the grammar. Parser theory gives you that bound.
As an intellectual exercise, learning about parsing techniques is also fun and
rewarding.

But if your goal is just to implement a language and get it in front of users,
almost all of that stuff doesn't matter. It's really easy to get worked up by
the enthusiasm of the people who *are* into it and think that your front end
*needs* some whiz-bang generated combinator parser factory thing. I've seen
people burn tons of time writing and rewriting their parser using whatever
today's hot library or technique is.

That's time that doesn't add any value to your user's life. If you're just
trying to get your parser done, pick one of the bog-standard techniques, use it,
and move on. Recursive descent, Pratt parsing, and one of the popular parser
generators like ANTLR or Bison are all fine.

Take the extra time you saved not rewriting your parsing code and spend it
improving the compile error messages your compiler shows users. Good error
handling and reporting is more valuable to users than almost anything else you
can put time into in the front end.

</div>
