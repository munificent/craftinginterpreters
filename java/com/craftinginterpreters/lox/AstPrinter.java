//> Representing Code ast-printer
package com.craftinginterpreters.lox;

// Creates an unambiguous, if ugly, string representation of AST nodes.
/* Representing Code ast-printer < Statements and State not-yet
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
//< Statements and State not-yet
//> visit-methods
//> Statements and State not-yet
  @Override
  public String visitBlockStmt(Stmt.Block stmt) {
    StringBuilder builder = new StringBuilder();
    builder.append("(block ");

    for (Stmt statement : stmt.statements) {
      builder.append(statement.accept(this));
    }

    builder.append(")");
    return builder.toString();
  }
//< Statements and State not-yet
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
//> Statements and State not-yet

  @Override
  public String visitExpressionStmt(Stmt.Expression stmt) {
    return parenthesize(";", stmt.expression);
  }
//< Statements and State not-yet
//> Functions not-yet

  @Override
  public String visitFunctionStmt(Stmt.Function stmt) {
    StringBuilder builder = new StringBuilder();
    builder.append("(fun " + stmt.name.lexeme + "(");

    for (Token param : stmt.parameters) {
      if (param != stmt.parameters.get(0)) builder.append(" ");
      builder.append(param.lexeme);
    }

    builder.append(") ");

    for (Stmt body : stmt.body) {
      builder.append(body.accept(this));
    }

    builder.append(")");
    return builder.toString();
  }
//< Functions not-yet
//> Control Flow not-yet

  @Override
  public String visitIfStmt(Stmt.If stmt) {
    if (stmt.elseBranch == null) {
      return parenthesize2("if", stmt.condition, stmt.thenBranch);
    }

    return parenthesize2("if-else", stmt.condition, stmt.thenBranch,
        stmt.elseBranch);
  }
//< Control Flow not-yet
//> Statements and State not-yet

  @Override
  public String visitPrintStmt(Stmt.Print stmt) {
    return parenthesize("print", stmt.expression);
  }
//< Statements and State not-yet
//> Functions not-yet

  @Override
  public String visitReturnStmt(Stmt.Return stmt) {
    if (stmt.value == null) return "(return)";
    return parenthesize("return", stmt.value);
  }
//< Functions not-yet
//> Statements and State not-yet

  @Override
  public String visitVarStmt(Stmt.Var stmt) {
    if (stmt.initializer == null) {
      return parenthesize2("var", stmt.name);
    }

    return parenthesize2("var", stmt.name, "=", stmt.initializer);
  }
//< Statements and State not-yet
//> Control Flow not-yet

  @Override
  public String visitWhileStmt(Stmt.While stmt) {
    return parenthesize2("while", stmt.condition, stmt.body);
  }
//< Control Flow not-yet
//> Statements and State not-yet

  @Override
  public String visitAssignExpr(Expr.Assign expr) {
    return parenthesize2("=", expr.name.lexeme, expr.value);
  }
//< Statements and State not-yet

  @Override
  public String visitBinaryExpr(Expr.Binary expr) {
    return parenthesize(expr.operator.lexeme, expr.left, expr.right);
  }
//> Functions not-yet

  @Override
  public String visitCallExpr(Expr.Call expr) {
    return parenthesize2("call", expr.callee, expr.arguments);
  }
//< Functions not-yet
//> Classes not-yet

  @Override
  public String visitGetExpr(Expr.Get expr) {
    return parenthesize2(".", expr.object, expr.name.lexeme);
  }
//< Classes not-yet

  @Override
  public String visitGroupingExpr(Expr.Grouping expr) {
    return parenthesize("group", expr.expression);
  }

  @Override
  public String visitLiteralExpr(Expr.Literal expr) {
    return expr.value.toString();
  }
//> Control Flow not-yet

  @Override
  public String visitLogicalExpr(Expr.Logical expr) {
    return parenthesize(expr.operator.lexeme, expr.left, expr.right);
  }
//< Control Flow not-yet
//> Classes not-yet

  @Override
  public String visitSetExpr(Expr.Set expr) {
    return parenthesize2("=", expr.object, expr.name.lexeme, expr.value);
  }
//< Classes not-yet
//> Inheritance not-yet

  @Override
  public String visitSuperExpr(Expr.Super expr) {
    return parenthesize2("super", expr.method);
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
    return parenthesize(expr.operator.lexeme, expr.right);
  }
//> Statements and State not-yet

  @Override
  public String visitVariableExpr(Expr.Variable expr) {
    return expr.name.lexeme;
  }
//< Statements and State not-yet
//< visit-methods
//> print-utilities
  private String parenthesize(String name, Expr... exprs) {
    StringBuilder builder = new StringBuilder();

    builder.append("(").append(name);
    for (Expr expr : exprs) {
      builder.append(" ");
      builder.append(expr.accept(this));
    }
    builder.append(")");

    return builder.toString();
  }
//< print-utilities
//> omit
  // Note: AstPrinting other types syntax trees is now shown in the
  // book, but this is provided here as a reference for those reading
  // the full code.
  private String parenthesize2(String name, Object... parts) {
    StringBuilder builder = new StringBuilder();

    builder.append("(").append(name);

    for (Object part : parts) {
      builder.append(" ");

      if (part instanceof Expr) {
        builder.append(((Expr)part).accept(this));
//> Statements and State not-yet
      } else if (part instanceof Stmt) {
        builder.append(((Stmt) part).accept(this));
//< Statements and State not-yet
      } else if (part instanceof Token) {
        builder.append(((Token) part).lexeme);
      } else {
        builder.append(part);
      }
    }
    builder.append(")");

    return builder.toString();
  }
//< omit
/* Representing Code printer-main < Parsing Expressions not-yet

  public static void main(String[] args) {
    Expr expression = new Expr.Binary(
        new Expr.Unary(
            new Token(TokenType.MINUS, "-", null, 1),
            new Expr.Literal(123)),
        new Token(TokenType.STAR, "*", null, 1),
        new Expr.Grouping(
            new Expr.Literal(45.67)));

    System.out.println(new AstPrinter().print(expression));
  }
*/
}
