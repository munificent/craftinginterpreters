(function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);var f=new Error("Cannot find module '"+o+"'");throw f.code="MODULE_NOT_FOUND",f}var l=n[o]={exports:{}};t[o][0].call(l.exports,function(e){var n=t[o][1][e];return s(n?n:e)},l,l.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({1:[function(require,module,exports){
"use strict";

function Expr() {}
function Stmt() {}

exports.Expr = Expr;
exports.Stmt = Stmt;

function defineAst(name, baseClass, baseName, fields) {
  var constructor = function() {
    for (var i = 0; i < fields.length; i++) {
      this[fields[i]] = arguments[i];
    }
  }

  constructor.prototype = Object.create(baseClass.prototype);

  constructor.prototype.accept = function(visitor) {
    return visitor["visit" + name + baseName](this);
  }

  exports[name + baseName] = constructor;
}

function defineExpr(name, fields) {
  defineAst(name, Expr, "Expr", fields);
}

function defineStmt(name, fields) {
  defineAst(name, Stmt, "Stmt", fields);
}

defineExpr("Binary",      ["left", "op", "right"]);
defineExpr("Call",        ["fn, args"]);
defineExpr("Number",      ["value"]);
defineExpr("String",      ["value"]);
defineExpr("Unary",       ["op", "right"]);
defineExpr("Variable",    ["name"]);

defineStmt("Block",       ["statements"]);
defineStmt("Expression",  ["expression"]);
defineStmt("For",         ["name", "iterator", "body"]);
defineStmt("If",          ["condition", "thenBranch", "elseBranch"]);
defineStmt("Var",         ["name", "initializer"]);
defineStmt("While",       ["condition", "body"]);

/*
function Expr() {
}

function BinaryExpr(left, op, right) {
  Expr.call(this);
  this.left = left;
  this.op = op;
  this.right = right;
}

BinaryExpr.prototype = Object.create(Expr.prototype);

BinaryExpr.prototype.accept = function(visitor) {
  return visitor.visitBinaryExpr(this);
}

function CallExpr(fn, args) {
  Expr.call(this);
  this.fn = fn;
  this.args = args;
}

CallExpr.prototype = Object.create(Expr.prototype);

CallExpr.prototype.accept = function(visitor) {
  return visitor.visitCallExpr(this);
}

function NumberExpr(value) {
  Expr.call(this);
  this.value = value;
}

NumberExpr.prototype = Object.create(Expr.prototype);

NumberExpr.prototype.accept = function(visitor) {
  return visitor.visitNumberExpr(this);
}

function StringExpr(value) {
  Expr.call(this);
  this.value = value;
}

StringExpr.prototype = Object.create(Expr.prototype);

StringExpr.prototype.accept = function(visitor) {
  return visitor.visitStringExpr(this);
}

function UnaryExpr(op, right) {
  Expr.call(this);
  this.op = op;
  this.right = right;
}

UnaryExpr.prototype = Object.create(Expr.prototype);

UnaryExpr.prototype.accept = function(visitor) {
  return visitor.visitUnaryExpr(this);
}

function VariableExpr(name) {
  Expr.call(this);
  this.name = name;
}

VariableExpr.prototype = Object.create(Expr.prototype);

VariableExpr.prototype.accept = function(visitor) {
  return visitor.visitVariableExpr(this);
}

function Stmt() {
}

function BlockStmt(statements) {
  Stmt.call(this);
  this.statements = statements;
}

BlockStmt.prototype = Object.create(Stmt.prototype);

BlockStmt.prototype.accept = function(visitor) {
  return visitor.visitBlockStmt(this);
}

function ExpressionStmt(expression) {
  Stmt.call(this);
  this.expression = expression;
}

ExpressionStmt.prototype = Object.create(Stmt.prototype);

ExpressionStmt.prototype.accept = function(visitor) {
  return visitor.visitExpressionStmt(this);
}

exports.Expr = Expr;
exports.BinaryExpr = BinaryExpr;
exports.CallExpr = CallExpr;
exports.NumberExpr = NumberExpr;
exports.StringExpr = StringExpr;
exports.UnaryExpr = UnaryExpr;
exports.VariableExpr = VariableExpr;
exports.Stmt = Stmt;
exports.BlockStmt = BlockStmt;
exports.ExpressionStmt = ExpressionStmt;
*/

},{}],2:[function(require,module,exports){
"use strict";

var Token = require("./token");

// Returns true if `c` is an English letter or underscore.
function isAlpha(c) {
  return (c >= "a" && c <= "z") ||
         (c >= "A" && c <= "Z") ||
         c == "_";
}

// Returns true if `c` is an English letter, underscore, or digit.
function isAlphaNumeric(c) {
  return isAlpha(c) || isDigit(c);
}

// Returns true if `c` is a space, newline, or tab.
function isWhitespace(c) {
  return c == " " || c == "\n" || c == "\t";
}

// Returns true if `c` is a digit.
function isDigit(c) {
  return c >= "0" && c <= "9";
}

function Lexer(source) {
  this.source = source;
  this.start = 0;
  this.current = 0;
}

Lexer.punctuators = {
  "(": Token.leftParen,
  ")": Token.rightParen,
  "[": Token.leftBracket,
  "]": Token.rightBracket,
  "{": Token.leftBrace,
  "}": Token.rightBrace,
  ";": Token.semicolon,
  ",": Token.comma,
};

Lexer.prototype.nextToken = function() {
  this.skipWhitespace();

  if (this.current >= this.source.length) return new Token(Token.end, "");

  var c = this.advance();

  // See if it's a punctuator.
  var type = Lexer.punctuators[c];
  if (type != undefined) return this.makeToken(type);

  if (isAlpha(c)) return this.identifier();
  if (isDigit(c)) return this.number();

  switch (c) {
    case "\"": return this.string();
    case "+": return this.makeToken(Token.plus);
    case "-": return this.makeToken(Token.minus);
    case "*": return this.makeToken(Token.star);
    case "/": return this.makeToken(Token.slash);
    case "%": return this.makeToken(Token.percent);
    case "!":
      if (this.match("=")) return this.makeToken(Token.bangEqual);
      return this.makeToken(Token.bang);

    case ".":
      if (isDigit(this.peek())) return this.number();

      return this.makeToken(Token.dot);

    case "=":
      if (this.match("=")) return this.makeToken(Token.equalEqual);
      return this.makeToken(Token.equal);

    case "<":
      if (this.match("=")) return this.makeToken(Token.lessEqual);
      return this.makeToken(Token.less);

    case ">":
      if (this.match("=")) return this.makeToken(Token.greaterEqual);
      return this.makeToken(Token.greater);
  }

  return this.makeToken(Token.error);
}

Lexer.prototype.skipWhitespace = function() {
  this.consumeWhile(isWhitespace);

  // Don't include these characters in the next token.
  this.start = this.current;
}

Lexer.prototype.identifier = function() {
  this.consumeWhile(isAlphaNumeric);
  return this.makeToken(Token.identifier);
}

Lexer.prototype.number = function() {
  this.consumeWhile(isDigit);

  // Look for a fractional part.
  if (this.peek() == "." && !isAlpha(this.peek(1))) {
    // Consume the "."
    this.advance();

    this.consumeWhile(isDigit);
  }

  return this.makeToken(Token.number, parseFloat);
}

Lexer.prototype.string = function() {
  // TODO: Escapes.
  // TODO: What about newlines?
  this.consumeWhile(function(c) { return c != "\""; });

  // Unterminated string.
  if (this.isAtEnd()) return this.makeToken(Token.error);

  // The closing ".
  this.advance();

  return this.makeToken(Token.string, function(text) {
    // Trim the surrounding quotes.
    return text.substring(1, text.length - 1);
  });
}

Lexer.prototype.makeToken = function(type, valueFn) {
  var text = this.source.substring(this.start, this.current);
  var value = null;

  if (valueFn != undefined) value = valueFn(text);
  var token = new Token(type, text, value);

  this.start = this.current;
  return token;
}

Lexer.prototype.consumeWhile = function(matcher) {
  while (!this.isAtEnd() && matcher(this.peek())) this.advance();
}

Lexer.prototype.match = function(expected) {
  if (this.current >= this.source.length) return false;
  if (this.source[this.current] != expected) return false;

  this.current++;
  return true;
}

Lexer.prototype.advance = function() {
  this.current++;
  return this.source[this.current - 1];
}

Lexer.prototype.peek = function(ahead) {
  if (ahead === undefined) ahead = 0;
  if (this.current + ahead >= this.source.length) return "";

  return this.source[this.current + ahead];
}

Lexer.prototype.isAtEnd = function() {
  return this.current >= this.source.length;
}

module.exports = Lexer;

},{"./token":5}],3:[function(require,module,exports){
"use strict";

var Lexer = require("./lexer");
var Parser = require("./parser");
var Token = require("./token");

window.onload = function() {
  var input = document.querySelector("textarea#input");
  if (input.addEventListener) {
    input.addEventListener("input", refresh, false);
  } else if (input.attachEvent) {
    input.attachEvent("onpropertychange", refresh);
  }

  refresh();
};

function refresh() {
  var input = document.querySelector("textarea#input");

  displayTokens(input.value);

  var lexer = new Lexer(input.value);
  var parser = new Parser(lexer);
  var node = parser.parseProgram();

  displayAst(node);
  evaluateAst(node);
}

function displayTokens(source) {
  var lexer = new Lexer(input.value);
  var tokens = [];
  while (true) {
    var token = lexer.nextToken();
    tokens.push(token);
    if (token.type == Token.end) break;
  }

  var html = "";
  for (var i = 0; i < tokens.length; i++) {
    var token = tokens[i];
    html += "<span class='token " + token.type.toLowerCase() + "'>";
    if (token.value != null) {
      html += "<span class='value'>" + token.value + "</span>";
    } else {
      html += token.text;
    }
    html += "</span>";
  }

  document.querySelector("#tokens").innerHTML = html;
}

function displayAst(node) {

  var html = "<ul><li>" + astToHtml(node) + "</li></ul>";
  document.querySelector("#ast").innerHTML = html;
}

function astToHtml(node) {
  if (node === undefined) return "ERROR";

  return node.accept({
    visitBinaryExpr: function(node) {
      var html = "<span class='node'>" + node.op + "</span>";
      html += "<ul>";
      html += "<li>" + astToHtml(node.left) + "</li>";
      html += "<li>" + astToHtml(node.right) + "</li>";
      html += "</ul>";
      return html;
    },
    visitCallExpr: function(node) {
      var html = "<span class='node'>call</span>";
      html += "<ul>";
      html += "<li>" + astToHtml(node.fn) + "</li>";

      for (var i = 0; i < node.args.length; i++) {
        html += "<li>" + astToHtml(node.args[i]) + "</li>";
      }

      html += "</ul>";
      return html;
    },
    visitNumberExpr: function(node) {
      return "<span class='node number'>" + node.value + "</span>";
    },
    visitStringExpr: function(node) {
      return "<span class='node string'>" + node.value + "</span>";
    },
    visitUnaryExpr: function(node) {
      var html = "<span class='node'>" + node.op + "</span>";
      html += "<ul>";
      html += "<li>" + astToHtml(node.right) + "</li>";
      html += "</ul>";
      return html;
    },
    visitVariableExpr: function(node) {
      return "<span class='node var'>" + node.name + "</span>";
    }
  });
}

function evaluateAst(node) {
  var result = evaluate(node);
  document.querySelector("#evaluate").innerHTML = result.toString();
}

function evaluate(node) {
  return node.accept({
    visitBinaryExpr: function(node) {
      var left = evaluate(node.left);
      var right = evaluate(node.right);

      // TODO: Don't always use JS semantics.
      switch (node.op) {
        case Token.plus: return left + right;
        case Token.minus: return left - right;
        case Token.star: return left * right;
        case Token.slash: return left / right;
        case Token.percent: return left % right;
        case Token.equalEqual: return left === right;
        case Token.bangEqual: return left !== right;
        case Token.less: return left < right;
        case Token.greater: return left > right;
        case Token.lessEqual: return left <= right;
        case Token.greaterEqual: return left >= right;
      }

      throw "unknown operator " + node.op;
    },
    visitCallExpr: function(node) {
      throw "call not implemented";
    },
    visitNumberExpr: function(node) {
      return node.value;
    },
    visitStringExpr: function(node) {
      return node.value;
    },
    visitUnaryExpr: function(node) {
      var right = evaluate(node.right);

      // TODO: Don't always use JS semantics.
      switch (node.op) {
        case Token.plus: return +right;
        case Token.minus: return -right;
        case Token.bang: return !right;
      }

      throw "unknown operator " + node.op;
    },
    visitVariableExpr: function(node) {
      throw "variable not implemented";
    }
  });
}

},{"./lexer":2,"./parser":4,"./token":5}],4:[function(require,module,exports){
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
  return this.binary(this.comparison,
      [Token.equalEqual, Token.bangEqual]);
  /*
  var expr = this.comparison();

  while (this.match(Token.equalEqual) ||
         this.match(Token.bangEqual)) {
    var op = this.last.type;
    var right = this.comparison();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
  */
}

Parser.prototype.comparison = function() {
  return this.binary(this.term,
      [Token.less, Token.greater, Token.lessEqual, Token.greaterEqual]);
  /*
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
  */
}

Parser.prototype.term = function() {
  /*
  var expr = this.factor();

  while (this.match(Token.plus) ||
         this.match(Token.minus)) {
    var op = this.last.type;
    var right = this.factor();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
  */
  return this.binary(this.factor, [Token.plus, Token.minus]);
}

Parser.prototype.factor = function() {
  /*
  var expr = this.unary();

  while (this.match(Token.star) ||
         this.match(Token.slash) ||
         this.match(Token.percent)) {
    var op = this.last.type;
    var right = this.unary();
    expr = new BinaryExpr(expr, op, right);
  }

  return expr;
  */
  return this.binary(this.unary,
      [Token.star, Token.slash, Token.percent]);
}

Parser.prototype.binary = function(parseOperand, operators) {
  var expr = parseOperand.call(this);

  while (this.matchAny(operators)) {
    var op = this.last.type;
    var right = parseOperand.call(this);
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

Parser.prototype.matchAny = function(tokenTypes) {
  for (var i = 0; i < tokenTypes.length; i++) {
    if (this.match(tokenTypes[i])) return true;
  }

  return false;
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

},{"./ast":1,"./token":5}],5:[function(require,module,exports){
"use strict";

function Token(type, text, value) {
  this.type = type;
  this.text = text;
  this.value = value;
}

Token.prototype.toString = function() {
  var result = "Token." + this.type + " '" + this.text + "'";
  if (this.value != null) result += " " + this.value;
  return result;
}

// Token types.
Token.leftParen = "(";
Token.rightParen = ")";
Token.leftBracket = "[";
Token.rightBracket = "]";
Token.leftBrace = "{";
Token.rightBrace = "}";
Token.semicolon = ";";
Token.comma = ",";
Token.dot = ".";
Token.bang = "!";
Token.plus = "+";
Token.minus = "-";
Token.star = "*";
Token.slash = "/";
Token.percent = "%";
Token.equal = "=";
Token.equalEqual = "==";
Token.bangEqual = "!=";
Token.less = "<";
Token.greater = ">";
Token.lessEqual = "<=";
Token.greaterEqual = ">=";
Token.identifier = "identifier";
Token.string = "string";
Token.number = "number";
Token.end = "end";
Token.error = "error";

module.exports = Token;

},{}]},{},[3]);
