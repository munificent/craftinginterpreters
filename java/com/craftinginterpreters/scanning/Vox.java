package com.craftinginterpreters.scanning;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class Vox {
  public static void main(String[] args) throws IOException {
    while (true) {
      InputStreamReader input = new InputStreamReader(System.in);
      BufferedReader reader = new BufferedReader(input);

      System.out.print("> ");
      String source = reader.readLine();

      Scanner scanner = new Scanner(source);
      while (true) {
        Token token = scanner.readToken();
        System.out.println(token);
        if (token.type == TokenType.EOF) break;
      }
    }
  }
}
