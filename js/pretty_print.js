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
      return prettyPrint(node.expression) + ";";
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
