package com.craftinginterpreters.vox;

// Creates an unambiguous, if ugly, string representation of AST nodes.
class AstPrinter implements Stmt.Visitor<String, Void>, Expr.Visitor<String, Void> {
  String print(Expr expr) {
    return expr.accept(this, null);
  }

  String print(Stmt stmt) {
    return stmt.accept(this, null);
  }

  @Override
  public String visitBlockStmt(Stmt.Block stmt, Void context) {
    StringBuilder builder = new StringBuilder();
    builder.append("(block");

    for (Stmt inner : stmt.statements) {
      builder.append(" " + print(inner));
    }

    builder.append(")");
    return builder.toString();
  }

  @Override
  public String visitClassStmt(Stmt.Class stmt, Void context) {
    StringBuilder builder = new StringBuilder();
    builder.append("(class " + stmt.name);

    if (stmt.superclass != null) {
      builder.append(" < " + print(stmt.superclass));
    }

    for (Stmt.Function method : stmt.methods) {
      builder.append(" " + print(method));
    }

    builder.append(")");
    return builder.toString();
  }

  @Override
  public String visitExpressionStmt(Stmt.Expression stmt, Void context) {
    return join("(; ", stmt.expression, ")");
  }

  @Override
  public String visitForStmt(Stmt.For stmt, Void context) {
    return null;
  }

  @Override
  public String visitFunctionStmt(Stmt.Function stmt, Void context) {
    return join("(fun (", String.join(" ", stmt.parameters), ") ", stmt.body, ")");
  }

  @Override
  public String visitIfStmt(Stmt.If stmt, Void context) {
    String result = join("(if ", stmt.condition, " then ", stmt.thenBranch);
    if (stmt.elseBranch != null) {
      result += join(" else ", stmt.elseBranch);
    }

    return result + ")";
  }

  @Override
  public String visitReturnStmt(Stmt.Return stmt, Void context) {
    if (stmt.value == null) return "(return)";
    return join("(return ", stmt.value, ")");
  }

  @Override
  public String visitVarStmt(Stmt.Var stmt, Void context) {
    return join("(var ", stmt.name, " = ", stmt.initializer, ")");
  }

  @Override
  public String visitWhileStmt(Stmt.While stmt, Void context) {
    return join("(while ", stmt.condition, " ", stmt.body, ")");
  }

  @Override
  public String visitAssignExpr(Expr.Assign expr, Void context) {
    return join("(= ", expr.target, " ", expr.value, ")");
  }

  @Override
  public String visitBinaryExpr(Expr.Binary expr, Void context) {
    return join("(", expr.operator, " ", expr.left, " ", expr.right, ")");
  }

  @Override
  public String visitCallExpr(Expr.Call expr, Void context) {
    StringBuilder builder = new StringBuilder();
    builder.append("(call " + print(expr.target));

    for (Expr arg : expr.arguments) {
      builder.append(" " + print(arg));
    }

    builder.append(")");
    return builder.toString();
  }

  @Override
  public String visitGroupingExpr(Expr.Grouping expr, Void context) {
    return join("(group ", expr.expression, ")");
  }

  @Override
  public String visitLiteralExpr(Expr.Literal expr, Void context) {
    if (expr.value instanceof String) {
      return "\"" + ((String) expr.value).replace("\"", "\\\"") + "\"";
    }

    return expr.value.toString();
  }

  @Override
  public String visitLogicalExpr(Expr.Logical expr, Void context) {
    return join("(", expr.operator, " ", expr.left, " ", expr.right, ")");
  }

  @Override
  public String visitPropertyExpr(Expr.Property expr, Void context) {
    return join("(.", expr.object, " ", expr.name, ")");
  }

  @Override
  public String visitUnaryExpr(Expr.Unary expr, Void context) {
    return join("(", expr.operator, " ", expr.right, ")");
  }

  @Override
  public String visitVariableExpr(Expr.Variable expr, Void context) {
    return expr.name;
  }

  private String join(Object... parts) {
    StringBuilder builder = new StringBuilder();

    for (Object part : parts) {
      if (part instanceof Stmt) {
        builder.append(print((Stmt) part));
      } else if (part instanceof Expr) {
        builder.append(print((Expr)part));
      } else if (part instanceof Token) {
        builder.append(((Token)part).text);
      } else {
        builder.append(part);
      }
    }

    return builder.toString();
  }
}
