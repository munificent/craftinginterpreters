package com.craftinginterpreters.vox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

// Tree-walk interpreter.
class Interpreter implements Stmt.Visitor<Void, Environment>,
    Expr.Visitor<Object, Environment> {
  private final ErrorReporter errorReporter;

  // The top level global variables.
  private final Environment globals = new Environment(null);

  private final VoxClass objectClass;
  private final VoxClass classClass;
  private final VoxClass functionClass;

  Interpreter(ErrorReporter errorReporter) {
    this.errorReporter = errorReporter;

    // TODO: Methods.
    objectClass = new VoxClass("Object", null, null, new HashMap<>());
    classClass = new VoxClass("Class", objectClass, null, new HashMap<>());
    functionClass = new VoxClass("Function", objectClass, null, new HashMap<>());
    objectClass.setClass(classClass);
    classClass.setClass(classClass);
    functionClass.setClass(classClass);

    globals.assign("print", Callable.wrap(Primitives::print));
    globals.assign("clock", Callable.wrap(Primitives::clock));
    // TODO: What happens if a user tries to directly construct Class or
    // Function objects?
  }

  void run(String source) {
    Scanner scanner = new Scanner(source);
    Parser parser = new Parser(scanner, errorReporter);
    List<Stmt> statements = parser.parseProgram();

    // Don't run if there was a parse error.
    if (errorReporter.hadError) return;

    for (Stmt statement : statements) {
      execute(statement, globals);
    }
  }

  private Object evaluate(Expr expr, Environment env) {
    return expr.accept(this, env);
  }

  void execute(Stmt stmt, Environment env) {
    stmt.accept(this, env);
  }

  @Override
  public Void visitBlockStmt(Stmt.Block stmt, Environment env) {
    env = new Environment(env);
    for (Stmt statement : stmt.statements) {
      execute(statement, env);
    }
    return null;
  }

  @Override
  public Void visitClassStmt(Stmt.Class stmt, Environment env) {
    Map<String, VoxFunction> methods = new HashMap<>();

    Object superclass = objectClass;
    if (stmt.superclass != null) {
      superclass = evaluate(stmt.superclass, env);
      if (!(superclass instanceof VoxClass)) {
        throw new RuntimeError(Primitives.stringify(superclass) + " is not a class.",
            stmt.name);
      }
    }

    VoxFunction constructor = null;
    for (Stmt.Function method : stmt.methods) {
      VoxFunction function = new VoxFunction(method, env);
      function.setClass(functionClass);

      if (method.name.text.equals(stmt.name.text)) {
        constructor = function;
      } else {
        methods.put(method.name.text, function);
      }
    }

    VoxClass voxClass = new VoxClass(stmt.name.text, (VoxClass)superclass, constructor, methods);
    voxClass.setClass(classClass);
    env.define(stmt.name, voxClass);
    return null;
  }

  @Override
  public Void visitExpressionStmt(Stmt.Expression stmt, Environment env) {
    evaluate(stmt.expression, env);
    return null;
  }

  @Override
  public Void visitForStmt(Stmt.For stmt, Environment env) {
    return null;
  }

  @Override
  public Void visitFunctionStmt(Stmt.Function stmt, Environment env) {
    VoxFunction function = new VoxFunction(stmt, env);
    function.setClass(functionClass);
    env.define(stmt.name, function);
    return null;
  }

  @Override
  public Void visitIfStmt(Stmt.If stmt, Environment env) {
    env = new Environment(env);
    if (Primitives.isTrue(evaluate(stmt.condition, env))) {
      execute(stmt.thenBranch, env);
    } else if (stmt.elseBranch != null) {
      execute(stmt.elseBranch, env);
    }
    return null;
  }

  @Override
  public Void visitReturnStmt(Stmt.Return stmt, Environment env) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value, env);

    throw new Return(value);
  }

  @Override
  public Void visitVarStmt(Stmt.Var stmt, Environment env) {
    Object value = evaluate(stmt.initializer, env);
    env.define(stmt.name, value);
    return null;
  }

  @Override
  public Void visitWhileStmt(Stmt.While stmt, Environment env) {
    env = new Environment(env);
    while (Primitives.isTrue(evaluate(stmt.condition, env))) {
      execute(stmt.body, env);
    }
    return null;
  }

  @Override
  public Object visitAssignExpr(Expr.Assign expr, Environment env) {
    Object value = evaluate(expr.value, env);

    if (expr.object != null) {
      Object object = evaluate(expr.object, env);
      if (object instanceof VoxObject) {
        ((VoxObject)object).properties.put(expr.name.text, value);
      } else {
        throw new RuntimeError("Cannot add properties to primitive values.",
            expr.name);
      }
    } else {
      env.find(expr.name).assign(expr.name.text, value);
    }

    return value;
  }

  @Override
  public Object visitBinaryExpr(Expr.Binary expr, Environment env) {
    Object left = evaluate(expr.left, env);
    Object right = evaluate(expr.right, env);

    // TODO: Type check arithmetic operators.
    switch (expr.operator.type) {
      case BANG_EQUAL: return !Primitives.isEqual(left, right);
      case EQUAL_EQUAL: return Primitives.isEqual(left, right);
      case GREATER: return (double)left > (double)right;
      case GREATER_EQUAL: return (double)left >= (double)right;
      case LESS: return (double)left < (double)right;
      case LESS_EQUAL: return (double)left <= (double)right;
      case MINUS: return (double)left - (double)right;
      case PERCENT: return (double)left % (double)right;
      case PLUS:
        if (left instanceof Double && right instanceof Double) {
          return (double)left + (double)right;
        }

        if (left instanceof String || right instanceof String) {
          return Primitives.stringify(left) + Primitives.stringify(right);
        }

        throw new RuntimeError("Cannot add " + left + " and " + right + ".",
            expr.operator);

      case SLASH: return (double)left / (double)right;
      case STAR: return (double)left * (double)right;
    }

    // Unreachable.
    return null;
  }

  @Override
  public Object visitCallExpr(Expr.Call expr, Environment env) {
    Object callable = evaluate(expr.callee, env);

    List<Object> arguments = new ArrayList<>();
    for (Expr argument : expr.arguments) {
      arguments.add(evaluate(argument, env));
    }

    if (!(callable instanceof Callable)) {
      throw new RuntimeError(
          Primitives.represent(callable) + " cannot be called.", expr.paren);
    }

    Callable function = (Callable)callable;
    if (arguments.size() < function.requiredArguments()) {
      throw new RuntimeError("Not enough arguments.", expr.paren);
    }

    return function.call(this, arguments);
  }

  @Override
  public Object visitGroupingExpr(Expr.Grouping expr, Environment env) {
    return evaluate(expr.expression, env);
  }

  @Override
  public Object visitLiteralExpr(Expr.Literal expr, Environment env) {
    return expr.value;
  }

  @Override
  public Object visitLogicalExpr(Expr.Logical expr, Environment env) {
    Object left = evaluate(expr.left, env);

    if (expr.operator.type == TokenType.OR && Primitives.isTrue(left)) return left;
    if (expr.operator.type == TokenType.AND && !Primitives.isTrue(left)) return left;

    return evaluate(expr.right, env);
  }

  @Override
  public Object visitPropertyExpr(Expr.Property expr, Environment env) {
    Object object = evaluate(expr.object, env);
    if (object instanceof VoxObject) {
      return ((VoxObject)object).getProperty(expr.name);
    }

    throw new RuntimeError(
        "Cannot access properties on primitive values.",
        expr.name);
  }

  @Override
  public Object visitThisExpr(Expr.This expr, Environment env) {
    return env.find(expr.name).get(expr.name.text);
  }

  @Override
  public Object visitUnaryExpr(Expr.Unary expr, Environment env) {
    Object right = evaluate(expr.right, env);

    // TODO: Handle conversions.
    switch (expr.operator.type) {
      case BANG: return !Primitives.isTrue(right);
      case MINUS: return -(double)right;
      case PLUS: return +(double)right;
    }

    // TODO: Test error cases.

    // Unreachable.
    return null;
  }

  @Override
  public Object visitVariableExpr(Expr.Variable expr, Environment env) {
    return env.find(expr.name).get(expr.name.text);
    // TODO: Talk about late binding:
    //
    //   if (false) variableThatIsNotDefined;
    //
    // No error in a late bound language, but error in eager.
  }
}
