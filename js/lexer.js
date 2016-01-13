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

Lexer.keywords = {
  "and": Token.and,
  "class": Token.class_,
  "else": Token.else_,
  "fun": Token.fun,
  "for": Token.for_,
  "if": Token.if_,
  "or": Token.or,
  "var": Token.var_,
  "while": Token.while_
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

  // See if the identifier is a reserved word.
  var text = this.source.substring(this.start, this.current);
  var type = Lexer.keywords[text];
  if (type === undefined) type = Token.identifier;

  return this.makeToken(type);
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
