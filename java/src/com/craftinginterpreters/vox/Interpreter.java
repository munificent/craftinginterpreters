package com.craftinginterpreters.vox;

import java.util.ArrayList;
import java.util.List;

// Creates an unambiguous, if ugly, string representation of AST nodes.
class Interpreter implements Stmt.Visitor<Void, Void>, Expr.Visitor<Object, Void> {
  private Variables variables;

  Interpreter() {
    variables = new Variables(null, 0, "print", (Callable)this::primitivePrint);
  }

  void run(String source) {
    Lexer lexer = new Lexer(source);
    Parser parser = new Parser(lexer, null);
    List<Stmt> statements = parser.parseProgram();

    for (Stmt statement : statements) {
      execute(statement);
    }
  }

  private Object evaluate(Expr expr, Void context) {
    return expr.accept(this, context);
  }

  private void execute(Stmt stmt) {
    stmt.accept(this, null);
  }

  @Override
  public Void visitBlockStmt(Stmt.Block stmt, Void context) {
    Variables before = variables;

    try {
      for (Stmt statement : stmt.statements) {
        execute(statement);
      }
    } finally {
      variables = before;
    }
    return null;
  }

  @Override
  public Void visitClassStmt(Stmt.Class stmt, Void context) {
    return null;
  }

  @Override
  public Void visitExpressionStmt(Stmt.Expression stmt, Void context) {
    evaluate(stmt.expression, context);
    return null;
  }

  @Override
  public Void visitForStmt(Stmt.For stmt, Void context) {
    return null;
  }

  @Override
  public Void visitFunctionStmt(Stmt.Function stmt, Void context) {
    return null;
  }

  @Override
  public Void visitIfStmt(Stmt.If stmt, Void context) {
    return null;
  }

  @Override
  public Void visitReturnStmt(Stmt.Return stmt, Void context) {
    return null;
  }

  @Override
  public Void visitVarStmt(Stmt.Var stmt, Void context) {
    Object value = evaluate(stmt.initializer, context);

    variables = variables.define(0, stmt.name, value);
    return null;
  }

  @Override
  public Void visitWhileStmt(Stmt.While stmt, Void context) {
    return null;
  }

  @Override
  public Object visitAssignExpr(Expr.Assign expr, Void context) {
    return null;
  }

  @Override
  public Object visitBinaryExpr(Expr.Binary expr, Void context) {
    Object left = evaluate(expr.left, context);
    Object right = evaluate(expr.right, context);

    // TODO: Handle conversions.
    // TODO: String equality.
    switch (expr.operator.type) {
      case BANG_EQUAL: return left != right;
      case EQUAL_EQUAL: return left == right;
      case GREATER: return (double)left > (double)right;
      case GREATER_EQUAL: return (double)left >= (double)right;
      case LESS: return (double)left < (double)right;
      case LESS_EQUAL: return (double)left <= (double)right;
      case MINUS: return (double)left - (double)right;
      case PERCENT: return (double)left % (double)right;
      case PLUS: return (double)left + (double)right;
      case SLASH: return (double)left / (double)right;
      case STAR: return (double)left * (double)right;
    }

    // Unreachable.
    return null;
  }

  @Override
  public Object visitCallExpr(Expr.Call expr, Void context) {
    Object callable = evaluate(expr.callee, context);

    List<Object> arguments = new ArrayList<>();
    for (Expr argument : expr.arguments) {
      arguments.add(evaluate(argument, context));
    }

    if (callable instanceof Callable) {
      return ((Callable)callable).call(arguments);
    }

    // TODO: User-defined functions.
    // TODO: Error.
    return null;
  }

  @Override
  public Object visitGroupingExpr(Expr.Grouping expr, Void context) {
    return evaluate(expr.expression, context);
  }

  @Override
  public Object visitLiteralExpr(Expr.Literal expr, Void context) {
    return expr.value;
  }

  @Override
  public Object visitLogicalExpr(Expr.Logical expr, Void context) {
    Object left = evaluate(expr.left, context);

    if (expr.operator.type == TokenType.OR && isTrue(left)) return left;
    if (expr.operator.type == TokenType.AND && !isTrue(left)) return left;

    return evaluate(expr.right, context);
  }

  @Override
  public Object visitPropertyExpr(Expr.Property expr, Void context) {
    return null;
  }

  @Override
  public Object visitUnaryExpr(Expr.Unary expr, Void context) {
    Object right = evaluate(expr.right, context);

    // TODO: Handle conversions.
    switch (expr.operator.type) {
      case BANG: return !isTrue(right);
      case MINUS: return -(double)right;
      case PLUS: return +(double)right;
    }

    // Unreachable.
    return null;
  }

  @Override
  public Object visitVariableExpr(Expr.Variable expr, Void context) {
    return variables.lookUp(expr.name);
    // TODO: Talk about late binding:
    //
    //   if (false) variableThatIsNotDefined;
    //
    // No error in a late bound language, but error in eager.
  }

  private boolean isTrue(Object object) {
    if (object == null) return false;
    if (object instanceof Boolean) return (boolean)object;
    return true;
  }

  private Object primitivePrint(List<Object> arguments) {
    Object value = arguments.get(0);
    if (value instanceof Double) {
      // TODO: Hack. Work around Java adding ".0" to integer-valued doubles.
      String text = value.toString();
      if (text.endsWith(".0")) text = text.substring(0, text.length() - 2);
      System.out.println(text);
    } else {
      System.out.println(value);
    }

    return value;
  }
}
