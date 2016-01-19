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

// Call.
testExpr("a()", "(call a)");
testExpr("a(1)", "(call a 1)");
testExpr("a(1, 2, 3, 4)", "(call a 1 2 3 4)");
testExpr("a(1)(2)", "(call (call a 1) 2)");

// Property.
testExpr("a.b", "(.b a)");
testExpr("a.b.c.d", "(.d (.c (.b a)))");
testExpr("1.a", "(.a 1)");
testExpr("a.b().c()", "(call (.c (call (.b a))))");
// TODO: Invalid tokens after ".".

// Unary.
testExpr("+-!1 or -2 * +3", "(or (+ (- (! 1))) (* (- 2) (+ 3)))");

// Binary associativity.
testExpr("1 * 2 / 3 % 4 * 5 / 6 % 7", "(% (/ (* (% (/ (* 1 2) 3) 4) 5) 6) 7)");
testExpr("1 + 2 - 3 + 4 - 5", "(- (+ (- (+ 1 2) 3) 4) 5)");
testExpr("1 < 2 > 3 <= 4 >= 5", "(>= (<= (> (< 1 2) 3) 4) 5)");
testExpr("1 == 2 != 3 == 4 != 5", "(!= (== (!= (== 1 2) 3) 4) 5)");
testExpr("1 or 2 or 3", "(or (or 1 2) 3)");
testExpr("1 and 2 and 3", "(and (and 1 2) 3)");
testExpr("a = b = c = d", "(= a (= b (= c d)))");

// Binary precedence.
testExpr("1 * 2 + 3 / 4 - 5", "(- (+ (* 1 2) (/ 3 4)) 5)");
testExpr("1 + 2 < 3 - 4 > 5", "(> (< (+ 1 2) (- 3 4)) 5)");
testExpr("1 < 2 == 3 > 4 != 5", "(!= (== (< 1 2) (> 3 4)) 5)");
testExpr("1 and 2 == 3 and 4 != 5", "(and (and 1 (== 2 3)) (!= 4 5))");
testExpr("1 or 2 and 3 or 4 and 5", "(or (or 1 (and 2 3)) (and 4 5))");
testExpr("a = b or c = d or e", "(= a (= (or b c) (or d e)))");

// TODO: Valid and invalid assignment targets.
// a + b = c
// (a) = b

// Expression statement.
testStmt("1 + 2;", "(; (+ 1 2))");

// Block statement.
testStmt("{}", "(block)");
testStmt("{ 1; }", "(block (; 1))");
testStmt("{ 1; 2; 3; }", "(block (; 1) (; 2) (; 3))");

// Class declaration.
testStmt("class F {}", "(class F ())");
testStmt("class F < B {}", "(class F B ())");
testStmt("class F < B { a() {} b() {} }",
    "(class F B ((fun a () (block)) (fun b () (block))))");
testStmt("class F < (B + C) {}", "(class F (+ B C) ())");

// Function declaration.
testStmt("fun f() {}", "(fun f () (block))");
testStmt("fun f(a, b, c) { d; e; }", "(fun f (a b c) (block (; d) (; e)))");
// TODO: Error if body is not block.

// If statement.
testStmt("if (1 == 2) 3;", "(if (== 1 2) then (; 3))");
testStmt("if (1 == 2) 3; else 4;", "(if (== 1 2) then (; 3) else (; 4))");
testStmt("if (1 == 2) {} else {}", "(if (== 1 2) then (block) else (block))");

// Dangling else.
testStmt("if (1 == 2) if (3 == 4) 5; else 6;",
    "(if (== 1 2) then (if (== 3 4) then (; 5) else (; 6)))");

// Variable declaration.
testStmt("var name = 1 + 2;", "(var name = (+ 1 2))");
// TODO: Error if initializer is a statement.

// Return.
testStmt("return 1 or 2;", "(return (or 1 2))");
testStmt("return;", "(return)");

// While.
testStmt("while (1) 2;", "(while 1 (; 2))");
testStmt("while (1) while (2) 3;", "(while 1 (while 2 (; 3)))");
testStmt("while (1) {}", "(while 1 (block))");

// TODO: Test parse errors.
