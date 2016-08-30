//>= Framework
package com.craftinginterpreters.lox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
//>= Scanning
import java.util.List;
//>= Resolving and Binding
import java.util.Map;
//>= Framework

public class Lox {
  private final ErrorReporter reporter = new ErrorReporter();
//>= Evaluating Expressions
  private final Interpreter interpreter;
//>= Evaluating Expressions

  private Lox() {
    interpreter = new Interpreter(reporter);
  }
//>= Framework

  private void runFile(String path) throws IOException {
    byte[] bytes = Files.readAllBytes(Paths.get(path));
    String source = new String(bytes, Charset.defaultCharset());

    run(source);

    // Indicate an error in the exit code.
    if (reporter.hadError) System.exit(65);
    if (reporter.hadRuntimeError) System.exit(70);
  }

  private void repl() throws IOException {
    InputStreamReader input = new InputStreamReader(System.in);
    BufferedReader reader = new BufferedReader(input);

    for (;;) {
      System.out.print("> ");
      String source = reader.readLine();
      run(source);
      reporter.reset();
    }
  }

  public static void main(String[] args) throws IOException {
    Lox lox = new Lox();

    if (args.length > 1) {
      System.out.println("Usage: jlox [script]");
    } else if (args.length == 1) {
      lox.runFile(args[0]);
    } else {
      lox.repl();
    }
  }

//>= Framework
  private void run(String source) {
/*== Framework
    // For now, just echo the source back.
    System.out.println(source);
*/
//>= Scanning
    Scanner scanner = new Scanner(source, reporter);
    List<Token> tokens = scanner.scanTokens();
/*== Scanning
    // For now, just print the tokens.
    for (Token token : tokens) {
      System.out.println(token);
    }
*/
//>= Parsing Expressions

    Parser parser = new Parser(tokens, reporter);
/*>= Parsing Expressions < Statements and State
    Expr expression = parser.parseExpression();
*/
/*== Parsing Expressions

    // For now, just print the tree.
    System.out.println(new AstPrinter().print(expression));
*/
//>= Statements and State
    List<Stmt> statements = parser.parseProgram();

    // Stop if there was a syntax error.
    if (reporter.hadError) return;
//>= Resolving and Binding

    Resolver resolver = new Resolver(reporter);
    Map<Expr, Integer> locals = resolver.resolve(statements);

    // Stop if there was a resolution error.
    if (reporter.hadError) return;

/*== Evaluating Expressions
    interpreter.interpret(expression);
*/
/*>= Statements and State < Resolving and Binding
    interpreter.interpret(statements);
*/
//>= Resolving and Binding
    interpreter.interpret(statements, locals);
//>= Framework
  }
}
