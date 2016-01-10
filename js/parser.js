"use strict";

var Token = require("./token");

// TODO: Use them qualified?
var ast = require("./ast");
var Expr = ast.Expr;
var AssignExpr = ast.AssignExpr;
var BinaryExpr = ast.BinaryExpr;
var CallExpr = ast.CallExpr;
var LogicalExpr = ast.LogicalExpr;
var NumberExpr = ast.NumberExpr;
var PropertyExpr = ast.PropertyExpr;
var StringExpr = ast.StringExpr;
var UnaryExpr = ast.UnaryExpr;
var VariableExpr = ast.VariableExpr;
var Stmt = ast.Stmt;
var BlockStmt = ast.BlockStmt;
var ExpressionStmt = ast.ExpressionStmt;
var IfStmt = ast.IfStmt;
var VarStmt = ast.VarStmt;
var WhileStmt = ast.WhileStmt;

function Parser(lexer) {
  this.lexer = lexer;
  this.current = null;
  this.last = null;
}

Parser.prototype.parse = function() {
  return this.statement();

  // TODO: Consume end.
}

Parser.prototype.statement = function() {
  // TODO: Other statements.

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
  if (this.match(Token.leftBrace)) {
    var statements = [];

    while (!this.match(Token.rightBrace)) {
      statements.push(this.statement());
    }

    return new BlockStmt(statements);
  }

  // Expression statement.
  var expr = this.expression();
  this.consume(Token.semicolon);
  return new ExpressionStmt(expr);
}

Parser.prototype.expression = function() {
  return this.assignment();
}

Parser.prototype.assignment = function() {
  var expr = this.or();

  if (this.match(Token.equal)) {
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
  if (this.peek() != tokenType) return false;

  this.last = this.current;
  this.current = this.lexer.nextToken();
  return true;
}

Parser.prototype.consume = function(tokenType) {
  if (this.peek() != tokenType) {
    // TODO: Report error better.
    console.log("Error! Expect " + tokenType + " got " + this.peek());
  }

  this.last = this.current;
  this.current = null;
  return this.last;
}

// Returns the type of the current token.
Parser.prototype.peek = function() {
  if (this.current == null) this.current = this.lexer.nextToken();

  return this.current.type;
}

module.exports = Parser;
