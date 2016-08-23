//>= The Pancake Language
package com.craftinginterpreters.pancake;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.*;

public class Pancake {
  private enum Result {
    COMPILE_ERROR,
    RUNTIME_ERROR,
    SUCCESS
  }

  private final Map<String, Runnable> dictionary = new HashMap<>();
  private final List<Object> code = new ArrayList<>();
  private final Stack<Double> stack = new Stack<>();

  private Pancake() {
    dictionary.put("+", this::plus);
    dictionary.put("-", this::minus);
    dictionary.put("*", this::times);
    dictionary.put("/", this::divide);
    dictionary.put("print", this::print);
  }

  private void runFile(String path) throws IOException {
    byte[] bytes = Files.readAllBytes(Paths.get(path));
    String source = new String(bytes, Charset.defaultCharset());

    Result result = run(source);

    // Indicate an error in the exit code.
    if (result == Result.COMPILE_ERROR) System.exit(65);
    if (result == Result.RUNTIME_ERROR) System.exit(70);
  }

  private void repl() throws IOException {
    InputStreamReader input = new InputStreamReader(System.in);
    BufferedReader reader = new BufferedReader(input);

    for (;;) {
      // Show the current stack contents.
      for (double value : stack) {
        System.out.print(value);
        System.out.print(" ");
      }
      System.out.print("> ");

      String source = reader.readLine();
      run(source);
    }
  }

  public static void main(String[] args) throws IOException {
    Pancake pancake = new Pancake();

    if (args.length > 1) {
      System.out.println("Usage: pancake [script]");
    } else if (args.length == 1) {
      pancake.runFile(args[0]);
    } else {
      pancake.repl();
    }
  }

  private Result run(String source) {
    Result result = Result.SUCCESS;

    String[] words = source.split(" ");

    boolean inComment = false;
    for (String word : words) {
      if (inComment) {
        if (word.equals(")")) inComment = false;
      } else if (word.equals("(")) {
        inComment = true;
      } else {
        Runnable definition = dictionary.get(word);
        if (definition != null) {
          code.add(definition);
        } else {
          try {
            code.add(Double.parseDouble(word));
          } catch (NumberFormatException ex) {
            System.err.println("'" + word + "' is not a valid word.");
            result = Result.COMPILE_ERROR;
          }
        }
      }
    }

    if (inComment) {
      System.err.println("Unterminated comment.");
      result = Result.COMPILE_ERROR;
    }

    // Don't run if there is a compile error.
    if (result != Result.SUCCESS) return result;

    for (Object instruction : code) {
      if (instruction instanceof Runnable) {
        try {
          ((Runnable)instruction).run();
        } catch (EmptyStackException ex) {
          System.err.println("Stack underflow.");
          result = Result.RUNTIME_ERROR;
        }
      } else {
        stack.push((Double)instruction);
      }
    }

    return result;
  }

  private void plus() {
    double b = stack.pop();
    double a = stack.pop();
    stack.push(a + b);
  }

  private void minus() {
    double b = stack.pop();
    double a = stack.pop();
    stack.push(a - b);
  }

  private void times() {
    double b = stack.pop();
    double a = stack.pop();
    stack.push(a * b);
  }

  private void divide() {
    double b = stack.pop();
    double a = stack.pop();
    stack.push(a / b);
  }

  private void print() {
    double a = stack.pop();
    System.out.println(a);
  }
}
