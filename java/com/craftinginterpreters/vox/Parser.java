package com.craftinginterpreters.vox;

import java.util.ArrayList;
import java.util.List;

class Parser {
  private final Scanner scanner;
  private final ErrorReporter errorReporter;
  private Token current;
  private Token previous;

  private interface ExprParser {
    Expr parse();
  }

  Parser(Scanner scanner, ErrorReporter errorReporter) {
    this.scanner = scanner;
    this.errorReporter = errorReporter;
  }

  List<Stmt> parseProgram () {
    List<Stmt> statements = new ArrayList<>();
    while (!match(TokenType.EOF)) {
      statements.add(statement());
    }

    return statements;
  }

  private Stmt statement() {
    if (match(TokenType.CLASS)) return classStatement();
    if (match(TokenType.FUN)) return function("function");
    if (match(TokenType.IF)) return ifStatement();
    if (match(TokenType.RETURN)) return returnStatement();
    if (match(TokenType.VAR)) return varStatement();
    if (match(TokenType.WHILE)) return whileStatement();
    if (check(TokenType.LEFT_BRACE)) return block();

    // Expression statement.
    Expr expr = expression();
    consume(TokenType.SEMICOLON, "Expect ';' after expression.");
    return new Stmt.Expression(expr);
  }

  private Stmt classStatement() {
    Token name = consume(TokenType.IDENTIFIER, "Expect class name.");

    Expr superclass = null;
    if (match(TokenType.LESS)) {
      superclass = primary();
    }

    List<Stmt.Function> methods = new ArrayList<>();
    consume(TokenType.LEFT_BRACE, "Expect '{' before class body.");

    while (!check(TokenType.RIGHT_BRACE) && !check(TokenType.EOF)) {
      methods.add(function("method"));
    }

    consume(TokenType.RIGHT_BRACE, "Expect '}' after class body.");

    return new Stmt.Class(name, superclass, methods);
  }

  private Stmt ifStatement() {
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

  private Stmt returnStatement() {
    Expr value = null;
    if (!check(TokenType.SEMICOLON)) {
      value = expression();
    }

    consume(TokenType.SEMICOLON, "Expect ';' after return value.");
    return new Stmt.Return(value);
  }

  private Stmt varStatement() {
    Token name = consume(TokenType.IDENTIFIER, "Expect variable name.");
    if (name.text.equals("this")) error("'this' cannot be a variable name.");

    consume(TokenType.EQUAL, "Expect '=' after variable name.");
    Expr initializer = expression();
    consume(TokenType.SEMICOLON, "Expect ';' after variable initializer.");

    return new Stmt.Var(name, initializer);
  }

  private Stmt whileStatement() {
    consume(TokenType.LEFT_PAREN, "Expect '(' after 'while'.");
    Expr condition = expression();
    consume(TokenType.RIGHT_PAREN, "Expect '(' after condition.");
    Stmt body = statement();

    return new Stmt.While(condition, body);
  }

  private Stmt.Function function(String kind) {
    Token name = consume(TokenType.IDENTIFIER, "Expect " + kind + " name.");

    consume(TokenType.LEFT_PAREN, "Expect '(' after " + kind + " name.");
    List<Token> params = new ArrayList<>();
    if (!check(TokenType.RIGHT_PAREN)) {
      do {
        Token param = consume(TokenType.IDENTIFIER, "Expect parameter name.");
        params.add(param);
      } while (match(TokenType.COMMA));
    }
    consume(TokenType.RIGHT_PAREN, "Expect ')' after parameters.");

    Stmt body = block();
    return new Stmt.Function(name, params, body);
  }

  private Stmt block() {
    consume(TokenType.LEFT_BRACE, "Expect '{' before block.");
    List<Stmt> statements = new ArrayList<>();

    while (!check(TokenType.RIGHT_BRACE) && !check(TokenType.EOF)) {
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
      Token name = ((Expr.Variable)expr).name;
      if (name.text.equals("this")) error("Cannot assign to 'this'.");
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
    // TODO: Refactor to use this:
//    return binary(this::unary, TokenType.PERCENT, TokenType.SLASH, TokenType.STAR);
  }

//  private Expr binary(ExprParser operandParser, TokenType... types) {
//    Expr expr = operandParser.parse();
//
//    while (match(types)) {
//      Token operator = previous;
//      Expr right = operandParser.parse();
//      expr = new Expr.Binary(expr, operator, right);
//    }
//
//    return expr;
//  }

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

        if (!check(TokenType.RIGHT_PAREN)) {
          do {
            arguments.add(expression());
          } while (match(TokenType.COMMA));
        }

        Token paren = consume(TokenType.RIGHT_PAREN,
            "Expect ')' after argument list.");
        expr = new Expr.Call(expr, paren, arguments);
      } else if (match(TokenType.DOT)) {
        Token name = consume(TokenType.IDENTIFIER,
            "Expect property name after '.'.");
        expr = new Expr.Property(expr, name);
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

    if (match(TokenType.THIS)) return new Expr.Variable(previous);

    if (match(TokenType.IDENTIFIER)) {
      return new Expr.Variable(previous);
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
      if (check(type)) {
        found = true;
        break;
      }
    }
    if (!found) return false;

    previous = current;
    current = scanner.readToken();
    return true;
  }

  private Token consume(TokenType type, String message) {
    if (!check(type)) {
      error(message);
    }

    previous = current;
    current = scanner.readToken();
    return previous;
  }

  // Returns true if the current token is of tokenType, but does not consume it.
  private boolean check(TokenType tokenType) {
    if (current == null) current = scanner.readToken();
    return current.type == tokenType;
  }

  private void error(String message) {
    errorReporter.error(current.line, message);
  }
}
