package com.craftinginterpreters.vox;

import java.util.*;

// Tree-walk interpreter.
class Interpreter implements Stmt.Visitor<Void, Environment>,
    Expr.Visitor<Object, Environment> {
  private final ErrorReporter errorReporter;

  // The top level global variables.
  private final Environment globals = new Environment(null);

  private Map<Expr, Integer> locals;

  Interpreter(ErrorReporter errorReporter) {
    this.errorReporter = errorReporter;

    globals.define("print", Callable.wrap(Primitives::print));
    globals.define("clock", Callable.wrap(Primitives::clock));
  }

  void run(String source) {
    errorReporter.hadError = false;
    Scanner scanner = new Scanner(source, errorReporter);
    List<Token> tokens = scanner.scanTokens();
    Parser parser = new Parser(tokens, errorReporter);
    List<Stmt> statements = parser.parseProgram();

    if (!errorReporter.hadError) {
      Resolver resolver = new Resolver(errorReporter);
      locals = resolver.resolve(statements);
    }

    // Don't run if there was a parse or resolution error.
    if (errorReporter.hadError) return;

    for (Stmt statement : statements) {
      execute(statement, globals);
    }
  }

  private Object evaluate(Expr expr, Environment environment) {
    return expr.accept(this, environment);
  }

  void execute(Stmt stmt, Environment environment) {
    stmt.accept(this, environment);
  }

  @Override
  public Void visitBlockStmt(Stmt.Block stmt,
                                    Environment environment) {
    environment = environment.beginScope();
    for (Stmt statement : stmt.statements) {
      execute(statement, environment);
    }
    return null;
  }

  @Override
  public Void visitClassStmt(Stmt.Class stmt,
                                    Environment environment) {
    environment.declare(stmt.name);

    Map<String, VoxFunction> methods = new HashMap<>();
    Object superclass = null;
    if (stmt.superclass != null) {
      superclass = evaluate(stmt.superclass, environment);
      if (!(superclass instanceof VoxClass)) {
        throw new RuntimeError("Superclass must be a class.", stmt.name);
      }
    }

    for (Stmt.Function method : stmt.methods) {
      VoxFunction function = new VoxFunction(method, environment,
          method.name.text.equals("init"));
        methods.put(method.name.text, function);
    }

    VoxClass klass = new VoxClass(stmt.name.text,
        (VoxClass)superclass, methods);

    environment.set(stmt.name, klass);
    return null;
  }

  @Override
  public Void visitExpressionStmt(Stmt.Expression stmt,
                                  Environment environment) {
    evaluate(stmt.expression, environment);
    return null;
  }

  @Override
  public Void visitForStmt(Stmt.For stmt,
                           Environment environment) {
    return null;
  }

  @Override
  public Void visitFunctionStmt(Stmt.Function stmt,
                                Environment environment) {
    environment.declare(stmt.name);
    VoxFunction function = new VoxFunction(stmt, environment, false);
    environment.set(stmt.name, function);
    return null;
  }

  @Override
  public Void visitIfStmt(Stmt.If stmt,
                          Environment environment) {
    environment = environment.beginScope();

    if (Primitives.isTrue(evaluate(stmt.condition, environment))) {
      execute(stmt.thenBranch, environment);
    } else if (stmt.elseBranch != null) {
      execute(stmt.elseBranch, environment);
    }

    return null;
  }

  @Override
  public Void visitReturnStmt(Stmt.Return stmt,
                              Environment environment) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value, environment);

    throw new Return(value);
  }

  @Override
  public Void visitVarStmt(Stmt.Var stmt,
                           Environment environment) {
    Object value = null;
    if (stmt.initializer != null) {
      value = evaluate(stmt.initializer, environment);
    }
    environment.define(stmt.name.text, value);
    return null;
  }

  @Override
  public Void visitWhileStmt(Stmt.While stmt,
                             Environment environment) {
    environment = environment.beginScope();

    while (Primitives.isTrue(evaluate(stmt.condition, environment))) {
      execute(stmt.body, environment);
    }

    return null;
  }

  @Override
  public Object visitAssignExpr(Expr.Assign expr,
                                Environment environment) {
    Object value = evaluate(expr.value, environment);

    if (expr.object != null) {
      Object object = evaluate(expr.object, environment);
      if (object instanceof VoxInstance) {
        ((VoxInstance)object).fields.put(expr.name.text, value);
      } else {
        throw new RuntimeError("Only instances have fields.",
            expr.name);
      }
    } else {
      Integer distance = locals.get(expr);
      if (distance != null) {
        environment.setAt(distance, expr.name, value);
      } else {
        globals.set(expr.name, value);
      }
    }

    return value;
  }

  @Override
  public Object visitBinaryExpr(Expr.Binary expr,
                                Environment environment) {
    Object left = evaluate(expr.left, environment);
    Object right = evaluate(expr.right, environment);

    switch (expr.operator.type) {
      case BANG_EQUAL: return !Primitives.isEqual(left, right);
      case EQUAL_EQUAL: return Primitives.isEqual(left, right);

      case GREATER:
        checkNumberOperands(expr.operator, left, right);
        return (double)left > (double)right;

      case GREATER_EQUAL:
        checkNumberOperands(expr.operator, left, right);
        return (double)left >= (double)right;
      case LESS:
        checkNumberOperands(expr.operator, left, right);
        return (double)left < (double)right;

      case LESS_EQUAL:
        checkNumberOperands(expr.operator, left, right);
        return (double)left <= (double)right;

      case MINUS:
        checkNumberOperands(expr.operator, left, right);
        return (double)left - (double)right;

      case PLUS:
        if (left instanceof Double && right instanceof Double) {
          return (double)left + (double)right;
        }

        if (left instanceof String || right instanceof String) {
          return Primitives.stringify(left) + Primitives.stringify(right);
        }

        throw new RuntimeError(
            "Operands must be two numbers or two strings.",
            expr.operator);

      case SLASH:
        checkNumberOperands(expr.operator, left, right);
        return (double)left / (double)right;

      case STAR:
        checkNumberOperands(expr.operator, left, right);
        return (double)left * (double)right;
    }

    // Unreachable.
    return null;
  }

  @Override
  public Object visitCallExpr(Expr.Call expr, Environment environment) {
    Object callee = evaluate(expr.callee, environment);

    List<Object> arguments = new ArrayList<>();
    for (Expr argument : expr.arguments) {
      arguments.add(evaluate(argument, environment));
    }

    if (!(callee instanceof Callable)) {
      throw new RuntimeError(
          "Can only call functions and classes.", expr.paren);
    }

    Callable function = (Callable)callee;
    if (arguments.size() < function.requiredArguments()) {
      throw new RuntimeError("Not enough arguments.", expr.paren);
    }

    return function.call(this, arguments);
  }

  @Override
  public Object visitGroupingExpr(Expr.Grouping expr,
                                  Environment environment) {
    return evaluate(expr.expression, environment);
  }

  @Override
  public Object visitLiteralExpr(Expr.Literal expr,
                                 Environment environment) {
    return expr.value;
  }

  @Override
  public Object visitLogicalExpr(Expr.Logical expr,
                                 Environment environment) {
    Object left = evaluate(expr.left, environment);

    if (expr.operator.type == TokenType.OR &&
        Primitives.isTrue(left)) {
      return left;
    }

    if (expr.operator.type == TokenType.AND &&
        !Primitives.isTrue(left)) {
      return left;
    }

    return evaluate(expr.right, environment);
  }

  @Override
  public Object visitPropertyExpr(Expr.Property expr,
                                  Environment environment) {
    Object object = evaluate(expr.object, environment);
    if (object instanceof VoxInstance) {
      return ((VoxInstance)object).getProperty(expr.name);
    }

    throw new RuntimeError("Only instances have properties.",
        expr.name);
  }

  @Override
  public Object visitSuperExpr(Expr.Super expr,
                               Environment environment) {
    VoxClass methodClass = (VoxClass)environment.get("class",
        expr.keyword.line);
    VoxClass superclass = methodClass.superclass;

    VoxInstance receiver = (VoxInstance)environment.get("this",
        expr.keyword.line);

    VoxFunction method = superclass.findMethod(receiver,
        expr.method.text);
    if (method == null) {
      throw new RuntimeError("Undefined property '" + expr.method.text +
          "'.", expr.method);
    }

    return method;
  }

  @Override
  public Object visitThisExpr(Expr.This expr, Environment environment) {
    return environment.get(expr.name);
  }

  @Override
  public Object visitUnaryExpr(Expr.Unary expr,
                               Environment environment) {
    Object right = evaluate(expr.right, environment);

    switch (expr.operator.type) {
      case BANG: return !Primitives.isTrue(right);
      case MINUS:
        checkNumberOperand(expr.operator, right);
        return -(double)right;
    }

    // Unreachable.
    return null;
  }

  @Override
  public Object visitVariableExpr(Expr.Variable expr,
                                  Environment environment) {
    Integer distance = locals.get(expr);
    if (distance != null) {
      return environment.getAt(distance, expr.name);
    } else {
      return globals.get(expr.name);
    }
  }

  private void checkNumberOperands(Token operator,
                                   Object left, Object right) {
    if (left instanceof Double && right instanceof Double) {
      return;
    }

    throw new RuntimeError("Operands must be numbers.", operator);
  }

  private void checkNumberOperand(Token operator, Object left) {
    if (left instanceof Double) {
      return;
    }

    throw new RuntimeError("Operand must be a number.", operator);
  }
}
