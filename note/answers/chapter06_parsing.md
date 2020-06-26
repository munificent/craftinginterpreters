1.  The comma operator has the lowest precedence, so it goes between expression
    and equality:

    ```ebnf
    expression     → comma ;
    comma          → equality ( "," equality )* ;
    equality       → comparison ( ( "!=" | "==" ) comparison )* ;
    comparison     → addition ( ( ">" | ">=" | "<" | "<=" ) addition )* ;
    addition       → multiplication ( ( "-" | "+" ) multiplication )* ;
    multiplication → unary ( ( "/" | "*" ) unary )* ;
    unary          → ( "!" | "-" | "--" | "++" ) unary
                   | postfix ;
    postfix        → primary ( "--" | ++" )* ;
    primary        → NUMBER | STRING | "true" | "false" | "nil"
                   | "(" expression ")" ;
    ```

    We could define a new syntax tree node by adding this to the `defineAst()`
    call:

    ```java
    "Comma    : Expr left, Expr right",
    ```

    But a simpler choice is to treat it like any other binary operator and
    reuse Expr.Binary.

    Parsing is similar to other infix operators (except that we don't bother to
    keep the operator token):

    ```java
    private Expr expression() {
      return comma();
    }

    private Expr comma() {
      Expr expr = equality();

      while (match(COMMA)) {
        Token operator = previous();
        Expr right = equality();
        expr = new Expr.Binary(expr, operator, right);
      }

      return expr;
    }
    ```

    Keep in mind that commas are already used in the grammar to separate
    arguments in function calls. With the above change, this:

    ```lox
    foo(1, 2)
    ```

    Now gets parsed as:

    ```lox
    foo((1, 2))
    ```

    In other words, pass a single argument to `foo`, the result of evaluating
    `1, 2`. That's not what we want. To fix that, we simply change the way we
    parse function arguments to require a higher precedence expression than the
    comma operator:

    ```java
    if (!check(RIGHT_PAREN)) {
      do {
        if (arguments.size() >= 8) {
          error(peek(), "Cannot have more than 8 arguments.");
        }
        arguments.add(equality()); // <-- was expression().
      } while (match(COMMA));
    }
    ```

2.  We just need one new rule.

    ```ebnf
    expression  → conditional ;
    conditional → equality ( "?" expression ":" conditional )? ;
    // Other rules...
    ```

    The precedence of the operands is pretty interesting. The left operand has
    higher precedence than the others, and the middle operand has lower
    precedence than the condition expression itself. That allows:

        a ? b = c : d

    Again, I won't bother showing the scanner and token changes since they're
    pretty obvious.

    ```java
    private Expr expression() {
      return conditional();
    }

    private Expr conditional() {
      Expr expr = equality();

      if (match(QUESTION)) {
        Expr thenBranch = expression();
        consume(COLON,
            "Expect ':' after then branch of conditional expression.");
        Expr elseBranch = conditional();
        expr = new Expr.Conditional(expr, thenBranch, elseBranch);
      }

      return expr;
    }
    ```

3.  Here's an updated grammar. The grammar itself doesn't "know" that some of
    these productions are errors. The parser handles that.

    ```ebnf
    expression     → equality ;
    equality       → comparison ( ( "!=" | "==" ) comparison )* ;
    comparison     → addition ( ( ">" | ">=" | "<" | "<=" ) addition )* ;
    addition       → multiplication ( ( "-" | "+" ) multiplication )* ;
    multiplication → unary ( ( "/" | "*" ) unary )* ;
    unary          → ( "!" | "-" | "--" | "++" ) unary
                   | postfix ;
    postfix        → primary ( "--" | ++" )* ;
    primary        → NUMBER | STRING | "true" | "false" | "nil"
                   | "(" expression ")"
                   // Error productions...
                   | ( "!=" | "==" ) equality
                   | ( ">" | ">=" | "<" | "<=" ) comparison
                   | ( "+" ) addition
                   | ( "/" | "*" ) multiplication ;
    ```

    Note that "-" isn't an error production because that *is* a valid prefix
    expression.

    With the normal infix productions, the operand non-terminals are one
    precedence level higher than the operator's own precedence. In order to
    handle a series of operators of the same precedence, the rules explicitly
    allow repetition.

    With the error productions, though, the right-hand operand rule is the same
    precedence level. That will effectively strip off the erroneous leading
    operator and then consume a series of infix uses of operators at the same
    level by reusing the existing correct rule. For example:

    ```lox
    + a - b + c - d
    ```

    The error production for `+` will match the leading `+` and then use
    `addition` to also match the rest of the expression.

    ```java
    private Expr primary() {
      if (match(FALSE)) return new Expr.Literal(false);
      if (match(TRUE)) return new Expr.Literal(true);
      if (match(NIL)) return new Expr.Literal(null);

      if (match(NUMBER, STRING)) {
        return new Expr.Literal(previous().literal);
      }

      if (match(LEFT_PAREN)) {
        Expr expr = expression();
        consume(RIGHT_PAREN, "Expect ')' after expression.");
        return new Expr.Grouping(expr);
      }

      // Error productions.
      if (match(BANG_EQUAL, EQUAL_EQUAL)) {
        error(previous(), "Missing left-hand operand.");
        equality();
        return null;
      }

      if (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
        error(previous(), "Missing left-hand operand.");
        comparison();
        return null;
      }

      if (match(PLUS)) {
        error(previous(), "Missing left-hand operand.");
        addition();
        return null;
      }

      if (match(SLASH, STAR)) {
        error(previous(), "Missing left-hand operand.");
        multiplication();
        return null;
      }

      throw error(peek(), "Expect expression.");
    }
    ```
