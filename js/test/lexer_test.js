"use strict";

var Lexer = require("../lexer");
var Token = require("../token");

function test(source, expectedTokens) {
  var lexer = new Lexer(source);

  // The tokens should complete with an END.
  expectedTokens.push([Token.end, "", null]);

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

test("([{)]},;.!", [
  [Token.leftParen,    "(", null],
  [Token.leftBracket,  "[", null],
  [Token.leftBrace,    "{", null],
  [Token.rightParen,   ")", null],
  [Token.rightBracket, "]", null],
  [Token.rightBrace,   "}", null],
  [Token.comma,        ",", null],
  [Token.semicolon,    ";", null],
  [Token.dot,          ".", null],
  [Token.bang,         "!", null]
]);

test("+-*/% = == != < > <= >=", [
  [Token.plus,         "+", null],
  [Token.minus,        "-", null],
  [Token.star,         "*", null],
  [Token.slash,        "/", null],
  [Token.percent,      "%", null],
  [Token.equal,        "=", null],
  [Token.equalEqual,   "==", null],
  [Token.bangEqual,    "!=", null],
  [Token.less,         "<", null],
  [Token.greater,      ">", null],
  [Token.lessEqual,    "<=", null],
  [Token.greaterEqual, ">=", null]
]);

// Maximal munch.
test("===>==<==!==", [
  [Token.equalEqual,   "==", null],
  [Token.equal,        "=", null],
  [Token.greaterEqual, ">=", null],
  [Token.equal,        "=", null],
  [Token.lessEqual,    "<=", null],
  [Token.equal,        "=", null],
  [Token.bangEqual,    "!=", null],
  [Token.equal,        "=", null]
]);

test("1234 0 000 -123", [
  [Token.number,        "1234", 1234],
  [Token.number,        "0",    0],
  [Token.number,        "000",  0],
  [Token.minus,         "-",    null],
  [Token.number,        "123",  123]
]);

test("12.34 .45 56. 1.2.3 4..5", [
  [Token.number,        "12.34", 12.34],
  [Token.number,        ".45",   0.45],
  [Token.number,        "56.",   56.0],
  [Token.number,        "1.2",   1.2],
  [Token.number,        ".3",    0.3],
  [Token.number,        "4.",    4.0],
  [Token.number,        ".5",    0.5]
]);

test("123.name 45..name", [
  [Token.number,        "123",   123],
  [Token.dot,           ".",     null],
  [Token.identifier,    "name",  null],
  [Token.number,        "45.",   45.0],
  [Token.dot,           ".",     null],
  [Token.identifier,    "name",  null]
]);

// TODO: Reserved words.
