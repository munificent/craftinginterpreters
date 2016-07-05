//>= Syntax Trees
package com.craftinginterpreters.vox;

import java.util.Arrays;

// Creates an unambiguous, if ugly, string representation of AST nodes.
/*>= Syntax Trees <= Interpreting ASTs
class AstPrinter implements Expr.Visitor<String> {
*/
//>= Variables
class AstPrinter implements Expr.Visitor<String>, Stmt.Visitor<String> {
//>= Syntax Trees
  String print(Expr expr) {
    return expr.accept(this);
  }
//>= Variables

  String print(Stmt stmt) {
    return stmt.accept(this);
  }

  @Override
  public String visitAssignExpr(Expr.Assign expr) {
    return join("=", expr.name.text, expr.value);
  }
//>= Syntax Trees

  @Override
  public String visitBinaryExpr(Expr.Binary expr) {
    return join(expr.operator, expr.left, expr.right);
  }
//>= Functions

  @Override
  public String visitCallExpr(Expr.Call expr) {
    return join("call", expr.callee, expr.arguments);
  }
//>= Classes

  @Override
  public String visitGetExpr(Expr.Get expr) {
    return join(".", expr.object, expr.name.text);
  }
//>= Syntax Trees

  @Override
  public String visitGroupingExpr(Expr.Grouping expr) {
    return join("group", expr.expression);
  }

  @Override
  public String visitLiteralExpr(Expr.Literal expr) {
    if (expr.value instanceof String) {
      String escaped = ((String) expr.value).replace("\"", "\\\"");
      return "\"" + escaped + "\"";
    }

    return expr.value.toString();
  }
//>= Control Flow

  @Override
  public String visitLogicalExpr(Expr.Logical expr) {
    return join(expr.operator, expr.left, expr.right);
  }
//>= Classes

  @Override
  public String visitSetExpr(Expr.Set expr) {
    return join("=", expr.object, expr.name.text, expr.value);
  }
//>= Inheritance

  @Override
  public String visitSuperExpr(Expr.Super expr) {
    return join("super", expr.method);
  }
//>= Classes

  @Override
  public String visitThisExpr(Expr.This expr) {
    return "this";
  }
//>= Syntax Trees

  @Override
  public String visitUnaryExpr(Expr.Unary expr) {
    return join(expr.operator, expr.right);
  }
//>= Variables

  @Override
  public String visitVariableExpr(Expr.Variable expr) {
    return expr.name.text;
  }

  @Override
  public String visitBlockStmt(Stmt.Block stmt) {
    return join("block", stmt.statements);
  }
//>= Classes

  @Override
  public String visitClassStmt(Stmt.Class stmt) {
    StringBuilder builder = new StringBuilder();
    builder.append("(class " + stmt.name.text);
//>= Inheritance

    if (stmt.superclass != null) {
      builder.append(" < " + print(stmt.superclass));
    }
//>= Classes

    for (Stmt.Function method : stmt.methods) {
      builder.append(" " + print(method));
    }

    builder.append(")");
    return builder.toString();
  }
//>= Variables

  @Override
  public String visitExpressionStmt(Stmt.Expression stmt) {
    return join(";", stmt.expression);
  }
//>= Uhh

  @Override
  public String visitForStmt(Stmt.For stmt) {
    return null;
  }
//>= Functions

  @Override
  public String visitFunctionStmt(Stmt.Function stmt) {
    StringBuilder builder = new StringBuilder();
    builder.append("(fun " + stmt.name.text + "(");

    for (Token param : stmt.parameters) {
      if (param != stmt.parameters.get(0)) builder.append(" ");
      builder.append(param.text);
    }

    builder.append(") " + print(stmt.body) + ")");
    return builder.toString();
  }
//>= Control Flow

  @Override
  public String visitIfStmt(Stmt.If stmt) {
    if (stmt.elseBranch == null) {
      return join("if", stmt.condition, "then", stmt.thenBranch);
    }

    return join("if", stmt.condition, "then", stmt.thenBranch,
        "else", stmt.elseBranch);
  }
//>= Functions

  @Override
  public String visitReturnStmt(Stmt.Return stmt) {
    if (stmt.value == null) return "(return)";
    return join("return", stmt.value);
  }
//>= Variables

  @Override
  public String visitVarStmt(Stmt.Var stmt) {
    if (stmt.initializer == null) {
      return join("var", stmt.name.text);
    }

    return join("var", stmt.name.text, "=", stmt.initializer);
  }
//>= Control Flow

  @Override
  public String visitWhileStmt(Stmt.While stmt) {
    return join("while", stmt.condition, stmt.body);
  }
//>= Syntax Trees

  private String join(Object... parts) {
    StringBuilder builder = new StringBuilder();

    builder.append("(");
    addParts(builder, Arrays.asList(parts));
    builder.append(")");
    return builder.toString();
  }

  private void addParts(StringBuilder builder, Iterable parts) {
    for (Object part : parts) {
      if (builder.length() > 1 && !builder.toString().endsWith(" ")) {
        builder.append(" ");
      }

      if (part instanceof Expr) {
        builder.append(print((Expr)part));
//>= Variables
      } else if (part instanceof Stmt) {
        builder.append(print((Stmt) part));
//>= Syntax Trees
      } else if (part instanceof Token) {
        builder.append(((Token) part).text);
      } else if (part instanceof Iterable) {
        addParts(builder, (Iterable)part);
      } else {
        builder.append(part);
      }
    }
  }
/*== Syntax Trees

  public static void main(String[] args) {
    Expr expression = new Expr.Binary(
        new Expr.Unary(
            new Token(TokenType.MINUS, "-", null, 1),
            new Expr.Literal(123)),
        new Token(TokenType.STAR, "*", null, 1),
        new Expr.Grouping(
            new Expr.Literal("str")));

    System.out.println(new AstPrinter().print(expression));
  }
*/
//>= Syntax Trees
}
