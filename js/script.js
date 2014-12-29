
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
  displayAst(input.value);
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

function displayAst(source) {
  var lexer = new Lexer(input.value);
  var parser = new Parser(lexer);
  var node = parser.parseProgram();

  var html = "<ul><li>" + astToString(node) + "</li></ul>";
  document.querySelector("#ast").innerHTML = html;
}

function astToString(node) {
  // TODO: Visitor pattern.
  if (node instanceof BinaryNode) {
    var html = "<span class='node'>" + node.op.toLowerCase() + "</span>";
    html += "<ul>";
    html += "<li>" + astToString(node.left) + "</li>";
    html += "<li>" + astToString(node.right) + "</li>";
    html += "</ul>";
    return html;
  } else if (node instanceof CallNode) {
    var html = "<span class='node'>call</span>";
    html += "<ul>";
    html += "<li>" + astToString(node.fn) + "</li>";

    for (var i = 0; i < node.args.length; i++) {
      html += "<li>" + astToString(node.args[i]) + "</li>";
    }

    html += "</ul>";
    return html;
  } else if (node instanceof NumberNode) {
    return "<span class='node number'>" + node.value + "</span>";
  } else if (node instanceof StringNode) {
    return "<span class='node string'>" + node.value + "</span>";
  } else if (node instanceof VariableNode) {
    return "<span class='node var'>" + node.name + "</span>";
  }
}
