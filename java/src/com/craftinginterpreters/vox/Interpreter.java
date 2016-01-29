package com.craftinginterpreters.vox;

import java.util.ArrayList;
import java.util.List;

// Creates an unambiguous, if ugly, string representation of AST nodes.
class Interpreter implements Stmt.Visitor<Void, Void>, Expr.Visitor<Object, Void> {
  private final ErrorReporter errorReporter;
  // TODO: Make this private and expose methods to change?
  Variables variables;

  Interpreter(ErrorReporter errorReporter) {
    this.errorReporter = errorReporter;
    variables = new Variables(null, "print", (Callable)Primitives::print);
  }

  void run(String source) {
    Lexer lexer = new Lexer(source);
    Parser parser = new Parser(lexer, errorReporter);
    List<Stmt> statements = parser.parseProgram();

    // Don't run if there was a parse error.
    if (errorReporter.hadError) return;

    for (Stmt statement : statements) {
      execute(statement);
    }
  }

  private Object evaluate(Expr expr, Void context) {
    return expr.accept(this, context);
  }

  void execute(Stmt stmt) {
    stmt.accept(this, null);
  }

  // Executes [stmt] in its own block scope.
  private void executeInScope(Stmt stmt) {
    Variables before = variables;
    try {
      execute(stmt);
    } finally {
      variables = before;
    }
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
    // TODO: Show what happens if don't define before creating fn.
    variables = variables.define(stmt.name, null);
    VoxFunction function = new VoxFunction(stmt, variables);
    variables.assign(stmt.name, function);
    return null;
  }

  @Override
  public Void visitIfStmt(Stmt.If stmt, Void context) {
    if (isTrue(evaluate(stmt.condition, context))) {
      executeInScope(stmt.thenBranch);
    } else if (stmt.elseBranch != null) {
      executeInScope(stmt.elseBranch);
    }
    return null;
  }

  @Override
  public Void visitReturnStmt(Stmt.Return stmt, Void context) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value, context);

    throw new Return(value);
  }

  @Override
  public Void visitVarStmt(Stmt.Var stmt, Void context) {
    Object value = evaluate(stmt.initializer, context);
    variables = variables.define(stmt.name, value);
    return null;
  }

  @Override
  public Void visitWhileStmt(Stmt.While stmt, Void context) {
    while (isTrue(evaluate(stmt.condition, context))) {
      executeInScope(stmt.body);
    }
    return null;
  }

  @Override
  public Object visitAssignExpr(Expr.Assign expr, Void context) {
    Object value = evaluate(expr.value, context);
    // TODO: Handle property assignment.
    variables.assign(expr.name, value);
    return value;
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
      return ((Callable)callable).call(this, arguments);
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
}
