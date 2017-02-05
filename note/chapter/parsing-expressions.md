
When talking about error recovery, note that it's less of a big deal today.
Back when machines were super slow, compile runs were infrequent and it was
important to report as many errors correctly as possible in a single run.

Today, compiling is fast and users often only look at the first error in each
compile run, fix it, and retry. Means we don't need very sophisticated error
recovery.

--

Note that we use an EOF token instead of just checking for the end of the token
list because the end of the file may be on a different line than the last
non-whitespace token.

--

When parsing number literals, mention checking for infinity.

--

Talk about EBNF and context-free grammars.
https://www.cs.cmu.edu/~pattis/misc/ebnf.pdf

- ebnf, other variations
- backus naur feud
- expressions versus statements
- bnf is a metasyntax
- designed for algol 58, first used in algol 60
- both lexical and grammatical syntax
- lexical doesn't recurse
- bnf originally language for humans, but then became input format for yacc
  et al
- backus led team that invented fortran

- challenges
  - add "--", "++", "|" and "&" to grammar
  - is an empty source file a valid program?
  - write the bnf for a language you know. if the language is complex, just do
    an interesting subset. what parts are hard?

--

error recovery:

The tricky part, of course, is that the first error may *cause* later **cascaded errors**. For example, if they accidentally started a string with `'` instead of `"`, then the rest of the string literal will likely cause a number of bogus syntax errors when the scanner and parser tries to treat it like code.

There is an art, called **error recovery** to getting back to a good state after an error is found to minimize the number of later spurious errors. We'll talk more about it during parsing.

--

In representing code, talking about how grammar generates sentences. Now go
in other direction.

- can play game in other direction as well
- given string of symbols, starting at start rule, can we pick set of
  productions that *match* string?
- ex: is "glorious fire-breathing dragon" valid monsters?
- what about "tiny troll beast"?
- call this process parsing
- more about it later

--

Explain difference between parse tree and AST now that we have intermediate
productions for handling precedence that aren't stored in tree.
