//>= Framework
package com.craftinginterpreters.vox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;
import java.util.Map;

public class Vox {
  private final ErrorReporter reporter = new ErrorReporter();
//>= Interpreting ASTs
  private final Interpreter interpreter;
//>= Framework

  private Vox() {
//>= Interpreting ASTs
    interpreter = new Interpreter(reporter);
//>= Framework
  }

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

//>= Interpreting ASTs
  void run(String source) {
/*== Framework
    // For now, just echo the source back.
    System.out.println(": " + source);
*/
//>= Scanning
    Scanner scanner = new Scanner(source, reporter);
    List<Token> tokens = scanner.scanTokens();
//>= Parsing Expressions

    Parser parser = new Parser(tokens, reporter);
//>= Variables

    List<Stmt> statements = parser.parseProgram();
    if (reporter.hadError) return;
//>= Closures

    Resolver resolver = new Resolver(reporter);
    Map<Expr, Integer> locals = resolver.resolve(statements);
//>= Interpreting ASTs

    // Don't run if there was a syntax error.
    if (reporter.hadError) return;
//>= Variables

    interpreter.interpret(statements, locals);
    // TODO: Interpret expressions for AST chapter.
//>= Interpreting ASTs
  }
}
