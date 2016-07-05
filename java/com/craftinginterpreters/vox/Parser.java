//>= Parsing Expressions
package com.craftinginterpreters.vox;

//>= Variables
import java.util.ArrayList;
//>= Parsing Expressions
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import static com.craftinginterpreters.vox.TokenType.*;

class Parser {
  private static final Set<TokenType> synchronizing = new HashSet<>();

  static {
    synchronizing.add(LEFT_BRACE);
    synchronizing.add(RIGHT_BRACE);
    synchronizing.add(RIGHT_BRACKET);
    synchronizing.add(RIGHT_PAREN);
    synchronizing.add(EQUAL);
    synchronizing.add(SEMICOLON);
  }

  private final List<Token> tokens;
  private int currentIndex = 0;
  private final ErrorReporter errorReporter;

  Parser(List<Token> tokens, ErrorReporter errorReporter) {
    this.tokens = tokens;
    this.errorReporter = errorReporter;
  }
//>= Variables

  List<Stmt> parseProgram() {
    List<Stmt> statements = new ArrayList<>();
    while (!isAtEnd()) {
      statements.add(statement());
    }

    return statements;
  }
//>= Parsing Expressions

  Expr parseExpression() {
//>= Variables
    return assignment();
/*>= Parsing Expressions <= Interpreting ASTs
    return equality();
*/
//>= Parsing Expressions
  }
//>= Variables

  private Stmt statement() {
//>= Classes
    if (match(CLASS)) return classStatement();
//>= Functions
    if (match(FUN)) return function("function");
//>= Control Flow
    if (match(IF)) return ifStatement();
//>= Functions
    if (match(RETURN)) return returnStatement();
//>= Variables
    if (match(VAR)) return varStatement();
//>= Control Flow
    if (match(WHILE)) return whileStatement();
//>= Variables
    if (check(LEFT_BRACE)) return block();

    // Expression statement.
    Expr expr = parseExpression();
    consume(SEMICOLON, "Expect ';' after expression.");
    return new Stmt.Expression(expr);
  }
//>= Classes

  private Stmt classStatement() {
    Token name = consume(IDENTIFIER, "Expect class name.");
//>= Inheritance

    Expr superclass = null;
    if (match(LESS)) {
      consume(IDENTIFIER, "Expect superclass name.");
      superclass = new Expr.Variable(previous());
    }
//>= Classes

    List<Stmt.Function> methods = new ArrayList<>();
    consume(LEFT_BRACE, "Expect '{' before class body.");

    while (!check(RIGHT_BRACE) && !isAtEnd()) {
      methods.add(function("method"));
    }

    consume(RIGHT_BRACE, "Expect '}' after class body.");

    return new Stmt.Class(name, superclass, methods);
  }
//>= Control Flow

  private Stmt ifStatement() {
    consume(LEFT_PAREN, "Expect '(' after 'if'.");
    Expr condition = parseExpression();
    consume(RIGHT_PAREN, "Expect ')' after if condition.");

    Stmt thenBranch = statement();
    Stmt elseBranch = null;
    if (match(ELSE)) {
      elseBranch = statement();
    }

    return new Stmt.If(condition, thenBranch, elseBranch);
  }
//>= Functions

  private Stmt returnStatement() {
    Token keyword = previous();
    Expr value = null;
    if (!check(SEMICOLON)) {
      value = parseExpression();
    }

    consume(SEMICOLON, "Expect ';' after return value.");
    return new Stmt.Return(keyword, value);
  }
//>= Variables

  private Stmt varStatement() {
    Token name = consume(IDENTIFIER, "Expect variable name.");

    Expr initializer = null;
    if (match(EQUAL)) {
      initializer = parseExpression();
    }

    consume(SEMICOLON, "Expect ';' after variable declaration.");

    return new Stmt.Var(name, initializer);
  }
//>= Control Flow

  private Stmt whileStatement() {
    consume(LEFT_PAREN, "Expect '(' after 'while'.");
    Expr condition = parseExpression();
    consume(RIGHT_PAREN, "Expect '(' after condition.");
    Stmt body = statement();

    return new Stmt.While(condition, body);
  }
//>= Functions

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
    consume(RIGHT_PAREN, "Expect ')' after parameters.");

    Stmt body = block();
    return new Stmt.Function(name, parameters, body);
  }
