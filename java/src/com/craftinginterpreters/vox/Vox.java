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
    Interpreter interpreter = new Interpreter(errorReporter);

    if (args.length == 1) {
      interpreter.run(readFile(args[0]));

      if (errorReporter.hadError) System.exit(65);
      return;
    }

    // No file argument, so run the REPL.
    while (true) {
      InputStreamReader input = new InputStreamReader(System.in);
      BufferedReader reader = new BufferedReader(input);

      System.out.print("> ");
      String source = reader.readLine();

      try {
        errorReporter.hadError = false;
        interpreter.run(source);
      } catch (RuntimeError error) {
        System.err.println("Runtime error: " + error.getMessage());
      }
    }
  }

  static String readFile(String path) throws IOException {
    byte[] bytes = Files.readAllBytes(Paths.get(path));
    return new String(bytes, Charset.defaultCharset());
  }
}
