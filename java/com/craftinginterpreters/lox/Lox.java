//> Scanning lox-class
package com.craftinginterpreters.lox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;
//> Resolving and Binding not-yet
import java.util.Map;
//< Resolving and Binding not-yet

public class Lox {
//> Evaluating Expressions interpreter-instance
  private static final Interpreter interpreter = new Interpreter();
//< Evaluating Expressions interpreter-instance
//> had-error
  static boolean hadError = false;
//< had-error
//> Evaluating Expressions had-runtime-error-field
  static boolean hadRuntimeError = false;

//< Evaluating Expressions had-runtime-error-field
  public static void main(String[] args) throws IOException {
    if (args.length > 1) {
      System.out.println("Usage: jlox [script]");
    } else if (args.length == 1) {
      runFile(args[0]);
    } else {
      runPrompt();
    }
  }
//> run-file
  private static void runFile(String path) throws IOException {
    byte[] bytes = Files.readAllBytes(Paths.get(path));
    run(new String(bytes, Charset.defaultCharset()));
//> exit-code

    // Indicate an error in the exit code.
    if (hadError) System.exit(65);
//< exit-code
//> Evaluating Expressions check-runtime-error
    if (hadRuntimeError) System.exit(70);
//< Evaluating Expressions check-runtime-error
  }
//< run-file
//> prompt
  private static void runPrompt() throws IOException {
    InputStreamReader input = new InputStreamReader(System.in);
    BufferedReader reader = new BufferedReader(input);

    for (;;) { // [repl]
      System.out.print("> ");
      run(reader.readLine());
//> reset-had-error
      hadError = false;
//< reset-had-error
    }
  }
//< prompt
//> run
  private static void run(String source) {
    Scanner scanner = new Scanner(source);
    List<Token> tokens = scanner.scanTokens();
/* Scanning run < Parsing Expressions print-ast

    // For now, just print the tokens.
    for (Token token : tokens) {
      System.out.println(token);
    }
*/
//> Parsing Expressions print-ast
    Parser parser = new Parser(tokens);
/* Parsing Expressions print-ast < Statements and State parse-statements
    Expr expression = parser.parse();
*/
//> Statements and State parse-statements
    List<Stmt> statements = parser.parse();
//< Statements and State parse-statements

    // Stop if there was a syntax error.
    if (hadError) return;

//< Parsing Expressions print-ast
//> Resolving and Binding not-yet

    Resolver resolver = new Resolver();
    Map<Expr, Integer> locals = resolver.resolve(statements);

    // Stop if there was a resolution error.
    if (hadError) return;

/* Parsing Expressions print-ast < Evaluating Expressions interpreter-interpret
    System.out.println(new AstPrinter().print(expression));
*/
/* Evaluating Expressions interpreter-interpret < Statements and State interpret-statements
    interpreter.interpret(expression);
*/
/* Statements and State interpret-statements < Resolving and Binding not-yet
    interpreter.interpret(statements);
*/
    interpreter.interpret(statements, locals);
//< Resolving and Binding not-yet
  }
//< run
//> lox-error
  static void error(int line, String message) {
    report(line, "", message);
  }

  static private void report(int line, String where, String message) {
    System.err.println(
        "[line " + line + "] Error" + where + ": " + message);
    hadError = true;
  }
//< lox-error
//> Parsing Expressions token-error
  static void error(Token token, String message) {
    if (token.type == TokenType.EOF) {
      report(token.line, " at end", message);
    } else {
      report(token.line, " at '" + token.lexeme + "'", message);
    }
  }
//< Parsing Expressions token-error
//> Evaluating Expressions runtime-error-method
  static void runtimeError(RuntimeError error) {
    System.err.println(error.getMessage() +
        "\n[line " + error.token.line + "]");
    hadRuntimeError = true;
  }
//< Evaluating Expressions runtime-error-method
}
