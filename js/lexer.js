
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

  if (this.current >= this.source.length) return new Token(Token.END, "end");

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
      // TODO: Allow numbers starting with "."?
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
  if (this.peek() == "." && isDigit(this.peek(1))) {
    // Consume the "."
    this.advance();

    this.consumeWhile(isDigit);
  }

  return this.makeToken(Token.NUMBER, parseFloat);
}

Lexer.prototype.string = function() {
  // TODO: Escapes.
  this.consumeWhile(function(c) { return c != "\""; });

  // Unterminated string.
  if (this.isAtEnd()) return this.makeToken(Token.ERROR);

  // The closing ".
  this.advance();

  return this.makeToken(Token.STRING, function(text) {
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
