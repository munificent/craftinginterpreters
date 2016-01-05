"use strict";

var Lexer = require("./lexer");
var Parser = require("./parser");
var Token = require("./token");

window.onload = function() {
  var input = document.querySelector("textarea#input");
  if (input.addEventListener) {
    input.addEventListener("input", refresh, false);
  } else if (input.attachEvent) {
    input.attachEvent("onpropertychange", refresh);
  }

  refresh();
};

function refresh() {
  var input = document.querySelector("textarea#input");

  displayTokens(input.value);

  var lexer = new Lexer(input.value);
  var parser = new Parser(lexer);
  var node = parser.parse();

  displayAst(node);
  evaluateAst(node);
}

function displayTokens(source) {
  var lexer = new Lexer(input.value);
  var tokens = [];
  while (true) {
    var token = lexer.nextToken();
    tokens.push(token);
    if (token.type == Token.end) break;
  }

  var html = "";
  for (var i = 0; i < tokens.length; i++) {
    var token = tokens[i];
    html += "<span class='token " + token.type.toLowerCase() + "'>";
    if (token.value != null) {
      html += "<span class='value'>" + token.value + "</span>";
    } else {
      html += token.text;
    }
    html += "</span>";
  }

  document.querySelector("#tokens").innerHTML = html;
}

function displayAst(node) {

  var html = "<ul><li>" + astToHtml(node) + "</li></ul>";
  document.querySelector("#ast").innerHTML = html;
}

function astToHtml(node) {
  if (node === undefined) return "ERROR";

  return node.accept({
    visitBinaryExpr: function(node) {
      var html = "<span class='node'>" + node.op + "</span>";
      html += "<ul>";
      html += "<li>" + astToHtml(node.left) + "</li>";
      html += "<li>" + astToHtml(node.right) + "</li>";
      html += "</ul>";
      return html;
    },
    visitCallExpr: function(node) {
      var html = "<span class='node'>call</span>";
      html += "<ul>";
      html += "<li>" + astToHtml(node.fn) + "</li>";

      for (var i = 0; i < node.args.length; i++) {
        html += "<li>" + astToHtml(node.args[i]) + "</li>";
      }

      html += "</ul>";
      return html;
    },
    visitNumberExpr: function(node) {
      return "<span class='node number'>" + node.value + "</span>";
    },
    visitStringExpr: function(node) {
      return "<span class='node string'>" + node.value + "</span>";
    },
    visitUnaryExpr: function(node) {
      var html = "<span class='node'>" + node.op + "</span>";
      html += "<ul>";
      html += "<li>" + astToHtml(node.right) + "</li>";
      html += "</ul>";
      return html;
    },
    visitVariableExpr: function(node) {
      return "<span class='node var'>" + node.name + "</span>";
    }
  });
}

function evaluateAst(node) {
  var result = evaluate(node);
  document.querySelector("#evaluate").innerHTML = result.toString();
}

function evaluate(node) {
  return node.accept({
    visitBinaryExpr: function(node) {
      var left = evaluate(node.left);
      var right = evaluate(node.right);

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

      throw "unknown operator " + node.op;
    },
    visitCallExpr: function(node) {
      throw "call not implemented";
    },
    visitNumberExpr: function(node) {
      return node.value;
    },
    visitStringExpr: function(node) {
      return node.value;
    },
    visitUnaryExpr: function(node) {
      var right = evaluate(node.right);

      // TODO: Don't always use JS semantics.
      switch (node.op) {
        case Token.plus: return +right;
        case Token.minus: return -right;
        case Token.bang: return !right;
      }

      throw "unknown operator " + node.op;
    },
    visitVariableExpr: function(node) {
      throw "variable not implemented";
    }
  });
}
