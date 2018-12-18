##1

It's:

```
expression
| parsePrecedence(PREC_ASSIGNMENT)
| | grouping
| | | expression
| | | | parsePrecedence(PREC_ASSIGNMENT)
| | | | | unary // for "-"
| | | | | | parsePrecedence(PREC_UNARY)
| | | | | | | number
| | | | | binary // for "+"
| | | | | | parsePrecedence(PREC_FACTOR) // PREC_TERM + 1
| | | | | | | number
| | binary // for "*"
| | | parsePrecedence(PREC_UNARY) // PREC_FACTOR + 1
| | | | number
| | binary // for "-"
| | | parsePrecedence(PREC_FACTOR) // PREC_TERM + 1
| | | | unary // for "-"
| | | | | parsePrecedence(PREC_UNARY)
| | | | | | number
```

## 2

Lox only has one other: left parenthesis is used as a prefix expression for
grouping, and as an infix expression for invoking a function.

Several languages allow `+` as a prefix unary operator as a parallel to `-` and
then also of course use infix `+` for addition.

A number of languages use square brackets for list or array literals, which
makes `[` a prefix expression and then also use square brackets as a subscript
operator to access elements from a list.

C uses `*` as a prefix operator to dereference a pointer and as infix for
multiplication. Likewise, `&` is a prefix address-of operator and infix bitwise
and.

`*` and `&` aren't prefix *expressions* in Ruby, but they can appear in prefix
position before an argument in an argument list.

## 3

The `?:` operator has lower precedence than almost anything, so we add a new `PREC_CONDITIONAL` level between `PREC_ASSIGN` and `PREC_OR`. I'll skip adding the new TokenType enums for `?` and `:`. That part is pretty obvious. In the new row in the table for the `?` token type, we call:

```c
static void conditional()
{
  // Compile the then branch.
  parsePrecedence(compiler, PREC_CONDITIONAL);

  consume(compiler, TOKEN_COLON,
          "Expect ':' after then branch of conditional operator.");

  // Compile the else branch.
  parsePrecedence(compiler, PREC_ASSIGNMENT);
}
```

Of course a full implementation needs more code to actually do the conditional
evaluation, but that should compile the operands with the right precedence. Note
that the precedence of the operands is a little unusual. The precedence of the
last operand is *lower* than the conditional expression itself.

That might be surprising, but it's how C rolls.
