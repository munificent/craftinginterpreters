package com.craftinginterpreters.vox;

public class Vox {
  public static void main(String[] args) {

    Lexer lexer = new Lexer("class Foo < Bar { foo(a, b) {} }");

//    while (true) {
//      Token token = lexer.nextToken();
//      System.out.println(token);
//      if (token.type == TokenType.EOF) break;
//    }

    Parser parser = new Parser(lexer, null);
    Stmt stmt = parser.parseStatement();
    AstPrinter printer = new AstPrinter();
    System.out.println(printer.print(stmt));
  }
}
