var Lexer = require("./lexer.js");
var Token = require("./token.js");

function testLexer(source, expectedTokens) {
  var lexer = new Lexer(source);

  // The tokens should complete with an END.
  expectedTokens.push([Token.END, "", null]);

  var results = [];
  var failed = false;

  for (var i = 0; i < expectedTokens.length; i++) {
    var expected = new Token(
        expectedTokens[i][0],
        expectedTokens[i][1],
        expectedTokens[i][2]);
    var token = lexer.nextToken();

    if (token.type === expected.type &&
        token.text === expected.text &&
        token.value === token.value) {
      results.push("OK  " + token);
    } else {
      results.push("ERR " + token + " (expected " + expected + ")");
      failed = true;
    }
  }

  if (failed) {
    console.log("FAIL: " + source);
    for (var i = 0; i < results.length; i++) {
      console.log("      " + results[i]);
    }
  } else {
    console.log("PASS: " + source)
  }
}

testLexer("([{)]},;", [
  [Token.LEFT_PAREN,    "(", null],
  [Token.LEFT_BRACKET,  "[", null],
  [Token.LEFT_BRACE,    "{", null],
  [Token.RIGHT_PAREN,   ")", null],
  [Token.RIGHT_BRACKET, "]", null],
  [Token.RIGHT_BRACE,   "}", null],
  [Token.COMMA,         ",", null],
  [Token.SEMICOLON,     ";", null]
]);

testLexer("1234 0 000 -123", [
  [Token.NUMBER,        "1234", 1234],
  [Token.NUMBER,        "0",    0],
  [Token.NUMBER,        "000",  0],
  [Token.MINUS,         "-",    null],
  [Token.NUMBER,        "123",  123]
]);

testLexer("12.34 .45 56. 1.2.3 4..5", [
  [Token.NUMBER,        "12.34", 12.34],
  [Token.NUMBER,        ".45",   0.45],
  [Token.NUMBER,        "56.",   56.0],
  [Token.NUMBER,        "1.2",   1.2],
  [Token.NUMBER,        ".3",    0.3],
  [Token.NUMBER,        "4.",    4.0],
  [Token.NUMBER,        ".5",    0.5]
]);

testLexer("123.name 45..name", [
  [Token.NUMBER,        "123",   123],
  [Token.DOT,           ".",     null],
  [Token.IDENTIFIER,    "name",  null],
  [Token.NUMBER,        "45.",   45.0],
  [Token.DOT,           ".",     null],
  [Token.IDENTIFIER,    "name",  null]
]);

// TODO: Operators, maximal munch. Reserved words.
