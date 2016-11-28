
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
