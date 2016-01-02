(function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);var f=new Error("Cannot find module '"+o+"'");throw f.code="MODULE_NOT_FOUND",f}var l=n[o]={exports:{}};t[o][0].call(l.exports,function(e){var n=t[o][1][e];return s(n?n:e)},l,l.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({1:[function(require,module,exports){
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
exports.VariableExpr = VariableExpr;
exports.Stmt = Stmt;
exports.BlockStmt = BlockStmt;
exports.ExpressionStmt = ExpressionStmt;

},{}],2:[function(require,module,exports){
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
  "(": Token.LEFT_PAREN,
  ")": Token.RIGHT_PAREN,
  "[": Token.LEFT_BRACKET,
  "]": Token.RIGHT_BRACKET,
  "{": Token.LEFT_BRACE,
  "}": Token.RIGHT_BRACE,
  ";": Token.SEMICOLON,
  ",": Token.COMMA,
};

Lexer.prototype.nextToken = function() {
  this.skipWhitespace();

  if (this.current >= this.source.length) return new Token(Token.END, "");

  var c = this.advance();

  // See if it's a punctuator.
  var type = Lexer.punctuators[c];
  if (type != undefined) return this.makeToken(type);

  if (isAlpha(c)) return this.identifier();
  if (isDigit(c)) return this.number();

  switch (c) {
    case "\"": return this.string();
    case "+": return this.makeToken(Token.PLUS);
    case "-": return this.makeToken(Token.MINUS);
    case "*": return this.makeToken(Token.STAR);
    case "/": return this.makeToken(Token.SLASH);
    case "%": return this.makeToken(Token.PERCENT);
    case "!":
      if (this.match("=")) return this.makeToken(Token.BANG_EQUALS);
      return this.makeToken(Token.ERROR);

    case ".":
      if (isDigit(this.peek())) return this.number();

      return this.makeToken(Token.DOT);

    case "=":
      if (this.match("=")) return this.makeToken(Token.EQUALS_EQUALS);
      return this.makeToken(Token.EQUALS);

    case "<":
      if (this.match("=")) return this.makeToken(Token.LESS_EQUALS);
      return this.makeToken(Token.LESS);

    case ">":
      if (this.match("=")) return this.makeToken(Token.GREATER_EQUALS);
      return this.makeToken(Token.GREATER);
  }

  return this.makeToken(Token.ERROR);
}

Lexer.prototype.skipWhitespace = function() {
  this.consumeWhile(isWhitespace);

  // Don't include these characters in the next token.
  this.start = this.current;
}

Lexer.prototype.identifier = function() {
  this.consumeWhile(isAlphaNumeric);
  return this.makeToken(Token.IDENTIFIER);
}

Lexer.prototype.number = function() {
  this.consumeWhile(isDigit);

  // Look for a fractional part.
  if (this.peek() == "." && !isAlpha(this.peek(1))) {
    // Consume the "."
    this.advance();

    this.consumeWhile(isDigit);
  }

  return this.makeToken(Token.NUMBER, parseFloat);
}

Lexer.prototype.string = function() {
  // TODO: Escapes.
  // TODO: What about newlines?
  this.consumeWhile(function(c) { return c != "\""; });

  // Unterminated string.
  if (this.isAtEnd()) return this.makeToken(Token.ERROR);

  // The closing ".
  this.advance();

  return this.makeToken(Token.STRING, function(text) {
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
    if (token.type == Token.END) break;
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

  var html = "<ul><li>" + astToString(node) + "</li></ul>";
  document.querySelector("#ast").innerHTML = html;
}

function astToString(node) {
  if (node === undefined) return "ERROR";

  return node.accept({
    visitBinaryExpr: function(node) {
      var html = "<span class='node'>" + node.op.toLowerCase() + "</span>";
      html += "<ul>";
      html += "<li>" + astToString(node.left) + "</li>";
      html += "<li>" + astToString(node.right) + "</li>";
      html += "</ul>";
      return html;
    },
    visitCallExpr: function(node) {
      var html = "<span class='node'>call</span>";
      html += "<ul>";
      html += "<li>" + astToString(node.fn) + "</li>";

      for (var i = 0; i < node.args.length; i++) {
        html += "<li>" + astToString(node.args[i]) + "</li>";
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
        case Token.PLUS: return left + right;
        case Token.MINUS: return left - right;
        case Token.STAR: return left * right;
        case Token.SLASH: return left / right;
        case Token.PERCENT: return left % right;
        case Token.EQUALS_EQUALS: return left === right;
        case Token.BANG_EQUALS: return left !== right;
        case Token.LESS: return left < right;
        case Token.GREATER: return left > right;
        case Token.LESS_EQUALS: return left <= right;
        case Token.GREATER_EQUALS: return left >= right;
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
    visitVariableExpr: function(node) {
      throw "variable not implemented";
    }
  });
}

},{"./lexer":2,"./parser":4,"./token":5}],4:[function(require,module,exports){
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

},{"./ast":1,"./token":5}],5:[function(require,module,exports){

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
Token.LEFT_PAREN = "LEFT_PAREN";
Token.RIGHT_PAREN = "RIGHT_PAREN";
Token.LEFT_BRACKET = "LEFT_BRACKET";
Token.RIGHT_BRACKET = "RIGHT_BRACKET";
Token.LEFT_BRACE = "LEFT_BRACE";
Token.RIGHT_BRACE = "RIGHT_BRACE";
Token.SEMICOLON = "SEMICOLON";
Token.COMMA = "COMMA";
Token.DOT = "DOT";
Token.PLUS = "PLUS";
Token.MINUS = "MINUS";
Token.STAR = "STAR";
Token.SLASH = "SLASH";
Token.PERCENT = "PERCENT";
Token.EQUALS = "EQUALS";
Token.EQUALS_EQUALS = "EQUALS_EQUALS";
Token.BANG_EQUALS = "BANG_EQUALS";
Token.LESS = "LESS";
Token.GREATER = "GREATER";
Token.LESS_EQUALS = "LESS_EQUALS";
Token.GREATER_EQUALS = "GREATER_EQUALS";
Token.IDENTIFIER = "IDENTIFIER";
Token.STRING = "STRING";
Token.NUMBER = "NUMBER";
Token.END = "END";
Token.ERROR = "ERROR";

module.exports = Token;

},{}]},{},[3]);
