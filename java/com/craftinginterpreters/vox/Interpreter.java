package com.craftinginterpreters.vox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

// Tree-walk interpreter.
class Interpreter implements Stmt.Visitor<Environment, Environment>,
    Expr.Visitor<Object, Environment> {
  private final ErrorReporter errorReporter;

  // The top level global variables.
  private final Environment globals = new GlobalEnvironment();

  private final VoxClass objectClass;

  Interpreter(ErrorReporter errorReporter) {
    this.errorReporter = errorReporter;

    // TODO: Methods.
    objectClass = new VoxClass("Object", null, null, new HashMap<>());

    globals.define("print", Callable.wrap(Primitives::print));
    globals.define("clock", Callable.wrap(Primitives::clock));
  }

  void run(String source) {
    Scanner scanner = new Scanner(source);
    Parser parser = new Parser(scanner, errorReporter);
    List<Stmt> statements = parser.parseProgram();

    if (!errorReporter.hadError) {
      Resolver resolver = new Resolver(errorReporter);
      resolver.resolve(statements);
    }

    // Don't run if there was a parse error.
    if (errorReporter.hadError) return;

    for (Stmt statement : statements) {
      execute(statement, globals);
    }
  }

  private Object evaluate(Expr expr, Environment environment) {
    return expr.accept(this, environment);
  }

  Environment execute(Stmt stmt, Environment environment) {
    return stmt.accept(this, environment);
  }

  @Override
  public Environment visitBlockStmt(Stmt.Block stmt, Environment environment) {
    Environment before = environment;
    environment = environment.enterScope();
    for (Stmt statement : stmt.statements) {
      environment = execute(statement, environment);
    }
    return before;
  }

  @Override
  public Environment visitClassStmt(Stmt.Class stmt, Environment environment) {
    environment = environment.declare(stmt.name);

    Map<String, VoxFunction> methods = new HashMap<>();
    Object superclass = objectClass;
    if (stmt.superclass != null) {
      superclass = evaluate(stmt.superclass, environment);
      if (!(superclass instanceof VoxClass)) {
        throw new RuntimeError(Primitives.stringify(superclass) + " is not a class.",
            stmt.name);
      }
    }

    VoxFunction constructor = null;
    for (Stmt.Function method : stmt.methods) {
      VoxFunction function = new VoxFunction(method, environment);

      if (method.name.text.equals(stmt.name.text)) {
        constructor = function;
      } else {
        methods.put(method.name.text, function);
      }
    }

    VoxClass voxClass = new VoxClass(stmt.name.text,
        (VoxClass)superclass, constructor, methods);

    environment.set(stmt.name, voxClass);
    return environment;
  }

  @Override
  public Environment visitExpressionStmt(Stmt.Expression stmt, Environment environment) {
    evaluate(stmt.expression, environment);
    return environment;
  }

  @Override
  public Environment visitForStmt(Stmt.For stmt, Environment environment) {
    return environment;
  }

  @Override
  public Environment visitFunctionStmt(Stmt.Function stmt, Environment environment) {
    environment = environment.declare(stmt.name);
    VoxFunction function = new VoxFunction(stmt, environment);
    environment.set(stmt.name, function);
    return environment;
  }

  @Override
  public Environment visitIfStmt(Stmt.If stmt, Environment environment) {
    Environment before = environment;
    environment = environment.enterScope();

    if (Primitives.isTrue(evaluate(stmt.condition, environment))) {
      execute(stmt.thenBranch, environment);
    } else if (stmt.elseBranch != null) {
      execute(stmt.elseBranch, environment);
    }

    return before;
  }

  @Override
  public Environment visitReturnStmt(Stmt.Return stmt, Environment environment) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value, environment);

    throw new Return(value);
  }

  @Override
  public Environment visitVarStmt(Stmt.Var stmt, Environment environment) {
    Object value = evaluate(stmt.initializer, environment);
    return environment.define(stmt.name.text, value);
  }

  @Override
  public Environment visitWhileStmt(Stmt.While stmt, Environment environment) {
    Environment before = environment;
    environment = environment.enterScope();

    while (Primitives.isTrue(evaluate(stmt.condition, environment))) {
      execute(stmt.body, environment);
    }

    return before;
  }

  @Override
  public Object visitAssignExpr(Expr.Assign expr, Environment environment) {
    Object value = evaluate(expr.value, environment);

    if (expr.object != null) {
      Object object = evaluate(expr.object, environment);
      if (object instanceof VoxObject) {
        ((VoxObject)object).fields.put(expr.name.text, value);
      } else {
        throw new RuntimeError("Only instances have fields.",
            expr.name);
      }
    } else {
      environment.set(expr.name, value);
    }

    return value;
  }

  @Override
  public Object visitBinaryExpr(Expr.Binary expr, Environment environment) {
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
  public Object visitFieldExpr(Expr.Field expr, Environment environment) {
    Object object = evaluate(expr.object, environment);
    if (object instanceof VoxObject) {
      return ((VoxObject)object).getField(expr.name);
    }

    throw new RuntimeError("Only instances have fields.",
        expr.name);
  }

  @Override
  public Object visitGroupingExpr(Expr.Grouping expr, Environment environment) {
    return evaluate(expr.expression, environment);
  }

  @Override
  public Object visitLiteralExpr(Expr.Literal expr, Environment environment) {
    return expr.value;
  }

  @Override
  public Object visitLogicalExpr(Expr.Logical expr, Environment environment) {
    Object left = evaluate(expr.left, environment);

    if (expr.operator.type == TokenType.OR && Primitives.isTrue(left)) return left;
    if (expr.operator.type == TokenType.AND && !Primitives.isTrue(left)) return left;

    return evaluate(expr.right, environment);
  }

  @Override
  public Object visitThisExpr(Expr.This expr, Environment environment) {
    return environment.get(expr.name);
  }

  @Override
  public Object visitUnaryExpr(Expr.Unary expr, Environment environment) {
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
  public Object visitVariableExpr(Expr.Variable expr, Environment environment) {
    return environment.get(expr.name);
    // TODO: Talk about late binding:
    //
    //   if (false) variableThatIsNotDefined;
    //
    // No error in a late bound language, but error in eager.
  }

  private void checkNumberOperands(Token operator, Object left, Object right) {
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
