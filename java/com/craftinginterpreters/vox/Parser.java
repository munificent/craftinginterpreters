package com.craftinginterpreters.vox;

import java.util.ArrayList;
import java.util.List;

import static com.craftinginterpreters.vox.TokenType.*;

class Parser {
  private final Scanner scanner;
  private final ErrorReporter errorReporter;
  private Token current;
  private Token previous;

  Parser(Scanner scanner, ErrorReporter errorReporter) {
    this.scanner = scanner;
    this.errorReporter = errorReporter;

    // Read the first token.
    advance();
  }

  List<Stmt> parseProgram() {
    List<Stmt> statements = new ArrayList<>();
    while (!match(EOF)) {
      statements.add(statement());
    }

    return statements;
  }

  private Stmt statement() {
    if (match(CLASS)) return classStatement();
    if (match(FUN)) return function("function");
    if (match(IF)) return ifStatement();
    if (match(RETURN)) return returnStatement();
    if (match(VAR)) return varStatement();
    if (match(WHILE)) return whileStatement();
    if (check(LEFT_BRACE)) return block();

    // Expression statement.
    Expr expr = expression();
    synchronize(SEMICOLON, "Expect ';' after expression.");
    return new Stmt.Expression(expr);
  }

  private Stmt classStatement() {
    Token name = consume(IDENTIFIER, "Expect class name.");

    Expr superclass = null;
    if (match(LESS)) {
      superclass = primary();
    }

    List<Stmt.Function> methods = new ArrayList<>();
    consume(LEFT_BRACE, "Expect '{' before class body.");

    while (!check(RIGHT_BRACE) && !check(EOF)) {
      methods.add(function("method"));
    }

    synchronize(RIGHT_BRACE, "Expect '}' after class body.");

    return new Stmt.Class(name, superclass, methods);
  }

  private Stmt ifStatement() {
    consume(LEFT_PAREN, "Expect '(' after 'if'.");
    Expr condition = expression();
    consume(RIGHT_PAREN, "Expect ')' after if condition.");

    Stmt thenBranch = statement();
    Stmt elseBranch = null;
    if (match(ELSE)) {
      elseBranch = statement();
    }

    return new Stmt.If(condition, thenBranch, elseBranch);
  }

  private Stmt returnStatement() {
    Expr value = null;
    if (!check(SEMICOLON)) {
      value = expression();
    }

    consume(SEMICOLON, "Expect ';' after return value.");
    return new Stmt.Return(value);
  }

  private Stmt varStatement() {
    Token name = consume(IDENTIFIER, "Expect variable name.");
    synchronize(EQUAL, "Expect '=' after variable name.");
    Expr initializer = expression();
    consume(SEMICOLON, "Expect ';' after variable initializer.");

    return new Stmt.Var(name, initializer);
  }

  private Stmt whileStatement() {
    consume(LEFT_PAREN, "Expect '(' after 'while'.");
    Expr condition = expression();
    consume(RIGHT_PAREN, "Expect '(' after condition.");
    Stmt body = statement();

    return new Stmt.While(condition, body);
  }

  private Stmt.Function function(String kind) {
    Token name = consume(IDENTIFIER,
        "Expect " + kind + " name.");

    consume(LEFT_PAREN, "Expect '(' after " + kind + " name.");
    List<Token> parameters = new ArrayList<>();
    if (!check(RIGHT_PAREN)) {
      do {
        if (parameters.size() >= 8) {
          error("Cannot have more than 8 parameters.");
        }

        parameters.add(consume(IDENTIFIER, "Expect parameter name."));
      } while (match(COMMA));
    }
    synchronize(RIGHT_PAREN, "Expect ')' after parameters.");

    Stmt body = block();
    return new Stmt.Function(name, parameters, body);
  }

  private Stmt block() {
    consume(LEFT_BRACE, "Expect '{' before block.");
    List<Stmt> statements = new ArrayList<>();

    while (!check(RIGHT_BRACE) && !check(EOF)) {
      statements.add(statement());
    }

    synchronize(RIGHT_BRACE, "Expect '}' after block.");

    return new Stmt.Block(statements);
  }

  private Expr expression() {
    return assignment();
  }

