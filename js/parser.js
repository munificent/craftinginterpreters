"use strict";

var Token = require("./token");

// TODO: Use them qualified?
var ast = require("./ast"),
    Expr = ast.Expr,
    Program = ast.Program,
    AssignExpr = ast.AssignExpr,
    BinaryExpr = ast.BinaryExpr,
    CallExpr = ast.CallExpr,
    LogicalExpr = ast.LogicalExpr,
    NumberExpr = ast.NumberExpr,
    PropertyExpr = ast.PropertyExpr,
    StringExpr = ast.StringExpr,
    UnaryExpr = ast.UnaryExpr,
    VariableExpr = ast.VariableExpr,
    Stmt = ast.Stmt,
    BlockStmt = ast.BlockStmt,
    ClassStmt = ast.ClassStmt,
    ExpressionStmt = ast.ExpressionStmt,
    FunStmt = ast.FunStmt,
    IfStmt = ast.IfStmt,
    ReturnStmt = ast.ReturnStmt,
    VarStmt = ast.VarStmt,
    WhileStmt = ast.WhileStmt;

function Parser(lexer, errorReporter) {
  if (errorReporter === undefined) {
    errorReporter = {
      error: function(message) {
        console.log("Error: " + message);
      }
    };
  }

  this.lexer = lexer;
  this.errorReporter = errorReporter;
  this.current = null;
  this.last = null;

  this.errorReporter.hasError = false;
  this.needsMoreInput = false;
}

Parser.prototype.parseProgram = function() {
  var statements = [];
  while (!this.peek(Token.end)) {
    statements.push(this.statement());
  }

  return new Program(statements);
}

Parser.prototype.parse = function() {
  return this.statement();

  // TODO: Consume end.
}

Parser.prototype.statement = function() {
  // TODO: Other statements.

  // Class declaration.
  if (this.match(Token.class_)) {
    var name = this.consume(Token.identifier);

    var superclass = null;
    if (this.match(Token.less)) {
      superclass = this.primary();
    }

    var methods = [];
    this.consume(Token.leftBrace);

    while (!this.peek(Token.rightBrace) && !this.peek(Token.end)) {
      methods.push(this.fun());
    }

    this.consume(Token.rightBrace, "Expect '}' after class body.");

    return new ClassStmt(name.text, superclass, methods);
  }

  // Function declaration.
  if (this.match(Token.fun)) {
    return this.fun();
  }

  // If.
  if (this.match(Token.if_)) {
    this.consume(Token.leftParen);
    var condition = this.expression();
    this.consume(Token.rightParen);
    var thenBranch = this.statement();
    var elseBranch = null;
    if (this.match(Token.else_)) {
      elseBranch = this.statement();
    }

    return new IfStmt(condition, thenBranch, elseBranch);
  }

  // Return.
  if (this.match(Token.return_)) {
    var value = null;
    if (!this.peek(Token.semicolon)) {
      value = this.expression();
    }

    this.consume(Token.semicolon, "Expect ';' after return value.");
    return new ReturnStmt(value);
  }

  // Variable declaration.
  if (this.match(Token.var_)) {
    var name = this.consume(Token.identifier).text;

    // TODO: Make this optional.
    this.consume(Token.equal);
    var initializer = this.expression();
    this.consume(Token.semicolon);

    return new VarStmt(name, initializer);
  }

  // While.
  if (this.match(Token.while_)) {
    this.consume(Token.leftParen);
    var condition = this.expression();
    this.consume(Token.rightParen);
    var body = this.statement();

    return new WhileStmt(condition, body);
  }

  // Block.
  if (this.peek(Token.leftBrace)) {
    return this.block();
  }

  // Expression statement.
  var expr = this.expression();
  this.consume(Token.semicolon, "Expect ';' after expression.");
  return new ExpressionStmt(expr);
}

Parser.prototype.fun = function() {
  var name = this.consume(Token.identifier);

  this.consume(Token.leftParen);
  var parameters = [];
  if (!this.peek(Token.rightParen)) {
    do {
      var parameter = this.consume(Token.identifier);
      parameters.push(parameter.text);
    } while (this.match(Token.comma));
  }
  this.consume(Token.rightParen);

  var body = this.block();
  return new FunStmt(name.text, parameters, body);
}

Parser.prototype.block = function() {
  this.consume(Token.leftBrace);
  var statements = [];

  while (!this.peek(Token.rightBrace) && !this.peek(Token.end)) {
    statements.push(this.statement());
  }

  this.consume(Token.rightBrace, "Expect '}' after block.");

  return new BlockStmt(statements);
}

Parser.prototype.expression = function() {
  return this.assignment();
}

