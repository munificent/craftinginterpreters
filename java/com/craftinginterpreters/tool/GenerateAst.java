//> Representing Code generate-ast
package com.craftinginterpreters.tool;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.List;

public class GenerateAst {
  public static void main(String[] args) throws IOException {
    if (args.length != 1) {
      System.err.println("Usage: generate_ast <output directory>");
      System.exit(1);
    }
    String outputDir = args[0];
//> call-define-ast
    defineAst(outputDir, "Expr", Arrays.asList(
//> Statements and State not-yet
      "Assign   : Token name, Expr value",
//< Statements and State not-yet
      "Binary   : Expr left, Token operator, Expr right",
//> Functions not-yet
      "Call     : Expr callee, Token paren, List<Expr> arguments",
//< Functions not-yet
//> Classes not-yet
      "Get      : Expr object, Token name",
//< Classes not-yet
      "Grouping : Expr expression",
      "Literal  : Object value",
//> Control Flow not-yet
      "Logical  : Expr left, Token operator, Expr right",
//< Control Flow not-yet
//> Classes not-yet
      "Set      : Expr object, Token name, Expr value",
//< Classes not-yet
//> Inheritance not-yet
      "Super    : Token keyword, Token method",
//< Inheritance not-yet
//> Classes not-yet
      "This     : Token keyword",
//< Classes not-yet
      "Unary    : Token operator, Expr right",
//> Statements and State not-yet
      "Variable : Token name"
//< Statements and State not-yet
    ));
//> Statements and State not-yet

    defineAst(outputDir, "Stmt", Arrays.asList(
      "Block      : List<Stmt> statements",
/* Classes not-yet < Inheritance not-yet
      "Class      : Token name, List<Stmt.Function> methods",
*/
//> Inheritance not-yet
      "Class      : Token name, Expr superclass, List<Stmt.Function> methods",
//< Inheritance not-yet
      "Expression : Expr expression",
//> Functions not-yet
      "Function   : Token name, List<Token> parameters, List<Stmt> body",
//< Functions not-yet
//> Control Flow not-yet
      "If         : Expr condition, Stmt thenBranch, Stmt elseBranch",
//< Control Flow not-yet
      "Print      : Expr expression",
//> Functions not-yet
      "Return     : Token keyword, Expr value",
//< Functions not-yet
      "Var        : Token name, Expr initializer",
//> Control Flow not-yet
      "While      : Expr condition, Stmt body"
//< Control Flow not-yet
    ));
//< Statements and State not-yet
//< call-define-ast
  }
//> define-ast
  private static void defineAst(
      String outputDir, String baseName, List<String> types)
      throws IOException {
    String path = outputDir + "/" + baseName + ".java";
    PrintWriter writer = new PrintWriter(path, "UTF-8");

    writer.println("package com.craftinginterpreters.lox;");
    writer.println("");
    writer.println("import java.util.List;");
    writer.println("");
    writer.println("abstract class " + baseName + " {");

//> call-define-visitor
    defineVisitor(writer, baseName, types);

//< call-define-visitor
//> nested-classes
    // The AST classes.
    for (String type : types) {
      String className = type.split(":")[0].trim();
      String fields = type.split(":")[1].trim();
      defineType(writer, baseName, className, fields);
    }
//< nested-classes
//> base-accept-method

    // The base accept() method.
    writer.println("");
    writer.println("  abstract <R> R accept(Visitor<R> visitor);");

//< base-accept-method
    writer.println("}");
    writer.close();
  }
//< define-ast
//> define-visitor
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
//< define-visitor
//> define-type
  private static void defineType(
      PrintWriter writer, String baseName,
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
//> accept-method

    // Visitor pattern.
    writer.println();
    writer.println("    <R> R accept(Visitor<R> visitor) {");
    writer.println("      return visitor.visit" +
        className + baseName + "(this);");
    writer.println("    }");
//< accept-method

    // Fields.
    writer.println();
    for (String field : fields) {
      writer.println("    final " + field + ";");
    }

    writer.println("  }");
  }
//< define-type
//> pastry-visitor
  interface PastryVisitor {
    void visitBeignet(Beignet beignet); // [overload]
    void visitCruller(Cruller cruller);
  }
//< pastry-visitor
//> pastries
  abstract class Pastry {
//> pastry-accept
    abstract void accept(PastryVisitor visitor);
//< pastry-accept
  }

  class Beignet extends Pastry {
//> beignet-accept
    @Override
    void accept(PastryVisitor visitor) {
      visitor.visitBeignet(this);
    }
//< beignet-accept
  }

  class Cruller extends Pastry {
//> cruller-accept
    @Override
    void accept(PastryVisitor visitor) {
      visitor.visitCruller(this);
    }
//< cruller-accept
  }
//< pastries
}
