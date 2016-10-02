- The "novelty budget" and choosing which things to keep familiar and which to
  keep new.

- Learnability versus consistency. Being internally consistent leads to a
  simpler, more elegant language, but doesn't leverage what the user already
  knows.

- Building an entire ecosystem: implementation, spec, core libraries, docs, etc.

- Deciding how many things can be done at the library level versus special
  permissions only the language has.

- Choosing reserved words. Abbreviations to avoid name collisions.

- When to write a language spec and what it's for. Usefull to help think
  precisely about semantics. Doesn't help improve usability of language. Need to
  actually play with an implementation for that. Very time consuming. Users
  don't need it. They need more user-friendly docs. Important when you have
  multiple competing implementations.

- When to introduce new operators. Very hard to read. If users don't have
  intuition about precedence, they can't even visually parse it until they know
  what it is. Tempting, but resist.

  Some language designers, when presented with a problem, think "I know, I'll
  add a new operator." Now you have %*$! problems.

  Note that overloading existing operators is different from defining new
  ones. With the former, readers can still parse the code.

- Syntactic novelty. First time design language and write lexer and parser,
  excited to be able to do things different from other languages because you
  can and because novelty is fun. Do that. Try all sorts of new things. Get it
  out of your system. In practice, novelty has high cost.

- Put a code sample on the front page of your site.

- Which language features can have user-defined behavior. For example, for
  loop in some languages works with user-defined sequence types. Some
  languages allow operators to be overloaded, even assignment. Some allow
  user-defined truthiness.

  Trade off is power versus concrete readability.

- Something about evolving a language after it's released.