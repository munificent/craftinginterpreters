package com.craftinginterpreters.vox;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.List;

public class GenerateAst {
  public static void main(String[] args) throws IOException {
    defineAstType("Expr", Arrays.asList(
        "Assign   : Expr object, Token name, Expr value",
        "Binary   : Expr left, Token operator, Expr right",
        "Call     : Expr callee, Token paren, List<Expr> arguments",
        "Grouping : Expr expression",
        "Literal  : Object value",
        "Logical  : Expr left, Token operator, Expr right",
        "Property : Expr object, Token name",
        "Unary    : Token operator, Expr right",
        "Variable : Token name"
    ));

    defineAstType("Stmt", Arrays.asList(
        "Block       : List<Stmt> statements",
        "Class       : Token name, Expr superclass, List<Stmt.Function> methods",
        "Expression  : Expr expression",
        "For         : Token name, Expr iterator, Stmt body",
        "Function    : Token name, List<Token> parameters, Stmt body",
        "If          : Expr condition, Stmt thenBranch, Stmt elseBranch",
        "Return      : Expr value",
        "Var         : Token name, Expr initializer",
        "While       : Expr condition, Stmt body"
    ));
  }

  private static void defineAstType(String baseName, List<String> types)
      throws IOException {
    PrintWriter writer = new PrintWriter(
        "com/craftinginterpreters/vox/" + baseName + ".java", "UTF-8");

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
    writer.println("    abstract <R, C> R accept(Visitor<R, C> visitor, C context);");

    writer.println("}");
    writer.close();
  }

  private static void defineVisitor(PrintWriter writer, String baseName,
                                    List<String> types) {
    writer.println("  interface Visitor<R, C> {");

    for (String type : types) {
      String typeName = type.split(":")[0].trim();
      writer.println("    R visit" + typeName + baseName + "(" +
          typeName + " " + baseName.toLowerCase() + ", C context);");
    }

    writer.println("  }");
  }

  private static void defineType(PrintWriter writer, String baseName,
                                 String className, String fieldList) {
    writer.println("");
    writer.println("  static class " + className + " extends " + baseName + " {");

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
    writer.println("    <R, C> R accept(Visitor<R, C> visitor, C context) {");
    writer.println("      return visitor.visit" + className + baseName + "(this, context);");
    writer.println("    }");

    // Fields.
    writer.println();
    for (String field : fields) {
      writer.println("    final " + field + ";");
    }

    writer.println("  }");
  }
}