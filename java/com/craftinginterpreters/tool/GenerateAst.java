//>= Representing Code
package com.craftinginterpreters.tool;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.List;

public class GenerateAst {
  static String outputDir;

  public static void main(String[] args) throws IOException {
    if (args.length != 1) {
      System.err.println("Usage: generate_ast <output directory>");
      System.exit(1);
    }

    outputDir = args[0];

    defineAstType("Expr", Arrays.asList(
//>= Statements and State
        "Assign   : Token name, Expr value",
//>= Representing Code
        "Binary   : Expr left, Token operator, Expr right",
//>= Functions
        "Call     : Expr callee, Token paren, List<Expr> arguments",
//>= Classes
        "Get      : Expr object, Token name",
//>= Representing Code
        "Grouping : Expr expression",
        "Literal  : Object value",
//>= Control Flow
        "Logical  : Expr left, Token operator, Expr right",
//>= Classes
        "Set      : Expr object, Token name, Expr value",
//>= Inheritance
        "Super    : Token keyword, Token method",
//>= Classes
        "This     : Token keyword",
//>= Representing Code
        "Unary    : Token operator, Expr right",
//>= Statements and State
        "Variable : Token name"
//>= Representing Code
    ));
//>= Statements and State

    defineAstType("Stmt", Arrays.asList(
//>= Statements and State
        "Block       : List<Stmt> statements",
/*== Classes
        "Class       : Token name, List<Stmt.Function> methods",
*/
//>= Inheritance
        "Class       : Token name, Expr superclass, List<Stmt.Function> methods",
//>= Statements and State
        "Expression  : Expr expression",
//>= Functions
        "Function    : Token name, List<Token> parameters, List<Stmt> body",
//>= Control Flow
        "If          : Expr condition, Stmt thenBranch, Stmt elseBranch",
//>= Statements and State
        "Print       : Expr expression",
//>= Functions
        "Return      : Token keyword, Expr value",
//>= Statements and State
        "Var         : Token name, Expr initializer",
//>= Control Flow
        "While       : Expr condition, Stmt body"
//>= Statements and State
    ));
//>= Representing Code
  }

  private static void defineAstType(String baseName, List<String> types)
      throws IOException {
    String path = outputDir + "/" + baseName + ".java";
    PrintWriter writer = new PrintWriter(path, "UTF-8");

    writer.println("package com.craftinginterpreters.vox;");
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