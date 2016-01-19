"use strict";

// Converts the AST [node] to a string for debugging purposes.
function prettyPrint(node) {
  if (node === undefined) return "<error>";

  return node.accept({
    visitBlockStmt: function(node) {
      var result = "(block";
      for (var i = 0; i < node.statements.length; i++) {
        result += " " + prettyPrint(node.statements[i]);
      }

      return result + ")";
    },
    visitExpressionStmt: function(node) {
      return "(; " + prettyPrint(node.expression) + ")";
    },
    visitClassStmt: function(node) {
      var result = "(class " + node.name + " ";

      if (node.superclass != null) {
        result += prettyPrint(node.superclass) + " ";
      }

      result += "(";
      for (var i = 0; i < node.methods.length; i++) {
        if (i > 0) result += " ";
        result += prettyPrint(node.methods[i]);
      }

      return result + "))";
    },
    visitFunStmt: function(node) {
      var result = "(fun " + node.name + " (";

      for (var i = 0; i < node.parameters.length; i++) {
        if (i > 0) result += " ";
        result += node.parameters[i];
      }

      return result + ") " + prettyPrint(node.body) + ")";
    },
    visitIfStmt: function(node) {
      var result = "(if " + prettyPrint(node.condition) + " then ";
      result += prettyPrint(node.thenBranch);
      if (node.elseBranch !== null) {
        result += " else " + prettyPrint(node.elseBranch);
      }
      return result + ")";
    },
    visitReturnStmt: function(node) {
      if (node.value === null) return "(return)";
      return "(return " + prettyPrint(node.value) + ")";
    },
    visitVarStmt: function(node) {
      return "(var " + node.name + " = " + prettyPrint(node.initializer) + ")";
    },
    visitWhileStmt: function(node) {
      var result = "(while " + prettyPrint(node.condition) + " ";
      return result + prettyPrint(node.body) + ")";
    },

    visitAssignExpr: function(node) {
      var result = "(= " + prettyPrint(node.target) + " ";
      return result + prettyPrint(node.value) + ")";
    },
    visitBinaryExpr: function(node) {
      var result = "(" + node.op + " " + prettyPrint(node.left) + " ";
      return result + prettyPrint(node.right) + ")";
    },
    visitCallExpr: function(node) {
      var result = "(call " + prettyPrint(node.fn);
      for (var i = 0; i < node.args.length; i++) {
        result += " " + prettyPrint(node.args[i]);
      }

      return result + ")";
    },
    visitLogicalExpr: function(node) {
      var result = "(" + node.op + " " + prettyPrint(node.left) + " ";
      return result + prettyPrint(node.right) + ")";
    },
    visitNumberExpr: function(node) {
      return node.value.toString();
    },
    visitPropertyExpr: function(node) {
      return "(." + node.name + " " + prettyPrint(node.object) + ")";
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
