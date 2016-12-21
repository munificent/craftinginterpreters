//> Scanning 1
package com.craftinginterpreters.lox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;
//> Resolving and Binding 99
import java.util.Map;
//< Resolving and Binding 99

public class Lox {
  static boolean hadError = false;
//> Evaluating Expressions 99
  static boolean hadRuntimeError = false;

  private static final Interpreter interpreter = new Interpreter();

//< Evaluating Expressions 99
  public static void main(String[] args) throws IOException {
    if (args.length > 1) {
      System.out.println("Usage: jlox [script]");
    } else if (args.length == 1) {
      runFile(args[0]);
    } else {
      repl();
    }
  }
//> 2

  private static void runFile(String path) throws IOException {
    byte[] bytes = Files.readAllBytes(Paths.get(path));
    String source = new String(bytes, Charset.defaultCharset());
//> 99
    run(source);

    // Indicate an error in the exit code.
    if (hadError) System.exit(65);
//> Evaluating Expressions 99
    if (hadRuntimeError) System.exit(70);
//< Evaluating Expressions 99
//< 99
  }
//< 2
//> 3

  private static void repl() throws IOException {
    InputStreamReader input = new InputStreamReader(System.in);
    BufferedReader reader = new BufferedReader(input);

    for (;;) {
      System.out.print("> ");
      String source = reader.readLine();
//> 99
      run(source);
      hadError = false;
//> Evaluating Expressions 99
      hadRuntimeError = false;
//< Evaluating Expressions 99
//< 99
    }
  }
//< 3
//> 9

  private static void run(String source) {
    Scanner scanner = new Scanner(source);
    List<Token> tokens = scanner.scanTokens();
/* Scanning 10 < Representing Code 99
    // For now, just print the tokens.
    for (Token token : tokens) {
      System.out.println(token);
    }
*/
//> Parsing Expressions 99

    Parser parser = new Parser(tokens);
//< Parsing Expressions 99
/* Parsing Expressions 99 < Statements and State 99
    Expr expression = parser.parseExpression();
*/
/* Parsing Expressions 99 < Evaluating Expressions 99

    // For now, just print the tree.
    System.out.println(new AstPrinter().print(expression));
*/
//> Statements and State 99
    List<Stmt> statements = parser.parseProgram();

    // Stop if there was a syntax error.
    if (hadError) return;
//< Statements and State 99
//> Resolving and Binding 99

    Resolver resolver = new Resolver();
    Map<Expr, Integer> locals = resolver.resolve(statements);

    // Stop if there was a resolution error.
    if (hadError) return;

/* Evaluating Expressions 99 < Statements and State 99
    interpreter.interpret(expression);
*/
/* Statements and State 99 < Resolving and Binding 99
    interpreter.interpret(statements);
*/
    interpreter.interpret(statements, locals);
//< Resolving and Binding 99
  }
//< 9

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

//> Evaluating Expressions 99
  // TODO: Stack trace?
  static void runtimeError(RuntimeError error) {
    System.err.println(error.getMessage() +
        "\n[line " + error.token.line + "]");
    hadRuntimeError = true;
  }

//< Evaluating Expressions 99
  static private void report(int line, String location, String message) {
    System.err.println("[line " + line + "] Error" + location +
        ": " + message);
    hadError = true;
  }
}
