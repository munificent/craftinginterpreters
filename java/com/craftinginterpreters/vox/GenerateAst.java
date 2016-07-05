//>= Syntax Trees
package com.craftinginterpreters.vox;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.List;

public class GenerateAst {
  public static void main(String[] args) throws IOException {
    defineAstType("Expr", Arrays.asList(
        // TODO: Don't have object until Classes chapter.
//>= Variables
        "Assign   : Expr object, Token name, Expr value",
//>= Syntax Trees
        "Binary   : Expr left, Token operator, Expr right",
//>= Functions
        "Call     : Expr callee, Token paren, List<Expr> arguments",
//>= Syntax Trees
        "Grouping : Expr expression",
        "Literal  : Object value",
//>= Control Flow
        "Logical  : Expr left, Token operator, Expr right",
//>= Classes
        "Property : Expr object, Token name",
//>= Inheritance
        "Super    : Token keyword, Token method",
//>= Classes
        "This     : Token name",
//>= Syntax Trees
        "Unary    : Token operator, Expr right",
//>= Variables
        "Variable : Token name"
//>= Syntax Trees
    ));
//>= Variables

    defineAstType("Stmt", Arrays.asList(
        "Block       : List<Stmt> statements",
//>= Classes
        "Class       : Token name, Expr superclass, List<Stmt.Function> methods",
//>= Variables
        "Expression  : Expr expression",
//>= Uhh
        "For         : Token name, Expr iterator, Stmt body",
//>= Functions
        "Function    : Token name, List<Token> parameters, Stmt body",
//>= Control Flow
        "If          : Expr condition, Stmt thenBranch, Stmt elseBranch",
//>= Functions
        "Return      : Token keyword, Expr value",
//>= Variables
        "Var         : Token name, Expr initializer",
//>= Control Flow
        "While       : Expr condition, Stmt body"
//>= Variables
    ));
//>= Syntax Trees
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
    writer.println("  abstract <R> R accept(Visitor<R> visitor);");

    writer.println("}");
    writer.close();
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