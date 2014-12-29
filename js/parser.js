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
  return this.term();
}

Parser.prototype.term = function() {
  var expr = this.call();

  // TODO: Minus.
  while (this.match(Token.PLUS)) {
    var op = this.last.type;
    var right = this.call();
    expr = new BinaryNode(expr, op, right);
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

    expr = new CallNode(expr, args);
  }

  return expr;
}

Parser.prototype.primary = function() {
  // TODO: Switch on type?

  if (this.match(Token.NUMBER)) {
    return new NumberNode(this.last.value);
  }

  if (this.match(Token.STRING)) {
    return new StringNode(this.last.value);
  }

  if (this.match(Token.IDENTIFIER)) {
    return new VariableNode(this.last.text);
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

function Node() {
}

function BinaryNode(left, op, right) {
  Node.call(this);
  this.left = left;
  this.op = op;
  this.right = right;
}

BinaryNode.prototype = Object.create(Node.prototype);

function CallNode(fn, args) {
  Node.call(this);
  this.fn = fn;
  this.args = args;
}

CallNode.prototype = Object.create(Node.prototype);

function NumberNode(value) {
  Node.call(this);
  this.value = value;
}

NumberNode.prototype = Object.create(Node.prototype);

function StringNode(value) {
  Node.call(this);
  this.value = value;
}

StringNode.prototype = Object.create(Node.prototype);

function VariableNode(name) {
  Node.call(this);
  this.name = name;
}

VariableNode.prototype = Object.create(Node.prototype);
