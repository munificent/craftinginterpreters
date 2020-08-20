1.  Smalltalk has different call syntax for different arities. To define a
    method that takes multiple arguments, you use **keyword selectors**. Each
    argument has a piece of the method name preceding instead of using commas
    as a separator. For example, a method like:

    ```lox
    list.insert("element", 2)
    ```

    To insert "element" as index 2 would look like this in Smalltalk:

    ```smalltalk
    list insert: "element" at: 2
    ```

    Smalltalk doesn't use a dot to separate method name from receiver. More
    interestingly, the "insert:" and "at:" parts both form a single method
    call whose full name is "insert:at:". Since the selectors and the colons
    that separate them form part of the method's name, there's no way to call
    it with the wrong number of arguments. You can't pass too many or two few
    arguments to "insert:at:" because there would be no way to write that call
    while still actually naming that method.

2.  This requires juggling some code around. In GenerateAst, we need a node
    for function expressions. In the defineAst() call for Expr, add:

    ```java
    "Function : List<Token> parameters, List<Stmt> body",
    ```

    While we're at it, we can reuse that for function statements. A function
    *statement* is now just a name and a function expression:

    ```java
    "Function   : Token name, Expr.Function function",
    ```

    Over in LoxFunction, it will store an Expr.Function instead of a statement
    to handle both types. If the function does have a name, that is tracked
    separately, since lambdas won't have one:

    ```java
    class LoxFunction implements Callable {
      private final String name;
      private final Expr.Function declaration;
      private final Environment closure;

      LoxFunction(String name, Expr.Function declaration, Environment closure) {
        this.name = name;
        this.closure = closure;
        this.declaration = declaration;
      }
      @Override
      public String toString() {
        if (name == null) return "<fn>";
        return "<fn " + name + ">";
      }

      // ...
    }
    ```

    The parser changes are a little more complex.
    The logic to handle anonymous functions is separated out into a new method.
    The method to handle named functions is now a wrapper around the one
    that handles anonymous functions:

    ```java
    private Stmt.Function function(String kind) {
      Token name = consume(IDENTIFIER, "Expect " + kind + " name.");
      return new Stmt.Function(name, functionBody(kind));
    }

    private Expr.Function functionBody(String kind) {
      consume(LEFT_PAREN, "Expect '(' after " + kind + " name.");
      List<Token> parameters = new ArrayList<>();
      if (!check(RIGHT_PAREN)) {
        do {
          if (parameters.size() >= 8) {
            error(peek(), "Cannot have more than 8 parameters.");
          }

          parameters.add(consume(IDENTIFIER, "Expect parameter name."));
        } while (match(COMMA));
      }
      consume(RIGHT_PAREN, "Expect ')' after parameters.");

      consume(LEFT_BRACE, "Expect '{' before " + kind + " body.");
      List<Stmt> body = block();
      return new Expr.Function(parameters, body);
    }
    ```

    Now we can use `functionBody()` to parse lambdas. In `primary()`, add
    another clause:

    ```java
    if (match(FUN)) return functionBody("function");
    ```

    We've got one nasty little problem. We want lambdas to be a valid primary
    expression, and in theory any primary expression is allowed in a primary
    statement. But if you try to do:

    ```lox
    fun () {};
    ```

    Then the `declaration()` parser will match that `fun` and try to parse it
    as a named function declaration statement. It won't see a name and will
    report a parse error. Even though the above code is pointless, we want it
    to work to avoid a weird edge case in the grammar.

    To handle that, we only want to parse a function declaration if the current
    token is `fun` and the one past that is an identifier. That requires another
    token of lookahead, as we add:

    ```java
    private boolean checkNext(TokenType tokenType) {
      if (isAtEnd()) return false;
      if (tokens.get(current + 1).type == EOF) return false;
      return tokens.get(current + 1).type == tokenType;
    }
    ```

    Then, in `declaration()`, change the `match(FUN)) ...` line to:

    ```java
    if (check(FUN) && checkNext(IDENTIFIER)) {
      consume(FUN, null);
      return function("function");
    }
    ```

    Now only a function with a name is parsed as such.

3.  No, it isn't. Lox uses the same scope for the parameters and local variables
    immediately inside the body. That's why Stmt.Function stores the body as a
    list of statements, not a single Stmt.Block that would create its own
    nested scope separate from the parameters.

    In Java, it's an error because you aren't allowed to shadow local variables
    inside a method or collide them.

    It's an error in C because parameters and locals share the same scope.

    It is allowed in Dart. There, parameters are in a separate scope surrounding
    the function body.

    I'm not a fan of Dart's choice. I think shadowing should be allowed in
    general because it helps ensure changes to code are encapsulated and don't
    affect parts of the program unrelated to the change. (See this design note
    for more: http://localhost:8000/statements-and-state.html#design-note).

    But shadowing still usually leads to more confusing code, so it should be
    avoided when possible. The only thing putting parameters in an outer scope
    allows is shadowing those parameters, but I think any code that did that
    would be *very* hard to read. I would rather prohibit that outright.
