"use strict";

// Converts the AST [node] to a string for debugging purposes.
function prettyPrint(node) {
  if (node === undefined) return "<error>";

  return node.accept({
    visitBlockStmt: function(node) {
      var result = "{";
      for (var i = 0; i < node.statements.length; i++) {
        result += " " + prettyPrint(node.statements[i]);
      }

      return result + " }";
    },
    visitExpressionStmt: function(node) {
      return "(" + prettyPrint(node.expression) + " ;)";
    },
    visitIfStmt: function(node) {
      var result = "(if " + prettyPrint(node.condition) + " then ";
      result += prettyPrint(node.thenBranch);
      if (node.elseBranch !== null) {
        result += " else " + prettyPrint(node.elseBranch);
      }
      return result + ")";
    },
    visitVarStmt: function(node) {
      return "(var " + node.name + " = " + prettyPrint(node.initializer) + ")";
    },
    visitWhileStmt: function(node) {
      var result = "(while " + prettyPrint(node.condition) + " ";
      return result + prettyPrint(node.body) + ")";
    },

    visitBinaryExpr: function(node) {
      var result = "(" + prettyPrint(node.left);
      result += " " + node.op + " ";
      result += prettyPrint(node.right) + ")";
      return result;
    },
    visitCallExpr: function(node) {
      var result = "(" + prettyPrint(node.fn) + ")(";
      for (var i = 0; i < node.args.length; i++) {
        if (i > 0) result += ", ";
        result += prettyPrint(node.args[i]);
      }

      result += ")";
      return result;
    },
    visitLogicalExpr: function(node) {
      var result = "(" + prettyPrint(node.left);
      result += " " + node.op + " ";
      result += prettyPrint(node.right) + ")";
      return result;
    },
    visitNumberExpr: function(node) {
      return node.value.toString();
    },
    visitStringExpr: function(node) {
      // TODO: Escape special characters.
      return node.value.toString();
    },
    visitUnaryExpr: function(node) {
      var result = "(" + node.op + " ";
      result += prettyPrint(node.right) + ")";
      return result;
    },
    visitVariableExpr: function(node) {
      return node.name;
    }
  });
}

module.exports = prettyPrint;
