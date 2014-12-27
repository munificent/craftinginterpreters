
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

  var lexer = new Lexer(input.value);
  var tokens = [];
  while (true) {
    var token = lexer.nextToken();
    tokens.push(token);
    if (token.type == Token.END) break;
  }

  var output = document.querySelector("#output");

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

  output.innerHTML = html;
}

// Returns true if `c` is an English letter or underscore.
function isAlpha(c) {
  return (c >= "a" && c <= "z") ||
         (c >= "A" && c <= "Z") ||
         c == "_";
}

// Returns true if `c` is an English letter, underscore, or digit.
function isAlphaNumeric(c) {
  return isAlpha(c) || isDigit(c);
}

// Returns true if `c` is a space, newline, or tab.
function isWhitespace(c) {
  return c == " " || c == "\n" || c == "\t";
}

// Returns true if `c` is a digit.
function isDigit(c) {
  return c >= "0" && c <= "9";
}