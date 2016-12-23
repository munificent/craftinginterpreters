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
//> not-yet
  static boolean hadError = false;
//> Evaluating Expressions not-yet
  static boolean hadRuntimeError = false;

  private static final Interpreter interpreter = new Interpreter();
//< Evaluating Expressions not-yet

//< not-yet
  public static void main(String[] args) throws IOException {
    if (args.length > 1) {
      System.out.println("Usage: jlox [script]");
    } else if (args.length == 1) {
      runFile(args[0]);
    } else {
      repl();
    }
  }
//> run-file

  private static void runFile(String path) throws IOException {
    byte[] bytes = Files.readAllBytes(Paths.get(path));
    run(new String(bytes, Charset.defaultCharset()));
//> not-yet

    // Indicate an error in the exit code.
    if (hadError) System.exit(65);
//< not-yet
//> Evaluating Expressions not-yet
    if (hadRuntimeError) System.exit(70);
//< Evaluating Expressions not-yet
  }
//< run-file
//> repl

  private static void repl() throws IOException {
    InputStreamReader input = new InputStreamReader(System.in);
    BufferedReader reader = new BufferedReader(input);

    for (;;) {
      System.out.print("> ");
      run(reader.readLine());
    }
  }
//< repl
//> not-yet

  private static void run(String source) {
/* Scanning not-yet < Scanning not-yet
    // For now, just print the source.
    System.out.println(source);
*/
    Scanner scanner = new Scanner(source);
    List<Token> tokens = scanner.scanTokens();
/* Scanning print-tokens < Representing Code not-yet
    // For now, just print the tokens.
    for (Token token : tokens) {
      System.out.println(token);
    }
*/
//> Parsing Expressions not-yet

    Parser parser = new Parser(tokens);
//< Parsing Expressions not-yet
/* Parsing Expressions not-yet < Statements and State not-yet
    Expr expression = parser.parseExpression();
*/
/* Parsing Expressions not-yet < Evaluating Expressions not-yet

    // For now, just print the tree.
    System.out.println(new AstPrinter().print(expression));
*/
//> Statements and State not-yet
    List<Stmt> statements = parser.parseProgram();

    // Stop if there was a syntax error.
    if (hadError) return;
//< Statements and State not-yet
//> Resolving and Binding not-yet

    Resolver resolver = new Resolver();
    Map<Expr, Integer> locals = resolver.resolve(statements);

    // Stop if there was a resolution error.
    if (hadError) return;

/* Evaluating Expressions not-yet < Statements and State not-yet
    interpreter.interpret(expression);
*/
/* Statements and State not-yet < Resolving and Binding not-yet
    interpreter.interpret(statements);
*/
    interpreter.interpret(statements, locals);
//< Resolving and Binding not-yet
  }
//< not-yet
//> not-yet
  static void error(int line, String message) {
    report(line, "", message);
  }

  static void error(Token token, String message) {
    if (token.type == TokenType.EOF) {
      report(token.line, " at end", message);
    } else {
      report(token.line, " at '" + token.text + "'", message);
    }
  }
//< not-yet
//> Evaluating Expressions not-yet

  // TODO: Stack trace?
  static void runtimeError(RuntimeError error) {
    System.err.println(error.getMessage() +
        "\n[line " + error.token.line + "]");
    hadRuntimeError = true;
  }

//< Evaluating Expressions not-yet
//> not-yet
  static private void report(int line, String location, String message) {
    System.err.println("[line " + line + "] Error" + location +
        ": " + message);
    hadError = true;
  }
//< not-yet
}
