package com.craftinginterpreters.vox;

import java.util.*;

class Resolver implements Stmt.Visitor<Void, Void>,
    Expr.Visitor<Void, Void> {
  private final ErrorReporter errorReporter;

  private final Stack<Map<String, Boolean>> scopes = new Stack<>();

  Resolver(ErrorReporter errorReporter) {
    this.errorReporter = errorReporter;
  }

  void resolve(List<Stmt> statements) {
    for (Stmt statement : statements) {
      statement.accept(this, null);
    }
  }

  @Override
  public Void visitBlockStmt(Stmt.Block stmt, Void dummy) {
    beginScope();
    resolve(stmt.statements);
    endScope();
    return null;
  }

  @Override
  public Void visitClassStmt(Stmt.Class stmt, Void dummy) {
    declare(stmt.name);
    define(stmt.name);

    if (stmt.superclass != null) {
      resolve(stmt.superclass);
    }

    for (Stmt.Function method : stmt.methods) {
      // TODO: Note that we're in a class?
      resolve(method);
    }

    // TODO: Check for method collisions?
    return null;
  }

  @Override
  public Void visitExpressionStmt(Stmt.Expression stmt, Void dummy) {
    resolve(stmt.expression);
    return null;
  }

  @Override
  public Void visitForStmt(Stmt.For stmt, Void dummy) {
    return null;
  }

  @Override
  public Void visitFunctionStmt(Stmt.Function stmt, Void dummy) {
    declare(stmt.name);
    define(stmt.name);

    beginScope();
    for (Token param : stmt.parameters) {
      declare(param);
      define(param);
    }
    resolve(stmt.body);
    endScope();
    return null;
  }

  @Override
  public Void visitIfStmt(Stmt.If stmt, Void dummy) {
    beginScope();
    resolve(stmt.condition);
    resolve(stmt.thenBranch);
    endScope();

    if (stmt.elseBranch != null) {
      beginScope();
      resolve(stmt.elseBranch);
      endScope();
    }

    return null;
  }

  @Override
  public Void visitReturnStmt(Stmt.Return stmt, Void dummy) {
    if (stmt.value != null) resolve(stmt.value);
    return null;
  }

  @Override
  public Void visitVarStmt(Stmt.Var stmt, Void dummy) {
    declare(stmt.name);
    stmt.initializer.accept(this, null);
    define(stmt.name);
    return null;
  }

  @Override
  public Void visitWhileStmt(Stmt.While stmt, Void dummy) {
    beginScope();
    resolve(stmt.condition);
    resolve(stmt.body);
    endScope();
    return null;
  }

  @Override
  public Void visitAssignExpr(Expr.Assign expr, Void dummy) {
    resolve(expr.value);

    if (expr.object != null) {
      resolve(expr.object);
    }
    // TODO: Mark depth?
//      environment.set(expr.name, value);

    return null;
  }

  @Override
  public Void visitBinaryExpr(Expr.Binary expr, Void dummy) {
    resolve(expr.left);
    resolve(expr.right);
    return null;
  }

  @Override
  public Void visitCallExpr(Expr.Call expr, Void dummy) {
    resolve(expr.callee);

    for (Expr argument : expr.arguments) {
      resolve(argument);
    }

    return null;
  }

  @Override
  public Void visitGroupingExpr(Expr.Grouping expr, Void dummy) {
    resolve(expr.expression);
    return null;
  }

  @Override
  public Void visitLiteralExpr(Expr.Literal expr, Void dummy) {
    return null;
  }

  @Override
  public Void visitLogicalExpr(Expr.Logical expr, Void dummy) {
    resolve(expr.left);
    resolve(expr.right);
    return null;
  }

  @Override
  public Void visitPropertyExpr(Expr.Property expr, Void dummy) {
    resolve(expr.object);
    return null;
  }

  @Override
  public Void visitThisExpr(Expr.This expr, Void dummy) {
    // TODO: Error if outside class?
    return null;
  }

  @Override
  public Void visitUnaryExpr(Expr.Unary expr, Void dummy) {
    resolve(expr.right);
    return null;
  }

  @Override
  public Void visitVariableExpr(Expr.Variable expr, Void dummy) {
    // TODO: Mark name depth?
    for (int i = 0; i < scopes.size(); i++) {
      Map<String, Boolean> scope = scopes.get(i);
      if (scope.containsKey(expr.name.text)) {
        if (!scope.get(expr.name.text)) {
          errorReporter.error(expr.name,
              "A local variable cannot be used in its own initializer.");
        }

        break;
      }
    }

    return null;
  }

  private void resolve(Stmt stmt) {
    stmt.accept(this, null);
  }

  private void resolve(Expr expr) {
    expr.accept(this, null);
  }

  private void beginScope() {
    scopes.push(new HashMap<>());
  }

  private void endScope() {
    scopes.pop();
  }

  private void declare(Token name) {
    // Don't need to track top level variables.
    if (scopes.isEmpty()) return;

    Map<String, Boolean> scope = scopes.peek();

    if (scope.containsKey(name.text)) {
      errorReporter.error(name,
          "Variable with this name already declared in this scope.");
    }

    scope.put(name.text, false);
  }

  private void define(Token name) {
    // Don't need to track top level variables.
    if (scopes.isEmpty()) return;

    scopes.peek().put(name.text, true);
  }
}
