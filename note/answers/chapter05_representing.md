1.  There are a few ways to do it. Here is one:

    ```text
    expr → expr calls
    expr → IDENTIFIER
    expr → NUMBER

    calls → calls call
    calls → call

    call → "(" ")"
    call → "(" arguments ")"
    call → "." IDENTIFIER

    arguments → expr
    arguments → arguments "," expr
    ```

    It's the syntax for a function invocation.

2.  One way is to create a record or tuple containing a function pointer for
    each operation. In order to allow defining new types and passing them to
    existing code, these functions need to encapsulate the type entirely -- the
    existing code isn't aware of it, so it can't type check. You can do that by
    having the functions be closures that all close over the same shared object,
    "this", basically.

3.  Here you go:

    ```java
    class RpnPrinter implements Expr.Visitor<String> {
      String print(Expr expr) {
        return expr.accept(this);
      }

      @Override
      public String visitBinaryExpr(Expr.Binary expr) {
        return expr.left.accept(this) + " " +
               expr.right.accept(this) + " " +
               expr.operator.lexeme;
      }

      @Override
      public String visitGroupingExpr(Expr.Grouping expr) {
        return expr.expression.accept(this);
      }

      @Override
      public String visitLiteralExpr(Expr.Literal expr) {
        return expr.value.toString();
      }

      @Override
      public String visitUnaryExpr(Expr.Unary expr) {
        String operator = expr.operator.lexeme;
        if (expr.operator.type == TokenType.MINUS) {
          // Can't use same symbol for unary and binary.
          operator = "~";
        }

        return expr.right.accept(this) + " " + operator;
      }

      public static void main(String[] args) {
        Expr expression = new Expr.Binary(
            new Expr.Unary(
                new Token(TokenType.MINUS, "-", null, 1),
                new Expr.Literal(123)),
            new Token(TokenType.STAR, "*", null, 1),
            new Expr.Grouping(
                new Expr.Literal("str")));

        System.out.println(new AstPrinter().print(expression));
      }
    }
    ```

    Note that we have to handle unary "-" specially. In RPN, we can't use the
    same symbol for both unary and binary forms. When we encounter it, we
    wouldn't know whether to pop one or two numbers off the stack. So, to
    disambiguate, we pick a different symbol for negation.
