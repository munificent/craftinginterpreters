"use strict";

var Token = require("./token");
var ast = require("./ast");
var VariableExpr = ast.VariableExpr;

function Interpreter() {
  this.globals = new Context(null);

  this.globals.define("print", function(text) {
    console.log(text);
    return text;
  });
}

Interpreter.prototype.interpret = function(statement) {
  statement.accept(this, this.globals);
}

Interpreter.prototype.evaluate = function(expression, context) {
  return expression.accept(this, context);
}

Interpreter.prototype.visitBlockStmt = function(node, context) {
  context = new Context(context);

  for (var i = 0; i < node.statements.length; i++) {
    this.evaluate(node.statements[i], context);
  }
}

Interpreter.prototype.visitExpressionStmt = function(node, context) {
  this.evaluate(node.expression, context);
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

Interpreter.prototype.visitFunStmt = function(node, context) {
  context.define(node.name, new Fun(node.parameters, node.body, context));
}

Interpreter.prototype.visitIfStmt = function(node, context) {
  var condition = this.evaluate(node.condition, context);

  context = new Context(context);
  // TODO: Don't use JS truthiness.
  if (condition) {
    this.evaluate(node.thenBranch, context);
  } else {
    this.evaluate(node.elseBranch, context);
  }
}

Interpreter.prototype.visitVarStmt = function(node, context) {
  var value = this.evaluate(node.initializer, context);
  context.define(node.name, value);
}

Interpreter.prototype.visitWhileStmt = function(node, context) {
  // TODO: Don't use JS truthiness.
  while (this.evaluate(node.condition, context)) {
    this.evaluate(node.body, context);
  }
}

Interpreter.prototype.visitAssignExpr = function(node, context) {
  var value = this.evaluate(node.value, context);

  if (node.target instanceof VariableExpr) {
    context.assign(node.target.name, value);
  } else {
    // TODO: Implement assigning properties.
    throw "not impl";
  }

  return value;
}

Interpreter.prototype.visitBinaryExpr = function(node, context) {
  var left = this.evaluate(node.left, context);
  var right = this.evaluate(node.right, context);

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

Interpreter.prototype.visitCallExpr = function(node, context) {
  var fn = this.evaluate(node.fn, context);

  var args = [];
  for (var i = 0; i < node.args.length; i++) {
    args.push(this.evaluate(node.args[i], context));
  }

  // Primitive functions.
  if (fn instanceof Function) return fn.apply(this, args);

  if (fn instanceof Fun) {
    if (args.length != fn.parameters.length) {
      // TODO: Better message!
      throw new RuntimeError("Arity mismatch.");
    }

    context = new Context(fn.context);

    for (var i = 0; i < args.length; i++) {
      context.define(fn.parameters[i], args[i]);
    }

    return this.evaluate(fn.body, context);
  }

  throw new RuntimeError(fn.toString() + " cannot be called.");
}

Interpreter.prototype.visitLogicalExpr = function(node, context) {
  var left = this.evaluate(node.left, context);

  // TODO: Don't use JS truthiness.
  if (node.op == Token.and) {
    if (!left) return left;
  } else {
    if (left) return left;
  }

  return this.evaluate(node.right, context);
}

Interpreter.prototype.visitNumberExpr = function(node, context) {
  return node.value;
}

//   visitPropertyExpr: function(node) {
//     return "(." + node.name + " " + prettyPrint(node.object) + ")";
//   },

Interpreter.prototype.visitStringExpr = function(node, context) {
  return node.value;
}

Interpreter.prototype.visitUnaryExpr = function(node, context) {
  var right = this.evaluate(node.right, context);

  // TODO: Don't always use JS semantics.
  switch (node.op) {
    case Token.plus: return +right;
    case Token.minus: return -right;
    case Token.bang: return !right;
  }
}

Interpreter.prototype.visitVariableExpr = function(node, context) {
  return context.lookUp(node.name);
}

function RuntimeError(message) {
  // TODO: Capture source location to show callstack.
  this.message = message;
}

// TODO: Better name.
function Fun(parameters, body, closure) {
  this.parameters = parameters;
  this.body = body;
  this.closure;
}

function Context(outer) {
  this.outer = outer;
  this.variables = {};
}

Context.prototype.findDefinition = function(name) {
  var context = this;
  while (context != null) {
    if (context.variables.hasOwnProperty(name)) {
      return context.variables;
    }

    context = context.outer;
  }

  throw new RuntimeError("Variable '" + name + "' is not defined.");
}

Context.prototype.define = function(name, value) {
  // TODO: This does the wrong thing with closures and shadowing:
  //     var a = 1;
  //     {
  //       fun foo() { print(a); }
  //       foo(); // 1.
  //       var a = 2;
  //       foo(); // 2.
  //     }
  if (this.variables.hasOwnProperty(name)) {
    throw new RuntimeError("Variable '" + name + "' is already defined.");
  }

  this.variables[name] = value;
}

Context.prototype.lookUp = function(name) {
  return this.findDefinition(name)[name];
}

Context.prototype.assign = function(name, value) {
  this.findDefinition(name)[name] = value;
}

exports.Interpreter = Interpreter;
exports.RuntimeError = RuntimeError;

