1.  The modification to the unary covers the prefix forms.

    ```lox
    expression → equality
    equality   → comparison ( ( "!=" | "==" ) comparison )*
    comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )*
    term       → factor ( ( "-" | "+" ) factor )*
    factor     → unary ( ( "/" | "*" ) unary )*
    unary      → ( "!" | "-" | "--" | "++" ) unary
               | postfix
    postfix    → primary ( "--" | ++" )*
    primary    → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")"
    ```

    I won't bother showing the scanner and token changes since those are
    pretty obvious. (See how "=" and "==" are handled for scanning.)

    I won't show the code for changing the syntax tree classes. Basically,
    instead of a single Expr.Unary class, we'll split it into Expr.Prefix
    and Expr.Postfix. You could also keep a single Unary class and add a boolean
    flag to it to track whether the operator was in prefix or postfix position.

    private Expr unary() {
      if (match(BANG, MINUS, MINUS_MINUS, PLUS_PLUS)) {
        Token operator = previous();
        Expr right = unary();
        return new Expr.Prefix(operator, right);
      }

      return postfix();
    }

    private Expr postfix() {
      Expr expr = primary();

      while (match(MINUS_MINUS, PLUS_PLUS)) {
        expr = new Expr.Postfix(previous(), expr);
      }

      return expr;
    }

2.  We just need one new rule.

    ```lox
    expression  → conditional
    conditional → equality ( "?" expression ":" conditional )?
    // Other rules...
    ```

    The precedence of the operands is pretty interesting. The left operand has
    higher precedence than the others, and the middle operand has lower
    precedence than the condition expression itself. That allows:

        a ? b = c : d

    Again, I won't bother showing the scanner and token changes since they're
    pretty obvious.

    private Expr expression() {
      return conditional();
    }

    private Expr conditional() {
      Expr expr = equality();

      if (match(QUESTION)) {
        Expr thenBranch = expression();
        consume(":", "Expect ':' after then branch of conditional expression.");
        Expr elseBranch = conditional();
        expr = new Expr.Conditional(expr, thenBranch, elseBranch);
      }

      return expr;
    }

3.  Here's an updated grammar. The grammar itself doesn't "know" that some of
    these productions are errors. The parser handles that.

    ```lox
    expression → equality
    equality   → comparison ( ( "!=" | "==" ) comparison )*
    comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )*
    term       → factor ( ( "-" | "+" ) factor )*
    factor     → unary ( ( "/" | "*" ) unary )*
    unary      → ( "!" | "-" | "--" | "++" ) unary
               | postfix
    postfix    → primary ( "--" | ++" )*
    primary    → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")"
               // Error productions...
               | ( "!=" | "==" ) equality
               | ( ">" | ">=" | "<" | "<=" ) comparison
               | ( "+" ) term
               | ( "/" | "*" ) factor
    ```

    Things to note:

    * "-" isn't an error production because that *is* a valid prefix
      expression.

    * The precedence for each operator is one level higher than it is for the
      normal correct. We also don't parse a sequence, just a single RHS. Using
      the same precedence level handles a sequence for us and helps us only
      show the error once?

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
        term();
        return null;
      }

      if (match(SLASH, STAR)) {
        error(previous(), "Missing left-hand operand.");
        factor();
        return null;
      }

      throw error(peek(), "Expect expression.");
    }