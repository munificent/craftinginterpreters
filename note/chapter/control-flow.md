Mention (or challenge question?) about adding support for "break" to loops.

--

Talk about "for-in" loops and iterator protocols.

--

Though Truth and Falsehood be
Near twins, yet Truth a little elder is.
John Donne
Satyre III (c. 1598)

--

Explain desugaring. Old prose:

A common first step is eliminating <span name="sugar">**syntactic
sugar**</span>. Syntactic sugar is a dusting of grammatical shorthands that make
code more pleasant for users to read and write, but that don't let them express
anything they couldn't write using other existing, more primitive language
features.

<aside name="sugar">

This delighful turn of phrase was coined by Peter J. Landin in 1964 to describe
how some of the nice expression forms supported by languages like ALGOL were a
sugaring over the more fundamental, yet presumably less palatable lambda
calculus underneath.

![Slightly more than a spoonful of sugar.](image/a-map-of-the-territory/sugar.png)

</aside>

For example, in C, loops are mere syntactic sugar for labels and jumps. Take
this code:

```c
for (a = 0; a < 10; a++) {
  printf("%d\n", a);
}
```

The compiler can **desugar** it to:

```c
  a = 0;
for_start:
  if (!(a < 10)) goto for_end;
  a++;
  printf("%d\n", a);
  goto for_start;
for_end:
```

(Even that little `++` can be desugared to an explicit addition and assignment.)
Expanding these shorthands to more verbose but less numerous constructs makes
later passes easier, since they have fewer different language features to worry
about. If, for example, you desugar all `do`, `for`, and `while` loops to
`goto`, then a later optimization that applies to `goto` will speed up all three
of them.

Simplifying the code by removing syntactic sugar is only the beginning. There
are a <span name="established">pack</span> of well-established intermediate
representations and a veritable bestiary of carefully named and studied
optimizations that can be applied to code once translated to one of them.

<aside name="established">

If you want some keywords to search for, try "control flow graph", "static
single-assignment", "continuation-passing style", and "three-address code" for
IRs.

For optimizations, check out "constant propagation", "common subexpression
elimination", "loop invariant code motion", "global value numbering", "strength
reduction", "scalar replacement of aggregates", "dead code elimination", and
"loop unrolling", to name a few.

</aside>

---

Explain truthiness. Old prose:

We've got the logical operators next, but first we need to make a little side
trip to one of the great questions of Western philosophy: *what is truth?*

OK, maybe we're not going to really get into the universal question, but at
least inside the world of Lox, we need to decide what happens when you use
something other than `true` or `false` in a logical operator or other place
where a Boolean is expected.

We *could* just say it's an error because we don't roll with implicit
conversions, but most dynamically-typed languages aren't that ascetic. Instead,
they take the universe of values of all types and partition them into two sets,
one of which they define to be "true", or "truthful", or (my favorite) "truthy",
and the rest which are "false" or "falsey". This partitioning is somewhat arbitrary and gets <span name="weird">weird</span> in some languages.

<aside name="weird">

In JavaScript, strings are truthy, but empty strings are not. Arrays are truthy
but empty arrays are... also truthy. The number `0` is falsey, but the *string*
`"0"` is truthy.

In Python, empty strings are falsey like JS, but other empty sequences are falsey too.

In PHP, both the number `0` and the string `"0"` are falsey. Most other non-empty strings are truthy.

Get all that?

</aside>

Lox follows Ruby's simple rule: `false` and `nil` are falsey and everything else
is truthy.
