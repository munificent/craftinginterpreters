"use strict";

var Lexer = require("../lexer");
var Parser = require("../parser");
var prettyPrint = require("../pretty_print");

function test(source, parseFn, expectedAst) {
  var lexer = new Lexer(source);
  var parser = new Parser(lexer);

  var ast = parser[parseFn]();
  var actual = prettyPrint(ast);

  if (actual == expectedAst) {
    console.log("PASS: " + expectedAst);
  } else {
    console.log("FAIL: " + expectedAst);
    console.log(" got: " + actual);
  }
}

function testExpr(source, expectedAst) {
  test(source, "expression", expectedAst);
}

function testStmt(source, expectedAst) {
  test(source, "statement", expectedAst);
}

// Associativity.
testExpr("1 and 2 and 3", "((1 and 2) and 3)");
testExpr("1 or 2 or 3", "((1 or 2) or 3)");
testExpr("1 == 2 != 3 == 4 != 5", "((((1 == 2) != 3) == 4) != 5)");
testExpr("1 < 2 > 3 <= 4 >= 5", "((((1 < 2) > 3) <= 4) >= 5)");
testExpr("1 + 2 - 3 + 4 - 5", "((((1 + 2) - 3) + 4) - 5)");
testExpr("1 * 2 / 3 % 4 * 5 / 6 % 7", "((((((1 * 2) / 3) % 4) * 5) / 6) % 7)");

// Precedence.
testExpr("1 * 2 + 3 / 4 - 5", "(((1 * 2) + (3 / 4)) - 5)");
testExpr("1 + 2 < 3 - 4 > 5", "(((1 + 2) < (3 - 4)) > 5)");
testExpr("1 < 2 == 3 > 4 != 5", "(((1 < 2) == (3 > 4)) != 5)");
testExpr("1 and 2 == 3 and 4 != 5", "((1 and (2 == 3)) and (4 != 5))");
testExpr("1 or 2 and 3 or 4 and 5", "((1 or (2 and 3)) or (4 and 5))");

// Unary.
testExpr("+-!1 or -2 * +3", "((+ (- (! 1))) or ((- 2) * (+ 3)))");

// Expression statement.
testStmt("1 + 2;", "((1 + 2) ;)");

// Block statement.
testStmt("{}", "{ }");
testStmt("{ 1; }", "{ (1 ;) }");
testStmt("{ 1; 2; 3; }", "{ (1 ;) (2 ;) (3 ;) }");

// If statement.
testStmt("if (1 == 2) 3;", "(if (1 == 2) then (3 ;))");
testStmt("if (1 == 2) 3; else 4;", "(if (1 == 2) then (3 ;) else (4 ;))");

// Dangling else.
testStmt("if (1 == 2) if (3 == 4) 5; else 6;",
    "(if (1 == 2) then (if (3 == 4) then (5 ;) else (6 ;)))");

// While.
testStmt("while (1) 2;", "(while 1 (2 ;))");
testStmt("while (1) while (2) 3;", "(while 1 (while 2 (3 ;)))");

// Variable declaration.
testStmt("var name = 1 + 2;", "(var name = (1 + 2))");

// TODO: Test parse errors.
