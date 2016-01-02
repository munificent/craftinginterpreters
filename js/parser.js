var Token = require("./token");

// TODO: Use them qualified?
var ast = require("./ast");
var Expr = ast.Expr;
var BinaryExpr = ast.BinaryExpr;
var CallExpr = ast.CallExpr;
var NumberExpr = ast.NumberExpr;
var StringExpr = ast.StringExpr;
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

  while (this.match(Token.EQUALS_EQUALS) || this.match(Token.BANG_EQUALS)) {
    var op = this.last.type;
    var right = this.comparison();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
}

Parser.prototype.comparison = function() {
  var expr = this.term();

  while (this.match(Token.LESS) ||
         this.match(Token.GREATER) ||
         this.match(Token.LESS_EQUALS) ||
         this.match(Token.GREATER_EQUALS)) {
    var op = this.last.type;
    var right = this.term();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
}

Parser.prototype.term = function() {
  var expr = this.factor();

  while (this.match(Token.PLUS) || this.match(Token.MINUS)) {
    var op = this.last.type;
    var right = this.factor();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
}

Parser.prototype.factor = function() {
  var expr = this.call();

  while (this.match(Token.STAR) ||
         this.match(Token.SLASH) ||
         this.match(Token.PERCENT)) {
    var op = this.last.type;
    var right = this.call();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
}

Parser.prototype.call = function() {
  var expr = this.primary();

  while (this.match(Token.LEFT_PAREN)) {
    // TODO: Comma-separated list.
    var args = [];

    if (this.match(Token.RIGHT_PAREN)) {
      // No arguments.
    } else {
      do {
        args.push(this.expression());
      } while (this.match(Token.COMMA));

      // TODO: Consume and error if missing.
      this.match(Token.RIGHT_PAREN);
    }

    expr = new CallExpr(expr, args);
  }

  return expr;
}

Parser.prototype.primary = function() {
  // TODO: Switch on type?

  if (this.match(Token.NUMBER)) {
    return new NumberExpr(this.last.value);
  }

  if (this.match(Token.STRING)) {
    return new StringExpr(this.last.value);
  }

  if (this.match(Token.IDENTIFIER)) {
    return new VariableExpr(this.last.text);
  }

  if (this.match(Token.LEFT_PAREN)) {
    var expr = this.expression();
    this.consume(Token.RIGHT_PAREN);
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
