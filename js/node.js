function Node() {
}

function BinaryNode(left, op, right) {
  Node.call(this);
  this.left = left;
  this.op = op;
  this.right = right;
}

BinaryNode.prototype = Object.create(Node.prototype);

BinaryNode.prototype.accept = function(visitor) {
  return visitor.visitBinary(this);
}

function CallNode(fn, args) {
  Node.call(this);
  this.fn = fn;
  this.args = args;
}

CallNode.prototype = Object.create(Node.prototype);

CallNode.prototype.accept = function(visitor) {
  return visitor.visitCall(this);
}

function NumberNode(value) {
  Node.call(this);
  this.value = value;
}

NumberNode.prototype = Object.create(Node.prototype);

NumberNode.prototype.accept = function(visitor) {
  return visitor.visitNumber(this);
}

function StringNode(value) {
  Node.call(this);
  this.value = value;
}

StringNode.prototype = Object.create(Node.prototype);

StringNode.prototype.accept = function(visitor) {
  return visitor.visitString(this);
}

function VariableNode(name) {
  Node.call(this);
  this.name = name;
}

VariableNode.prototype = Object.create(Node.prototype);

VariableNode.prototype.accept = function(visitor) {
  return visitor.visitVariable(this);
}
