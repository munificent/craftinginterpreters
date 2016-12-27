//> Representing Code not-yet
package com.craftinginterpreters.lox;

import java.util.Arrays;

// Creates an unambiguous, if ugly, string representation of AST nodes.
/* Representing Code not-yet < Statements and State not-yet
class AstPrinter implements Expr.Visitor<String> {
*/
//> Statements and State not-yet
class AstPrinter implements Expr.Visitor<String>, Stmt.Visitor<String> {
//< Statements and State not-yet
  String print(Expr expr) {
    return expr.accept(this);
  }
//> Statements and State not-yet

  String print(Stmt stmt) {
    return stmt.accept(this);
  }

  @Override
  public String visitBlockStmt(Stmt.Block stmt) {
    return join("block", stmt.statements);
  }
//> Classes not-yet

  @Override
  public String visitClassStmt(Stmt.Class stmt) {
    StringBuilder builder = new StringBuilder();
    builder.append("(class " + stmt.name.lexeme);
//> Inheritance not-yet

    if (stmt.superclass != null) {
      builder.append(" < " + print(stmt.superclass));
    }
//< Inheritance not-yet

    for (Stmt.Function method : stmt.methods) {
      builder.append(" " + print(method));
    }

    builder.append(")");
    return builder.toString();
  }
//< Classes not-yet

  @Override
  public String visitExpressionStmt(Stmt.Expression stmt) {
    return join(";", stmt.expression);
  }
//> Functions not-yet

  @Override
  public String visitFunctionStmt(Stmt.Function stmt) {
    StringBuilder builder = new StringBuilder();
    builder.append("(fun " + stmt.name.lexeme + "(");

    for (Token param : stmt.parameters) {
      if (param != stmt.parameters.get(0)) builder.append(" ");
      builder.append(param.lexeme);
    }

    builder.append(") " + join(stmt.body) + ")");
    return builder.toString();
  }
//< Functions not-yet
//> Control Flow not-yet

  @Override
  public String visitIfStmt(Stmt.If stmt) {
    if (stmt.elseBranch == null) {
      return join("if", stmt.condition, "then", stmt.thenBranch);
    }

    return join("if", stmt.condition, "then", stmt.thenBranch,
        "else", stmt.elseBranch);
  }
//< Control Flow not-yet

  @Override
  public String visitPrintStmt(Stmt.Print stmt) {
    return join("print", stmt.expression);
  }
//> Functions not-yet

  @Override
  public String visitReturnStmt(Stmt.Return stmt) {
    if (stmt.value == null) return "(return)";
    return join("return", stmt.value);
  }
//< Functions not-yet

  @Override
  public String visitVarStmt(Stmt.Var stmt) {
    if (stmt.initializer == null) {
      return join("var", stmt.name.lexeme);
    }

    return join("var", stmt.name.lexeme, "=", stmt.initializer);
  }
//> Control Flow not-yet

  @Override
  public String visitWhileStmt(Stmt.While stmt) {
    return join("while", stmt.condition, stmt.body);
  }
//< Control Flow not-yet

  @Override
  public String visitAssignExpr(Expr.Assign expr) {
    return join("=", expr.name.lexeme, expr.value);
  }
//< Statements and State not-yet

  @Override
  public String visitBinaryExpr(Expr.Binary expr) {
    return join(expr.operator, expr.left, expr.right);
  }
//> Functions not-yet

  @Override
  public String visitCallExpr(Expr.Call expr) {
    return join("call", expr.callee, expr.arguments);
  }
//< Functions not-yet
//> Classes not-yet

  @Override
  public String visitGetExpr(Expr.Get expr) {
    return join(".", expr.object, expr.name.lexeme);
  }
//< Classes not-yet

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
//> Control Flow not-yet

  @Override
  public String visitLogicalExpr(Expr.Logical expr) {
    return join(expr.operator, expr.left, expr.right);
  }
//< Control Flow not-yet
//> Classes not-yet

  @Override
  public String visitSetExpr(Expr.Set expr) {
    return join("=", expr.object, expr.name.lexeme, expr.value);
  }
//< Classes not-yet
//> Inheritance not-yet

  @Override
  public String visitSuperExpr(Expr.Super expr) {
    return join("super", expr.method);
  }
//< Inheritance not-yet
//> Classes not-yet

  @Override
  public String visitThisExpr(Expr.This expr) {
    return "this";
  }
//< Classes not-yet

  @Override
  public String visitUnaryExpr(Expr.Unary expr) {
    return join(expr.operator, expr.right);
  }
//> Statements and State not-yet

  @Override
  public String visitVariableExpr(Expr.Variable expr) {
    return expr.name.lexeme;
  }
//< Statements and State not-yet

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
//> Statements and State not-yet
      } else if (part instanceof Stmt) {
        builder.append(print((Stmt) part));
//< Statements and State not-yet
      } else if (part instanceof Token) {
        builder.append(((Token) part).lexeme);
      } else if (part instanceof Iterable) {
        addParts(builder, (Iterable)part);
      } else {
        builder.append(part);
      }
    }
  }
/* Representing Code not-yet < Parsing Expressions not-yet

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
}
