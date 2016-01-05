"use strict";

var Lexer = require("../lexer");
var Parser = require("../parser");
var prettyPrint = require("../pretty_print");
//var Token = require("../token");

function test(source, expectedAst) {
  var lexer = new Lexer(source);
  var parser = new Parser(lexer);

  var ast = parser.parse();
  var actual = prettyPrint(ast);

  if (actual == expectedAst) {
    console.log("PASS: " + expectedAst);
  } else {
    console.log("FAIL: " + expectedAst);
    console.log(" got: " + actual);
  }
}


// Associativity.
test("1 == 2 != 3 == 4 != 5", "((((1 == 2) != 3) == 4) != 5)");
test("1 < 2 > 3 <= 4 >= 5", "((((1 < 2) > 3) <= 4) >= 5)");
test("1 + 2 - 3 + 4 - 5", "((((1 + 2) - 3) + 4) - 5)");
test("1 * 2 / 3 % 4 * 5 / 6 % 7", "((((((1 * 2) / 3) % 4) * 5) / 6) % 7)");

// TODO: Precedence. Unary. Etc.
