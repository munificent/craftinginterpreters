#!/usr/bin/env node
"use strict";

var fs = require("fs");
var process = require("process");
var readline = require("readline");

var interpreter = require("./interpreter");
var Interpreter = interpreter.Interpreter;
var RuntimeError = interpreter.RuntimeError;

var Lexer = require("./lexer");
var Parser = require("./parser");
var Token = require("./token");

var interpreter = new Interpreter();

if (process.argv.length == 2) {
  repl();
} else if (process.argv.length == 3) {
  runFile(process.argv[2]);
} else {
  console.log("Usage: vox [file]");
}

function execute(source) {
  var errorReporter = {
    error: function(message) {
      if (!errorReporter.needsMoreInput) {
        console.log(":( Syntax Error: " + message);
      }
    }
  };

  var lexer = new Lexer(source);
  var parser = new Parser(lexer, errorReporter);
  var program = parser.parseProgram();

  if (!errorReporter.hasError) {
    try {
      interpreter.interpret(program);
    } catch (error) {
      if (!(error instanceof RuntimeError)) throw error;

      console.log(":( Runtime Error: " + error.message);
    }
  }

  return errorReporter.needsMoreInput;
}

function repl() {
  var source = "";

  var io = readline.createInterface({
    input: process.stdin,
    output: process.stdout
  });

  io.setPrompt(">> ");
  io.prompt();

  io.on("line", function(line) {
    source += "\n" + line;

    if (execute(source)) {
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
}

function runFile(path) {
  var source = fs.readFileSync(path, "utf-8");
  execute(source);
}