Parser.prototype.assignment = function() {
  var expr = this.or();

  if (this.match(Token.equal)) {
    // Check that the left-hand side is a valid target.
    if (!(expr instanceof VariableExpr) &&
        !(expr instanceof PropertyExpr)) {
      this.error("Invalid assignment target.");
    }

    var value = this.assignment();
    return new AssignExpr(expr, value);
  }

  return expr;
}

Parser.prototype.or = function() {
  var expr = this.and();

  while (this.match(Token.or)) {
    var op = this.last.type;
    var right = this.and();
    expr = new LogicalExpr(expr, op, right);
  }

  return expr;
}

Parser.prototype.and = function() {
  var expr = this.equality();

  while (this.match(Token.and)) {
    var op = this.last.type;
    var right = this.equality();
    expr = new LogicalExpr(expr, op, right);
  }

  return expr;
}

Parser.prototype.equality = function() {
  var expr = this.comparison();

  while (this.match(Token.equalEqual) ||
         this.match(Token.bangEqual)) {
    var op = this.last.type;
    var right = this.comparison();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
}

Parser.prototype.comparison = function() {
  var expr = this.term();

  while (this.match(Token.less) ||
         this.match(Token.greater) ||
         this.match(Token.lessEqual) ||
         this.match(Token.greaterEqual)) {
    var op = this.last.type;
    var right = this.term();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
}

Parser.prototype.term = function() {
  var expr = this.factor();

  while (this.match(Token.plus) ||
         this.match(Token.minus)) {
    var op = this.last.type;
    var right = this.factor();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
}

Parser.prototype.factor = function() {
  var expr = this.unary();

  while (this.match(Token.star) ||
         this.match(Token.slash) ||
         this.match(Token.percent)) {
    var op = this.last.type;
    var right = this.unary();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;

  // TODO: Could use code like this for all of the binary operators instead.
  /*
  return this.binary(this.unary,
      [Token.star, Token.slash, Token.percent]);
  */
}

/*
Parser.prototype.binary = function(parseOperand, operators) {
  var expr = parseOperand.call(this);

  while (this.matchAny(operators)) {
    var op = this.last.type;
    var right = parseOperand.call(this);
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
}
*/

Parser.prototype.unary = function() {
  if (this.match(Token.plus) ||
      this.match(Token.minus) ||
      this.match(Token.bang)) {
    var op = this.last.type;
    var right = this.unary();
    return new UnaryExpr(op, right);
  }

  return this.call();
}

Parser.prototype.call = function() {
  var expr = this.primary();

  while (true) {
    if (this.match(Token.leftParen)) {
      var args = [];

      if (this.match(Token.rightParen)) {
        // No arguments.
      } else {
        do {
          args.push(this.expression());
        } while (this.match(Token.comma));

        this.consume(Token.rightParen);
      }

      expr = new CallExpr(expr, args);
    } else if (this.match(Token.dot)) {
      var name = this.consume(Token.identifier);
      expr = new PropertyExpr(expr, name.text);
    } else {
      break;
    }
  }

  return expr;
}

Parser.prototype.primary = function() {
  // TODO: Switch on type?

  if (this.match(Token.number)) {
    return new NumberExpr(this.last.value);
  }

  if (this.match(Token.string)) {
    return new StringExpr(this.last.value);
  }

  if (this.match(Token.identifier)) {
    return new VariableExpr(this.last.text);
  }

  if (this.match(Token.leftParen)) {
    var expr = this.expression();
    this.consume(Token.rightParen);
    return expr;
  }

  // TODO: Error handling.
}

/*
Parser.prototype.matchAny = function(tokenTypes) {
  for (var i = 0; i < tokenTypes.length; i++) {
    if (this.match(tokenTypes[i])) return true;
  }

  return false;
}
*/

Parser.prototype.match = function(tokenType) {
  if (!this.peek(tokenType)) return false;

  this.last = this.current;
  this.current = this.lexer.nextToken();
  return true;
}

Parser.prototype.consume = function(tokenType, message) {
  if (!this.peek(tokenType)) {
    if (message === undefined) {
      message = "Expected " + tokenType + " got " + this.current.type;
    }

    // If the first error happened because we unexpectedly hit the end of the
    // input, let the caller know.
    if (!this.errorReporter.hasError && this.current.type == Token.end) {
      this.errorReporter.needsMoreInput = true;
    }

    this.error(message);
  }

  this.last = this.current;
  this.current = null;
  return this.last;
}

// Returns true if the current token is of tokenType, but does not consume it.
Parser.prototype.peek = function(tokenType) {
  if (this.current == null) this.current = this.lexer.nextToken();

  return this.current.type == tokenType;
}

Parser.prototype.error = function(message) {
  this.errorReporter.error(message);
  this.errorReporter.hasError = true;
}

module.exports = Parser;
