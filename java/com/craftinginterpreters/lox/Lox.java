//>> Scanning 1
package com.craftinginterpreters.lox;

//>> 6
import java.io.BufferedReader;
//<< 6
import java.io.IOException;
//>> 7
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;
//<< 7
//>> Resolving and Binding 99
import java.util.Map;
//<< Resolving and Binding 99

public class Lox {
//>> 8
  private final ErrorReporter reporter = new ErrorReporter();
//>> Evaluating Expressions 99
  private final Interpreter interpreter;

  private Lox() {
    interpreter = new Interpreter(reporter);
  }
//<< Evaluating Expressions 99

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

//<< 8
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
//>> 9

  private void run(String source) {
    Scanner scanner = new Scanner(source, reporter);
    List<Token> tokens = scanner.scanTokens();
/*>= Scanning 10 < Representing Code 1
    // For now, just print the tokens.
    for (Token token : tokens) {
      System.out.println(token);
    }
*/
//>> Parsing Expressions 99

    Parser parser = new Parser(tokens, reporter);
//<< Parsing Expressions 99
/*>= Parsing Expressions 99 < Statements and State 99
    Expr expression = parser.parseExpression();
*/
/*>= Parsing Expressions 99 < Evaluating Expressions 99

    // For now, just print the tree.
    System.out.println(new AstPrinter().print(expression));
*/
//>> Statements and State 99
    List<Stmt> statements = parser.parseProgram();

    // Stop if there was a syntax error.
    if (reporter.hadError) return;
//<< Statements and State 99
//>> Resolving and Binding 99

    Resolver resolver = new Resolver(reporter);
    Map<Expr, Integer> locals = resolver.resolve(statements);

    // Stop if there was a resolution error.
    if (reporter.hadError) return;

/*>= Evaluating Expressions 99 < Statements and State 99
    interpreter.interpret(expression);
*/
/*>= Statements and State 99 < Resolving and Binding 99
    interpreter.interpret(statements);
*/
    interpreter.interpret(statements, locals);
//<< Resolving and Binding 99
  }
//<< 9
}
