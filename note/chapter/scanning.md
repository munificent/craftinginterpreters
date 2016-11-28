--

Challenge for scanning chapter: Add /* */ comments. Should they nest?

--

Talk about how Fortran didn't require whitespace to separate keywords.

--

"Lexeme" - the text matched by some lexical rule.

--

The scanner is the only pass that looks at each individual character. Perf
critical.

--

Talk about regular languages, Chomsky hierarchy, FSM.

- first define which sequences of characters produce what tokens
- formalism for this called "regular language"
- probably already familiar with this
- one corner of pl theory that has escaped into general use
- regex
- regex syntax
  - literal chars
  - |
  - *, ?, and +
  - grouping

- finite state machine
- monster that has a single state (roughly what kind of token currently on)
- eats chars, periodically excretes token
