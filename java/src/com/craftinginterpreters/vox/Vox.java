package com.craftinginterpreters.vox;

public class Vox {
  public static void main(String[] args) {
    System.out.println("Hey");

    Lexer lexer = new Lexer("if (1 == 2) {} else {}");

    while (true) {
      Token token = lexer.nextToken();
      System.out.println(token);
      if (token.type == TokenType.EOF) break;
    }

//    Parser parser = new Parser(lexer);
//
//    System.out.println(parser.parseProgram());
  }
}
