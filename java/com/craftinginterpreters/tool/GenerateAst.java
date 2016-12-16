//>= Representing Code 1
package com.craftinginterpreters.tool;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.List;

public class GenerateAst {
  private static String outputDir;

  public static void main(String[] args) throws IOException {
    if (args.length != 1) {
      System.err.println("Usage: generate_ast <output directory>");
      System.exit(1);
    }

    outputDir = args[0];

    defineAstType("Expr", Arrays.asList(
//>= Statements and State 1
        "Assign   : Token name, Expr value",
//>= Representing Code 1
        "Binary   : Expr left, Token operator, Expr right",
//>= Functions 1
        "Call     : Expr callee, Token paren, List<Expr> arguments",
//>= Classes 1
        "Get      : Expr object, Token name",
//>= Representing Code 1
        "Grouping : Expr expression",
        "Literal  : Object value",
//>= Control Flow 1
        "Logical  : Expr left, Token operator, Expr right",
//>= Classes 1
        "Set      : Expr object, Token name, Expr value",
//>= Inheritance 1
        "Super    : Token keyword, Token method",
//>= Classes 1
        "This     : Token keyword",
//>= Representing Code 1
        "Unary    : Token operator, Expr right",
//>= Statements and State 1
        "Variable : Token name"
//>= Representing Code 1
    ));
//>= Statements and State 1

    defineAstType("Stmt", Arrays.asList(
//>= Statements and State 1
        "Block       : List<Stmt> statements",
/*>= Classes 1 < Inheritance 1
        "Class       : Token name, List<Stmt.Function> methods",
*/
//>= Inheritance 1
        "Class       : Token name, Expr superclass, List<Stmt.Function> methods",
//>= Statements and State 1
        "Expression  : Expr expression",
//>= Functions 1
        "Function    : Token name, List<Token> parameters, List<Stmt> body",
//>= Control Flow 1
        "If          : Expr condition, Stmt thenBranch, Stmt elseBranch",
//>= Statements and State 1
        "Print       : Expr expression",
//>= Functions 1
        "Return      : Token keyword, Expr value",
//>= Statements and State 1
        "Var         : Token name, Expr initializer",
//>= Control Flow 1
        "While       : Expr condition, Stmt body"
//>= Statements and State 1
    ));
//>= Representing Code 1
  }

  private static void defineAstType(String baseName, List<String> types)
      throws IOException {
    String path = outputDir + "/" + baseName + ".java";
    PrintWriter writer = new PrintWriter(path, "UTF-8");

    writer.println("package com.craftinginterpreters.lox;");
    writer.println("");
    writer.println("import java.util.List;");
    writer.println("");
    writer.println("abstract class " + baseName + " {");

    defineVisitor(writer, baseName, types);

    // The AST classes.
    for (String type : types) {
      String className = type.split(":")[0].trim();
      String fields = type.split(":")[1].trim();
      defineType(writer, baseName, className, fields);
    }

    // The base accept() method.
    writer.println("");
    writer.println("  abstract <R> R accept(Visitor<R> visitor);");

    writer.println("}");
    writer.close();

    System.out.println("Generated " + path);
  }

  private static void defineVisitor(
      PrintWriter writer, String baseName, List<String> types) {
    writer.println("  interface Visitor<R> {");

    for (String type : types) {
      String typeName = type.split(":")[0].trim();
      writer.println("    R visit" + typeName + baseName + "(" +
          typeName + " " + baseName.toLowerCase() + ");");
    }

    writer.println("  }");
  }

  private static void defineType(PrintWriter writer, String baseName,
                                 String className, String fieldList) {
    writer.println("");
    writer.println("  static class " + className + " extends " +
        baseName + " {");

    // Constructor.
    writer.println("    " + className + "(" + fieldList + ") {");

    // Store parameters in fields.
    String[] fields = fieldList.split(", ");
    for (String field : fields) {
      String name = field.split(" ")[1];
      writer.println("      this." + name + " = " + name + ";");
    }

    writer.println("    }");

    // Visitor pattern.
    writer.println();
    writer.println(
        "    <R> R accept(Visitor<R> visitor) {");
    writer.println("      return visitor.visit" + className +
        baseName + "(this);");
    writer.println("    }");

    // Fields.
    writer.println();
    for (String field : fields) {
      writer.println("    final " + field + ";");
    }

    writer.println("  }");
  }
}