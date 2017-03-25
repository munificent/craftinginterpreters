--

Talk about how we handle unknown identifiers. Because we want to support the
REPL and forward references to global vars, we allow them. A stricter language
would not.

Should our interpreter behave differently when run in REPL versus file mode?

--

When introducing statements in the variables chapter explain that one difference
is that statements are purely about side effects since they produce no value.
A variable declaration is mainly for its side effect -- it modifies the
environment.

--

The error message on:

  if (true) var foo;

(i.e. a declaration where a statement is expected) isn't very helpful. Talk
about parsing a larger grammar to improve error messages. So, here, allow
parsing a declaration even though it isn't valid.

--

Lisp in Small Pieces has some good notes around choosing the semantics of top
level variables. I think chapter 3.4? Look for "Hyperstatic".

--

Explain that we store parentheses in the syntax tree so that we can report
`(a) = 2` as a syntax error.