//>= Variables

  private Stmt block() {
    consume(LEFT_BRACE, "Expect '{' before block.");
    List<Stmt> statements = new ArrayList<>();

    while (!check(RIGHT_BRACE) && !isAtEnd()) {
      statements.add(statement());
    }

    consume(RIGHT_BRACE, "Expect '}' after block.");

    return new Stmt.Block(statements);
  }

  private Expr assignment() {
/*>= Variables <= Closures
    Expr expr = equality();
*/
//>= Control Flow
    Expr expr = or();
//>= Variables

  if (match(EQUAL)) {
    Token equals = previous();
    Expr value = assignment();

    if (expr instanceof Expr.Variable) {
      Token name = ((Expr.Variable)expr).name;
      return new Expr.Assign(null, name, value);
//>= Classes
    } else if (expr instanceof Expr.Property) {
      Expr.Property property = (Expr.Property)expr;
      return new Expr.Assign(property.object, property.name, value);
//>= Variables
    }

    error("Invalid assignment target.", equals);
  }

    return expr;
  }
//>= Control Flow

  private Expr or() {
    Expr expr = and();

    while (match(OR)) {
      Token operator = previous();
      Expr right = and();
      expr = new Expr.Logical(expr, operator, right);
    }

    return expr;
  }

  private Expr and() {
    Expr expr = equality();

    while (match(AND)) {
      Token operator = previous();
      Expr right = equality();
      expr = new Expr.Logical(expr, operator, right);
    }

    return expr;
  }
//>= Parsing Expressions

  private Expr equality() {
    Expr expr = comparison();

    while (match(BANG_EQUAL, EQUAL_EQUAL)) {
      Token operator = previous();
      Expr right = comparison();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr comparison() {
    Expr expr = term();

    while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
      Token operator = previous();
      Expr right = term();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr term() {
    Expr expr = factor();

    while (match(MINUS, PLUS)) {
      Token operator = previous();
      Expr right = factor();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr factor() {
    Expr expr = unary();

    while (match(SLASH, STAR)) {
      Token operator = previous();
      Expr right = unary();
      expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr unary() {
    if (match(BANG, MINUS)) {
      Token operator = previous();
      Expr right = unary();
      return new Expr.Unary(operator, right);
    }

//>= Functions
    return call();
/*>= Parsing Expressions <= Variables
    return primary();
*/
//>= Parsing Expressions
  }
//>= Functions

  private List<Expr> argumentList() {
    List<Expr> arguments = new ArrayList<>();

    if (!check(RIGHT_PAREN)) {
      do {
        if (arguments.size() >= 8) {
          error("Cannot have more than 8 arguments.");
        }

        arguments.add(parseExpression());
      } while (match(COMMA));
    }

    return arguments;
  }

  private Expr finishCall(Expr callee) {
    List<Expr> arguments = argumentList();
    Token paren = consume(RIGHT_PAREN,
        "Expect ')' after arguments.");

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
//>= Parsing Expressions

  private Expr primary() {
    if (match(FALSE)) return new Expr.Literal(false);
    if (match(TRUE)) return new Expr.Literal(true);
    if (match(NIL)) return new Expr.Literal(null);

    if (match(NUMBER, STRING)) {
      return new Expr.Literal(previous().value);
    }

//>= Inheritance
    if (match(SUPER)) {
      Token keyword = previous();
      consume(DOT, "Expect '.' after 'super'.");
      Token method = consume(IDENTIFIER,
          "Expect superclass method name.");
      return new Expr.Super(keyword, method);
    }
//>= Classes

    if (match(THIS)) return new Expr.This(previous());
//>= Variables

    if (match(IDENTIFIER)) {
      return new Expr.Variable(previous());
    }
//>= Parsing Expressions

    if (match(LEFT_PAREN)) {
      Expr expr = parseExpression();
      consume(RIGHT_PAREN, "Expect ')' after expression.");
      return new Expr.Grouping(expr);
    }

    error("Unexpected token '" + current().text + "'.");
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

    if (!synchronizing.contains(type)) return null;

    while (!check(type) && !isAtEnd()) {
      advance();
    }

    return advance();
  }

  private Token advance() {
    if (!isAtEnd()) currentIndex++;
    return previous();
  }

  // Returns true if the current token is of tokenType, but
  // does not consume it.
  private boolean check(TokenType tokenType) {
    if (isAtEnd()) return false;
    return current().type == tokenType;
  }

  private boolean isAtEnd() {
    return current().type == EOF;
  }

  private Token current() {
    return tokens.get(currentIndex);
  }

  private Token previous() {
    return tokens.get(currentIndex - 1);
  }

  private void error(String message) {
    error(message, current());
  }

  private void error(String message, Token token) {
    errorReporter.error(token, message);
  }
}
