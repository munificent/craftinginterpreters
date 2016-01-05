"use strict";

var Token = require("./token");

// TODO: Use them qualified?
var ast = require("./ast");
var Expr = ast.Expr;
var BinaryExpr = ast.BinaryExpr;
var CallExpr = ast.CallExpr;
var NumberExpr = ast.NumberExpr;
var StringExpr = ast.StringExpr;
var UnaryExpr = ast.UnaryExpr;
var VariableExpr = ast.VariableExpr;
var Stmt = ast.Stmt;
var BlockStmt = ast.BlockStmt;
var ExpressionStmt = ast.ExpressionStmt;

function Parser(lexer) {
  this.lexer = lexer;
  this.current = null;
  this.last = null;
}

Parser.prototype.parseProgram = function() {
  return this.expression();

  // TODO: Consume end.
}

Parser.prototype.expression = function() {
  return this.equality();
}

Parser.prototype.equality = function() {
  var expr = this.comparison();

  while (this.match(Token.equalEqual) || this.match(Token.bangEqual)) {
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

  while (this.match(Token.plus) || this.match(Token.minus)) {
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
}

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

  while (this.match(Token.leftParen)) {
    // TODO: Comma-separated list.
    var args = [];

    if (this.match(Token.rightParen)) {
      // No arguments.
    } else {
      do {
        args.push(this.expression());
      } while (this.match(Token.comma));

      // TODO: Consume and error if missing.
      this.match(Token.rightParen);
    }

    expr = new CallExpr(expr, args);
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

Parser.prototype.match = function(tokenType) {
  if (this.current == null) this.current = this.lexer.nextToken();

  if (this.current.type != tokenType) return false;

  this.last = this.current;
  this.current = this.lexer.nextToken();
  return true;
}

Parser.prototype.consume = function(tokenType) {
  if (this.current == null) this.current = this.lexer.nextToken();

  if (this.current.type != tokenType) {
    // TODO: Report error better.
    console.log("Error! Expect " + tokenType + " got " + this.current.type);
  }

  this.last = this.current;
  this.current = this.lexer.nextToken();
  return this.last;
}

module.exports = Parser;
