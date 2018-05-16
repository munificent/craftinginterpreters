## 1

I've implemented this in another language, Wren. You can see the code here:

https://github.com/munificent/wren/blob/8fae8e4f1e490888e2cc9b2ea6b8e0d0ff9dd60f/src/vm/wren_compiler.c#L118-L130

Poke around in that file for "interp" to see everything. The basic idea is you
have two token types. TOKEN_STRING is for uninterpolated string literals, and
the last segment of an interpolated string. Every piece of a string literal that
precedes an interpolated expression uses a different TOKEN_INTERPOLATION type.

This:

```lox
"Tea will be ready in ${steep + cool} minutes."
```

Gets scanned like:

```text
TOKEN_INTERPOLATION "Tea will be ready in"
TOKEN_IDENTIFIER    "steep"
TOKEN_PLUS          "+"
TOKEN_IDENTIFIER    "cool"
TOKEN_STRING        "minutes."
```

(The interpolation delimiters themselves are discarded.)

And this:

```lox
"Nested ${"interpolation?! Are you ${"mad?!"}"}"
```

Scans as:

```text
TOKEN_INTERPOLATION "Nested "
TOKEN_INTERPOLATION "interpolation?! Are you "
TOKEN_STRING        "mad?!"
TOKEN_STRING        ""
TOKEN_STRING        ""
```

The two empty TOKEN_STRING tokens are because the interpolation appears at the
very end of the string. They tell the parser that they've reached the end of
the interpolated expression.

## 2

As far as I can tell, Java and C# don't actually specify it correctly. Unless
the verbiage is hidden away somewhere in the specs, I believe that this:

```java
List<List<String>> nestedList;
```

Should technically by a syntax error in a fully spec-compliant implementation
of Java or C#. However, all practical implementations don't follow the letter
of the spec and instead to what users want.

C++, as of C++0x, does actually specify this:

http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1757.html

It states that if a `<` has been scanned and no closing `>` has been scanned
yet, and there are no other intervening bracket characters, then a subsequent
`>>` is scanned as two `>` tokens instead of a single shift.

As far as implementation, I think javac handles this by scanning the `>>` as a
single shift token. When the parser is looking for a `>` to close the type
argument, if it sees a shift token, it splits it into two `>` tokens right then,
consumes the first, and then keeps parsing.

Microsoft's C# parser takes the opposite approach. It always scans `>>` as two
separate `>` tokens. Then, when parsing an expression, if it sees two `>` tokens
next to each other with no whitespace between them, it parses them as a shift
operator.

## 3

I don't generally like contextual keywords. It's fairly easy to write a real
parser that can handle them gracefully, but:

*   Users are often confused by them. Many programmers don't even realize that
    contextual keywords exist. They assume all identifiers are either fully
    reserved by the language or fully available for use.

*   Once an identifier becomes a keyword in some context, it quickly takes on
    that meaning to readers and becomes *very* confusing if you use it for your
    own name outside of that context. Now that C# has async/await, you will
    just anger your fellow C# users if you name a variable `await` in some
    non-async method because they are so used to seeing `await` used for its
    keyword meaning.

    So even though it's *technically* usable elsewhere, it's effectively fully
    reserved.

That being said, sometimes you have to do them. Once your language is in wide
use, reserving a new keyword is a breaking change to any code that was
previously using that name. If you can only reserve it inside a new context that
didn't previously exist (for example, async functions in C#), or in a context
where an identifier can't appear, then you can reserve it only in that context
and be confident that you didn't break any previous code.

So they're sort of an inevitable compromise when evolving a language over time.

Implementing them is pretty easy. The scanner scans them like regular
identifiers, since it doesn't generally know the surrounding context. In the
parser, you recognize the keyword in that context by looking for an identifier
token and checking to see if its lexeme is the right string.
