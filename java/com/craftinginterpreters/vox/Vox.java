//>= Framework
package com.craftinginterpreters.vox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;

public class Vox {
  public static void main(String[] args) throws IOException {
    if (args.length > 1) {
      System.out.println("Usage: jvox [script]");
      return;
    }

    ErrorReporter errorReporter = new ErrorReporter();
//>= Interpreting ASTs
    Interpreter interpreter = new Interpreter(errorReporter);

//>= Framework
    if (args.length == 1) {
      String source = readFile(args[0]);

/*== Framework
      // For now, just echo the source back.
      System.out.println(source);
*/
//>= Interpreting ASTs
      if (!interpret(interpreter, source)) System.exit(70);

//>= Framework

      // Indicate a compile error in the exit code.
      if (errorReporter.hadError) System.exit(65);
      return;
    }

    // No file argument, so run the REPL.
    for (;;) {
      InputStreamReader input = new InputStreamReader(System.in);
      BufferedReader reader = new BufferedReader(input);

      System.out.print("> ");
      String source = reader.readLine();

/*== Framework
      // For now, just echo the source back.
      System.out.println(": " + source);
*/
//>= Interpreting ASTs
      interpret(interpreter, source);
//>= Framework
    }
  }

//>= Interpreting ASTs
  private static boolean interpret(Interpreter interpreter,
                                   String source) {
    try {
      interpreter.run(source);
    } catch (RuntimeError error) {
      System.err.println(error);
      return false;
    }

    return true;
  }

//>= Framework
  private static String readFile(String path) throws IOException {
    byte[] bytes = Files.readAllBytes(Paths.get(path));
    return new String(bytes, Charset.defaultCharset());
  }
}
