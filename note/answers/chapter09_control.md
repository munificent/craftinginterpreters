1.  The basic idea is that the control flow operations become methods that take
    callbacks for the blocks to execute when true or false. You define two
    classes with singleton instances, one for true and one for false. The
    implementations of the control flow methods on the true class invoke the
    then callbacks. The ones on the false class implement the else callbacks.

    Like so:

    ```lox
    class True {
      ifThen(thenBranch) {
        return thenBranch();
      }

      ifThenElse(thenBranch, elseBranch) {
        return thenBranch();
      }
    }

    class False {
      ifThen(thenBranch) {
        return nil;
      }

      ifThenElse(thenBranch, elseBranch) {
        return elseBranch();
      }
    }
    ```

    Then we make singleton instances of these classes:

    ```lox
    var t = True();
    var f = False();
    ```

    You can try them out like so:

    ```lox
    fun test(condition) {
      fun ifThenFn() {
        print "if then -> then";
      }

      condition.ifThen(ifThenFn);

      fun ifThenElseThenFn() {
        print "if then else -> then";
      }

      fun ifThenElseElseFn() {
        print "if then else -> else";
      }

      condition.ifThenElse(ifThenElseThenFn, ifThenElseElseFn);
    }

    test(t);
    test(f);
    ```

    This is famously how Smalltalk implements its control flow.

    It looks cumbersome because Lox doesn't have lambdas -- anonymous function
    expressions -- but those would be easy to add to the language if
    we wanted to go in this direction.

    Even more powerful would a nice terse syntax for defining and passing a
    closure to a method. The Grace language has a particularly nice notation
    for passing multiple blocks to a method. If we adapted that to Lox, we'd
    get something like:

    ```text
    fun test(condition) {
      condition.ifThen {
        print "if then -> then";
      };

      condition.ifThen {
        print "if then else -> then";
      } else {
        print "if then else -> else";
      };
    }

    test(t);
    test(f);
    ```

    It starts to look like this control flow is built into the language even
    though it's only method calls.

2.  Scheme is the language that famously shows that all iteration can be
    represented in terms of recursion and conditional execution. To execute a
    chunk of code more than once, hoist it out into a function that calls itself
    at the end of its body for the next iteration.

    For example, we could represent this for loop:

    ```lox
    for (var i = 0; i < 100; i = i + 1) {
      print i;
    }
    ```

    Like so:

    ```lox
    fun forStep(i) {
      print i;
      if (i < 99) forStep(i + 1);
    }
    ```

    When you see heavy use of recursion like here where there are almost a
    hundred recursive calls, the concern is overflowing the stack. However, in
    many cases, you don't need to preserve any information from the previous
    call when beginning a recursive call. If the recursive call is in *tail
    position* -- it's the last thing in the body of the function -- then you
    can discard any stack space used by the previous call before beginning the
    next one.

    This **tail call optimization** lets you use recursion for an unbounded
    number of iterations while consuming only a constant amount of stack space.
    Scheme and some other functional languages require an implementation to
    perform this optimization so that users can safely rely on recursion for
    iteration.

3.  As usual, we start with the AST:

    ```java
    defineAst(outputDir, "Stmt", Arrays.asList(
      "Block      : List<Stmt> statements",
      "Break      : ",  // <--
      "Expression : Expr expression",
      "If         : Expr condition, Stmt thenBranch, Stmt elseBranch",
      "Print      : Expr expression",
      "Var        : Token name, Expr initializer",
      "While      : Expr condition, Stmt body"
    ));
    ```

    Break doesn't have any fields, which actually breaks the little generator
    script, so you also need to change defineType() to:

    ```java
    // Store parameters in fields.
    String[] fields;
    if (fieldList.isEmpty()) {
      fields = new String[0];
    } else {
      fields = fieldList.split(", ");
    }
    ```

    Run that to get the new now. Now we need to push the syntax through the
    front end, starting with the new keyword. In TokenType, add `BREAK`:

    ```java
    // Keywords.
    AND, BREAK, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    ```

    And then define it in the lexer:

    ```java
    keywords.put("break",  BREAK);
    ```

    In the parser, we match the keyword in `statement()`:

    ```java
    if (match(BREAK)) return breakStatement();
    ```

    Which calls:

    ```java
    private Stmt breakStatement() {
      consume(SEMICOLON, "Expect ';' after 'break'.");
      return new Stmt.Break();
    }
    ```

    We need some additional parser support. It should be a syntax error to use
    "break" outside of a loop. We do that by adding a field in Parser to track
    how many enclosing loops there currently are:

    ```java
    private int loopDepth = 0;
    ```

    In `forStatement()`, we update that when parsing the loop body:

    ```java
    try {
      loopDepth++;
      Stmt body = statement();

      if (increment != null) {
        body = new Stmt.Block(Arrays.asList(
            body,
            new Stmt.Expression(increment)));
      }

      if (condition == null) condition = new Expr.Literal(true);
      body = new Stmt.While(condition, body);

      if (initializer != null) {
        body = new Stmt.Block(Arrays.asList(initializer, body));
      }

      return body;
    } finally {
      loopDepth--;
    }
    ```

    Likewise `whileStatement()`:

    ```java
    try {
      loopDepth++;
      Stmt body = statement();

      return new Stmt.While(condition, body);
    } finally {
      loopDepth--;
    }
    ```

    Now we can check that when parsing the break statement:

    ```java
    private Stmt breakStatement() {
      if (loopDepth == 0) {
        error(previous(), "Must be inside a loop to use 'break'.");
      }
      consume(SEMICOLON, "Expect ';' after 'break'.");
      return new Stmt.Break();
    }
    ```

    To interpret this, we'll use exceptions to jump from the break out of the
    loop. In Interpreter, define a class:

    ```java
    private static class BreakException extends RuntimeException {}
    ```

    Executing a break simply throws that:

    ```java
    @Override
    public Void visitBreakStmt(Stmt.Break stmt) {
      throw new BreakException();
    }
    ```

    That gets caught by the while loop code and then proceeds from there.

    ```java
    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
      try {
        while (isTruthy(evaluate(stmt.condition))) {
          execute(stmt.body);
        }
      } catch (BreakException ex) {
        // Do nothing.
      }
      return null;
    }
    ```
