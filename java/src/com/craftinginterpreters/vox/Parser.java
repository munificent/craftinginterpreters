package com.craftinginterpreters.vox;

import java.util.ArrayList;
import java.util.List;

class Parser {
  Parser(Lexer lexer, ErrorReporter errorReporter) {
    this.lexer = lexer;
    this.errorReporter = errorReporter;

//    this.errorReporter.hasError = false;
//    this.needsMoreInput = false;
  }

  List<Stmt> parseProgram () {
    List<Stmt> statements = new ArrayList<>();
    while (!match(TokenType.EOF)) {
      statements.add(statement());
    }

    return statements;
  }

  Stmt parseStatement() {
    Stmt stmt = statement();
    consume(TokenType.EOF, "Expect end.");
    return stmt;
  }

  private Stmt statement() {
    // Class declaration.
    if (match(TokenType.CLASS)) {
      Token name = consume(TokenType.IDENTIFIER, "Expect class name.");

      Expr superclass = null;
      if (match(TokenType.LESS)) {
        superclass = primary();
      }

      List<Stmt.Function> methods = new ArrayList<>();
      consume(TokenType.LEFT_BRACE, "Expect '{' before class body.");

      while (!peek(TokenType.RIGHT_BRACE) && !peek(TokenType.EOF)) {
        methods.add(function("method"));
      }

      consume(TokenType.RIGHT_BRACE, "Expect '}' after class body.");

      return new Stmt.Class(name.text, superclass, methods);
    }

    // Function declaration.
    if (match(TokenType.FUN)) return function("function");

    // If.
    if (match(TokenType.IF)) {
      consume(TokenType.LEFT_PAREN, "Expect '(' after 'if'.");
      Expr condition = expression();
      consume(TokenType.RIGHT_PAREN, "Expect ')' after if condition.");
      Stmt thenBranch = statement();
      Stmt elseBranch = null;
      if (match(TokenType.ELSE)) {
        elseBranch = statement();
      }

      return new Stmt.If(condition, thenBranch, elseBranch);
    }

    // Return.
    if (match(TokenType.RETURN)) {
      Expr value = null;
      if (!peek(TokenType.SEMICOLON)) {
        value = expression();
      }

      consume(TokenType.SEMICOLON, "Expect ';' after return value.");
      return new Stmt.Return(value);
    }

    // Variable declaration.
    if (match(TokenType.VAR)) {
      Token name = consume(TokenType.IDENTIFIER, "Expect variable name.");

      // TODO: Make this optional.
      consume(TokenType.EQUAL, "Expect '=' after variable name.");
      Expr initializer = expression();
      consume(TokenType.SEMICOLON, "Expect ';' after variable initializer.");

      return new Stmt.Var(name.text, initializer);
    }

    // While.
    if (match(TokenType.WHILE)) {
      consume(TokenType.LEFT_PAREN, "Expect '(' after 'while'.");
      Expr condition = expression();
      consume(TokenType.RIGHT_PAREN, "Expect '(' after condition.");
      Stmt body = statement();

      return new Stmt.While(condition, body);
    }

    // Block.
    if (peek(TokenType.LEFT_BRACE)) return block();

    // Expression statement.
    Expr expr = expression();
    consume(TokenType.SEMICOLON, "Expect ';' after expression.");
    return new Stmt.Expression(expr);
  }

  private Stmt.Function function(String kind) {
    Token name = consume(TokenType.IDENTIFIER, "Expect " + kind + " name.");

    consume(TokenType.LEFT_PAREN, "Expect '(' after " + kind + " name.");
    List<String> parameters = new ArrayList<>();
    if (!peek(TokenType.RIGHT_PAREN)) {
      do {
        Token parameter = consume(TokenType.IDENTIFIER,
            "Expect parameter name.");
        parameters.add(parameter.text);
      } while (match(TokenType.COMMA));
    }
    consume(TokenType.RIGHT_PAREN, "Expect ')' after parameters.");

    Stmt body = block();
    return new Stmt.Function(name.text, parameters, body);
  }

  private Stmt block() {
    consume(TokenType.LEFT_BRACE, "Expect '{' before block.");
    List<Stmt> statements = new ArrayList<>();

    while (!peek(TokenType.RIGHT_BRACE) && !peek(TokenType.EOF)) {
      statements.add(statement());
    }

    consume(TokenType.RIGHT_BRACE, "Expect '}' after block.");

    return new Stmt.Block(statements);
  }

  private Expr expression() {
    return assignment();
  }

