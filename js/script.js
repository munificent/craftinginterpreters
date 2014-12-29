
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
  var node = parser.parseProgram();

  displayAst(node);
  evaluateAst(node);
}

function displayTokens(source) {
  var lexer = new Lexer(input.value);
  var tokens = [];
  while (true) {
    var token = lexer.nextToken();
    tokens.push(token);
    if (token.type == Token.END) break;
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

  var html = "<ul><li>" + astToString(node) + "</li></ul>";
  document.querySelector("#ast").innerHTML = html;
}

function astToString(node) {
  return node.accept({
    visitBinary: function(node) {
      var html = "<span class='node'>" + node.op.toLowerCase() + "</span>";
      html += "<ul>";
      html += "<li>" + astToString(node.left) + "</li>";
      html += "<li>" + astToString(node.right) + "</li>";
      html += "</ul>";
      return html;
    },
    visitCall: function(node) {
      var html = "<span class='node'>call</span>";
      html += "<ul>";
      html += "<li>" + astToString(node.fn) + "</li>";

      for (var i = 0; i < node.args.length; i++) {
        html += "<li>" + astToString(node.args[i]) + "</li>";
      }

      html += "</ul>";
      return html;
    },
    visitNumber: function(node) {
      return "<span class='node number'>" + node.value + "</span>";
    },
    visitString: function(node) {
      return "<span class='node string'>" + node.value + "</span>";
    },
    visitVariable: function(node) {
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
    visitBinary: function(node) {
      var left = evaluate(node.left);
      var right = evaluate(node.right);

      // TODO: Don't always use JS semantics.
      switch (node.op) {
        case Token.PLUS: return left + right;
        case Token.MINUS: return left - right;
        case Token.STAR: return left * right;
        case Token.SLASH: return left / right;
        case Token.PERCENT: return left % right;
        case Token.EQUALS_EQUALS: return left === right;
        case Token.BANG_EQUALS: return left !== right;
        case Token.LESS: return left < right;
        case Token.GREATER: return left > right;
        case Token.LESS_EQUALS: return left <= right;
        case Token.GREATER_EQUALS: return left >= right;
      }

      throw "unknown operator " + node.op;
    },
    visitCall: function(node) {
      throw "call not implemented";
    },
    visitNumber: function(node) {
      return node.value;
    },
    visitString: function(node) {
      return node.value;
    },
    visitVariable: function(node) {
      throw "variable not implemented";
    }
  });
}
