class Node:

class BinaryNode(Node):
  __init__(self, left, op, right):
    self.left = left
    self.op = op
    self.right = right

  def accept(self, visitor):
    return visitor.visitBinary(this)


class CallNode(Node):
  __init__(self, fn, args):
    self.fn = fn
    self.args = args

  def accept(visitor):
    return visitor.visitCall(self)


class NumberNode(Node):
  __init__(value):
    self.value = value

  def accept(self, visitor):
    return visitor.visitNumber(self)


class StringNode(Node):
  __init__(value):
    self.value = value

  def accept(self, visitor):
    return visitor.visitString(self)


class StringNode(Node):
  __init__(name):
    self.name = name

  def accept(self, visitor):
    return visitor.visitVariable(self)