  private Expr assignment() {
    Expr expr = or();

  if (match(EQUAL)) {
    Token equals = previous;
    Expr value = assignment();

    if (expr instanceof Expr.Variable) {
      Token name = ((Expr.Variable)expr).name;
      return new Expr.Assign(null, name, value);
    } else if (expr instanceof Expr.Property) {
      Expr.Property property = (Expr.Property)expr;
      return new Expr.Assign(property.object, property.name, value);
    }

    error("Invalid assignment target.", equals);
  }

    return expr;
  }

  private Expr or() {
    Expr expr = and();

    while (match(OR)) {
      Token operator = previous;
      Expr right = and();
      expr = new Expr.Logical(expr, operator, right);
    }

    return expr;
  }

  private Expr and() {
    Expr expr = equality();

    while (match(AND)) {
      Token operator = previous;
      Expr right = equality();
      expr = new Expr.Logical(expr, operator, right);
    }

    return expr;
  }

  private Expr equality() {
    Expr expr = comparison();

    while (match(BANG_EQUAL, EQUAL_EQUAL)) {
      Token operator = previous;
      Expr right = comparison();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr comparison() {
    Expr expr = term();

    while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
      Token operator = previous;
      Expr right = term();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr term() {
    Expr expr = factor();

    while (match(MINUS, PLUS)) {
      Token operator = previous;
      Expr right = factor();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr factor() {
    Expr expr = unary();

    while (match(SLASH, STAR)) {
      Token operator = previous;
      Expr right = unary();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr unary() {
    if (match(BANG, MINUS)) {
      Token operator = previous;
      Expr right = unary();
      return new Expr.Unary(operator, right);
    }

    return call();
  }

  private List<Expr> argumentList() {
    List<Expr> arguments = new ArrayList<>();

    if (!check(RIGHT_PAREN)) {
      do {
        if (arguments.size() >= 8) {
          error("Cannot have more than 8 arguments.");
        }

        arguments.add(expression());
      } while (match(COMMA));
    }

    return arguments;
  }

  private Expr finishCall(Expr callee) {
    List<Expr> arguments = argumentList();
    Token paren = synchronize(RIGHT_PAREN,
        "Expect ')' after argument list.");

    return new Expr.Call(callee, paren, arguments);
  }

  private Expr call() {
    Expr expr = primary();

    while (true) {
      if (match(LEFT_PAREN)) {
        expr = finishCall(expr);
      } else if (match(DOT)) {
        Token name = consume(IDENTIFIER,
            "Expect property name after '.'.");
        expr = new Expr.Property(expr, name);
      } else {
        break;
      }
    }

    return expr;
  }

  private Expr primary() {
    if (match(FALSE)) return new Expr.Literal(false);
    if (match(TRUE)) return new Expr.Literal(true);
    if (match(NULL)) return new Expr.Literal(null);

    if (match(NUMBER, STRING)) {
      return new Expr.Literal(previous.value);
    }

    if (match(SUPER)) {
      consume(DOT, "Expect '.' after 'super'.");
      Token method = consume(IDENTIFIER, "Expect superclass method name.");
      return new Expr.Super(method);
    }

    if (match(THIS)) return new Expr.This(previous);

    if (match(IDENTIFIER)) {
      return new Expr.Variable(previous);
    }

    if (match(LEFT_PAREN)) {
      Expr expr = expression();
      consume(RIGHT_PAREN, "Expect ')' after expression.");
      return new Expr.Grouping(expr);
    }

    error("Unexpected token '" + current.text + "'.");
    return null;
  }

  private boolean match(TokenType... types) {
    for (TokenType type : types) {
      if (check(type)) {
        advance();
        return true;
      }
    }

    return false;
  }

  private Token consume(TokenType type, String message) {
    if (check(type)) return advance();
    error(message);
    return null;
  }

  // TODO: Can we unify this with consume() by assuming certain token types
  // (right delimiters, "=", and ";"?) are always synchronized?
  private Token synchronize(TokenType type, String message) {
    if (check(type)) return advance();
    error(message);

    while (current.type != type && current.type != EOF) {
      advance();
    }

    return advance();
  }

  private Token advance() {
    // Report and skip any error tokens.
    // TODO: Move this to Scanner.
    for (;;) {
      previous = current;
      current = scanner.scanToken();

      if (current.type != ERROR) break;
      error((String)current.value);
    }

    return previous;
  }

  // Returns true if the current token is of tokenType, but
  // does not consume it.
  private boolean check(TokenType tokenType) {
    return current.type == tokenType;
  }

  private void error(String message) {
    error(message, current);
  }

  private void error(String message, Token token) {
    errorReporter.error(token, message);
  }
}
