"use strict";

var readline = require('readline');

var interpreter = require("./interpreter");
var Interpreter = interpreter.Interpreter;
var RuntimeError = interpreter.RuntimeError;

var Lexer = require("./lexer");
var Parser = require("./parser");
var Token = require("./token");
var prettyPrint = require("./pretty_print");

var io = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

io.setPrompt(">> ");
io.prompt();

var interpreter = new Interpreter();
var source = "";

io.on("line", function(line) {
  source += "\n" + line;

  var errorReporter = {
    error: function(message) {
      if (!errorReporter.needsMoreInput) {
        console.log(":( Syntax Error: " + message);
      }
    }
  };

  var lexer = new Lexer(source);
  var parser = new Parser(lexer, errorReporter);
  var statement = parser.parse();

  if (!errorReporter.hasError) {
    // console.log(" = " + prettyPrint(statement));
    try {
      interpreter.interpret(statement);
    } catch (error) {
      if (!(error instanceof RuntimeError)) throw error;

      console.log(":( Runtime Error: " + error.message);
    }
  }

  if (errorReporter.needsMoreInput) {
    io.setPrompt(" | ");
  } else {
    source = "";
    io.setPrompt(">> ");
  }

  io.prompt();
});

io.on("SIGINT", function() {
  console.log("");
  io.close();
});
