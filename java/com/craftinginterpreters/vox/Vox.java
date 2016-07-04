//>= Framework
package com.craftinginterpreters.vox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
//>= Scanning
import java.util.List;
//>= Closures
import java.util.Map;
//>= Framework

public class Vox {
  private final ErrorReporter reporter = new ErrorReporter();
//>= Interpreting ASTs
  private final Interpreter interpreter;
//>= Framework
//>= Interpreting ASTs

  private Vox() {
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
    Vox vox = new Vox();

    if (args.length > 1) {
      System.out.println("Usage: jvox [script]");
    } else if (args.length == 1) {
      vox.runFile(args[0]);
    } else {
      vox.repl();
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
/*>= Parsing Expressions <= Interpreting ASTs

    Expr expression = parser.parseExpression();
*/
/*== Parsing Expressions

    // For now, just print the tree.
    System.out.println(new AstPrinter().print(expression));
*/
//>= Variables

    List<Stmt> statements = parser.parseProgram();
    if (reporter.hadError) return;
//>= Closures

    Resolver resolver = new Resolver(reporter);
    Map<Expr, Integer> locals = resolver.resolve(statements);
//>= Interpreting ASTs

    // Don't run if there was a syntax error.
    if (reporter.hadError) return;
/*== Interpreting ASTs

    interpreter.interpret(expression);
*/
//>= Variables

    interpreter.interpret(statements, locals);
//>= Framework
  }
}
