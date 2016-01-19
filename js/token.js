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

// Using keywords for "and" and "or" instead of "||" and "&&" since we don't
// define the bitwise forms and it's weird to lex "||" without "|".
Token.and = "and";
Token.class_ = "class";
Token.else_ = "else";
Token.fun = "fun";
Token.for_ = "for";
Token.if_ = "if";
Token.or = "or";
Token.return_ = "return";
Token.var_ = "var";
Token.while_ = "while";

Token.end = "end";
Token.error = "error";

module.exports = Token;
