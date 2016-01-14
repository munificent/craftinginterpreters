"use strict";

var Token = require("./token");
var ast = require("./ast");
var VariableExpr = ast.VariableExpr;

function Interpreter() {
  this.globals = {
    "print": function(text) {
      console.log(text);
      return text;
    }
  };
}

function RuntimeError(message) {
  // TODO: Capture source location to show callstack.
  this.message = message;
}

Interpreter.prototype.interpret = function(statement) {
  statement.accept(this);
}

Interpreter.prototype.evaluate = function(expression) {
  return expression.accept(this);
}

Interpreter.prototype.visitBlockStmt = function(node) {
  for (var i = 0; i < node.statements.length; i++) {
    this.evaluate(node.statements[i]);
  }
}

Interpreter.prototype.visitExpressionStmt = function(node) {
  this.evaluate(node.expression);
}
  //   visitClassStmt: function(node) {
  //     var result = "(class " + node.name + " ";

  //     if (node.superclass != null) {
  //       result += prettyPrint(node.superclass) + " ";
  //     }

  //     result += "(";
  //     for (var i = 0; i < node.methods.length; i++) {
  //       if (i > 0) result += " ";
  //       result += prettyPrint(node.methods[i]);
  //     }

  //     return result + "))";
  //   },
  //   visitFunStmt: function(node) {
  //     var result = "(fun " + node.name + " (";

  //     for (var i = 0; i < node.parameters.length; i++) {
  //       if (i > 0) result += " ";
  //       result += node.parameters[i];
  //     }

  //     return result + ") " + prettyPrint(node.body) + ")";
  //   },

Interpreter.prototype.visitIfStmt = function(node) {
  var condition = this.evaluate(node.condition);

  // TODO: Don't use JS truthiness.
  if (condition) {
    this.evaluate(node.thenBranch);
  } else {
    this.evaluate(node.elseBranch);
  }
}

Interpreter.prototype.visitVarStmt = function(node) {
  // TODO: Check for name collision.
  var value = this.evaluate(node.initializer);
  this.globals[node.name] = value;
}

Interpreter.prototype.visitWhileStmt = function(node) {
  // TODO: Don't use JS truthiness.
  while (this.evaluate(node.condition)) {
    this.evaluate(node.body);
  }
}

Interpreter.prototype.visitAssignExpr = function(node) {
  var value = this.evaluate(node.value);

  if (node.target instanceof VariableExpr) {
    var name = node.target.name;
    if (!this.globals.hasOwnProperty(name)) {
      throw new RuntimeError("Variable '" + name + "' is not defined.");
    }

    this.globals[name] = value;
  } else {
    // TODO: Implement assigning properties.
    throw "not impl";
  }

  return value;
}

Interpreter.prototype.visitBinaryExpr = function(node) {
  var left = this.evaluate(node.left);
  var right = this.evaluate(node.right);

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
}

Interpreter.prototype.visitCallExpr = function(node) {
  var fn = this.evaluate(node.fn);

  var args = [];
  for (var i = 0; i < node.args.length; i++) {
    args.push(this.evaluate(node.args[i]));
  }

  if (fn instanceof Function) {
    return fn.apply(this, args);
  } else {
    throw "user-defined fns not implemented yet.";
  }
}

Interpreter.prototype.visitLogicalExpr = function(node) {
  var left = this.evaluate(node.left);

  // TODO: Don't use JS truthiness.
  if (node.op == Token.and) {
    if (!left) return left;
  } else {
    if (left) return left;
  }

  return this.evaluate(node.right);
}

Interpreter.prototype.visitNumberExpr = function(node) {
  return node.value;
}

//   visitPropertyExpr: function(node) {
//     return "(." + node.name + " " + prettyPrint(node.object) + ")";
//   },

Interpreter.prototype.visitStringExpr = function(node) {
  return node.value;
}

Interpreter.prototype.visitUnaryExpr = function(node) {
  var right = this.evaluate(node.right);

  // TODO: Don't always use JS semantics.
  switch (node.op) {
    case Token.plus: return +right;
    case Token.minus: return -right;
    case Token.bang: return !right;
  }
}

Interpreter.prototype.visitVariableExpr = function(node) {
  var name = node.name;
  if (!this.globals.hasOwnProperty(name)) {
    throw new RuntimeError("Variable '" + name + "' is not defined.");
  }

  return this.globals[name];
}

exports.Interpreter = Interpreter;
exports.RuntimeError = RuntimeError;