  private Expr assignment() {
    Expr expr = or();

  if (match(TokenType.EQUAL)) {
    Expr value = assignment();

    if (expr instanceof Expr.Variable) {
      String name = ((Expr.Variable)expr).name;
      // TODO: Test.
      if (name.equals("this")) error("Cannot assign to 'this'.");
      return new Expr.Assign(null, name, value);
    } else if (expr instanceof Expr.Property) {
      Expr.Property property = (Expr.Property)expr;
      return new Expr.Assign(property.object, property.name, value);
    }

    error("Invalid assignment target.");
  }

    return expr;
  }

  private Expr or() {
    Expr expr = and();

    while (match(TokenType.OR)) {
      Token operator = previous;
      Expr right = and();
      expr = new Expr.Logical(expr, operator, right);
    }

    return expr;
  }

  private Expr and() {
    Expr expr = equality();

    while (match(TokenType.AND)) {
      Token operator = previous;
      Expr right = equality();
      expr = new Expr.Logical(expr, operator, right);
    }

    return expr;
  }

  private Expr equality() {
    Expr expr = comparison();

    while (match(TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL)) {
      Token operator = previous;
      Expr right = comparison();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr comparison() {
    Expr expr = term();

    while (match(TokenType.GREATER, TokenType.GREATER_EQUAL,
                 TokenType.LESS, TokenType.LESS_EQUAL)) {
      Token operator = previous;
      Expr right = term();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr term() {
    Expr expr = factor();

    while (match(TokenType.MINUS, TokenType.PLUS)) {
      Token operator = previous;
      Expr right = factor();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr factor() {
    Expr expr = unary();

    // TODO: Refactor out redundancy between binary methods?

    while (match(TokenType.PERCENT, TokenType.SLASH, TokenType.STAR)) {
      Token operator = previous;
      Expr right = unary();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr unary() {
    if (match(TokenType.BANG, TokenType.MINUS, TokenType.PLUS)) {
      Token operator = previous;
      Expr right = unary();
      return new Expr.Unary(operator, right);
    }

    return call();
  }

  private Expr call() {
    Expr expr = primary();

    while (true) {
      if (match(TokenType.LEFT_PAREN)) {
        List<Expr> arguments = new ArrayList<>();

        if (!peek(TokenType.RIGHT_PAREN)) {
          do {
            arguments.add(expression());
          } while (match(TokenType.COMMA));
        }

        consume(TokenType.RIGHT_PAREN, "Expect ')' after argument list.");
        expr = new Expr.Call(expr, arguments);
      } else if (match(TokenType.DOT)) {
        Token name = consume(TokenType.IDENTIFIER,
            "Expect property name after '.'.");
        expr = new Expr.Property(expr, name.text);
      } else {
        break;
      }
    }

    return expr;
  }

  private Expr primary() {
    // TODO: Switch on type?

    if (match(TokenType.FALSE)) return new Expr.Literal(false);
    if (match(TokenType.TRUE)) return new Expr.Literal(true);
    if (match(TokenType.NULL)) return new Expr.Literal(null);

    if (match(TokenType.NUMBER, TokenType.STRING)) {
      return new Expr.Literal(previous.value);
    }

    if (match(TokenType.THIS)) return new Expr.Variable("this");

    if (match(TokenType.IDENTIFIER)) {
      return new Expr.Variable(previous.text);
    }

    if (match(TokenType.LEFT_PAREN)) {
      Expr expr = expression();
      consume(TokenType.RIGHT_PAREN, "Expect ')' after expression.");
      return new Expr.Grouping(expr);
    }

    error("Unexpected token '" + current.text + "'.");
    return null;
  }

  private boolean match(TokenType... types) {
    boolean found = false;
    for (TokenType type : types) {
      if (peek(type)) {
        found = true;
        break;
      }
    }
    if (!found) return false;

    previous = current;
    current = lexer.nextToken();
    return true;
  }

  private Token consume(TokenType type, String message) {
    if (!peek(type)) {
      error(message);
    }

//    // If the first error happened because we unexpectedly hit the end of the
//    // input, let the caller know.
//    if (!this.errorReporter.hasError && this.current.type == Token.end) {
//    this.errorReporter.needsMoreInput = true;
//    }
//
//    this.error("on " + this.current.type + ": " + message);
//    }

    previous = current;
    current = lexer.nextToken();
    return previous;
  }

  // TODO: Use different name since peek() in Lexer works differently.
  // Returns true if the current token is of tokenType, but does not consume it.
  private boolean peek(TokenType tokenType) {
    if (current == null) current = lexer.nextToken();
    return current.type == tokenType;
  }

  private void error(String message) {
    errorReporter.error(current.line, message);
  }

  private final Lexer lexer;
  private final ErrorReporter errorReporter;
  private Token current;
  private Token previous;
}
