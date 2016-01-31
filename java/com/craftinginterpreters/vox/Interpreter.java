package com.craftinginterpreters.vox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

// Creates an unambiguous, if ugly, string representation of AST nodes.
class Interpreter implements Stmt.Visitor<Variables, Variables>, Expr.Visitor<Object, Variables> {
  private final ErrorReporter errorReporter;
  // TODO: Make this a map to top-level definitions are mutually recursive.
  Variables globals;

  private final VoxClass classClass;
  private final VoxClass functionClass;

  Interpreter(ErrorReporter errorReporter) {
    this.errorReporter = errorReporter;
    globals = new Variables(null, "print", Callable.wrap(Primitives::print));
    globals = globals.define("clock", Callable.wrap(Primitives::clock));

    // TODO: Methods.
    VoxClass objectClass = new VoxClass("Object", null, new HashMap<>());
    classClass = new VoxClass("Class", null, new HashMap<>());
    functionClass = new VoxClass("Function", null, new HashMap<>());
    objectClass.setClass(classClass);
    classClass.setClass(classClass);
    functionClass.setClass(classClass);
  }

  void run(String source) {
    Lexer lexer = new Lexer(source);
    Parser parser = new Parser(lexer, errorReporter);
    List<Stmt> statements = parser.parseProgram();

    // Don't run if there was a parse error.
    if (errorReporter.hadError) return;

    for (Stmt statement : statements) {
      globals = execute(statement, globals);
    }
  }

  private Object evaluate(Expr expr, Variables locals) {
    return expr.accept(this, locals);
  }

  Variables execute(Stmt stmt, Variables locals) {
    return stmt.accept(this, locals);
  }

  @Override
  public Variables visitBlockStmt(Stmt.Block stmt, Variables locals) {
    Variables before = locals;
    for (Stmt statement : stmt.statements) {
      locals = execute(statement, locals);
    }
    return before;
  }

  @Override
  public Variables visitClassStmt(Stmt.Class stmt, Variables locals) {
    Map<String, VoxFunction> methods = new HashMap<>();

    // TODO: Superclass.

    // TODO: Show what happens if don't define before creating class.
    locals = locals.define(stmt.name.text, null);

    VoxFunction constructor = null;
    for (Stmt.Function method : stmt.methods) {
      VoxFunction function = new VoxFunction(method, locals);
      function.setClass(functionClass);

      if (method.name.text.equals(stmt.name.text)) {
        constructor = function;
      } else {
        methods.put(method.name.text, function);
      }
    }

    VoxClass voxClass = new VoxClass(stmt.name.text, constructor, methods);
    voxClass.setClass(classClass);
    locals.assign(stmt.name, voxClass);
    return locals;
  }

  @Override
  public Variables visitExpressionStmt(Stmt.Expression stmt, Variables locals) {
    evaluate(stmt.expression, locals);
    return locals;
  }

  @Override
  public Variables visitForStmt(Stmt.For stmt, Variables locals) {
    return locals;
  }

  @Override
  public Variables visitFunctionStmt(Stmt.Function stmt, Variables locals) {
    // TODO: Show what happens if don't define before creating fn.
    locals = locals.define(stmt.name.text, null);
    VoxFunction function = new VoxFunction(stmt, locals);
    function.setClass(functionClass);
    locals.assign(stmt.name, function);
    return locals;
  }

  @Override
  public Variables visitIfStmt(Stmt.If stmt, Variables locals) {
    if (Primitives.isTrue(evaluate(stmt.condition, locals))) {
      execute(stmt.thenBranch, locals);
    } else if (stmt.elseBranch != null) {
      execute(stmt.elseBranch, locals);
    }
    return locals;
  }

  @Override
  public Variables visitReturnStmt(Stmt.Return stmt, Variables locals) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value, locals);

    throw new Return(value);
  }

  @Override
  public Variables visitVarStmt(Stmt.Var stmt, Variables locals) {
    Object value = evaluate(stmt.initializer, locals);
    return locals.define(stmt.name, value);
  }

  @Override
  public Variables visitWhileStmt(Stmt.While stmt, Variables locals) {
    while (Primitives.isTrue(evaluate(stmt.condition, locals))) {
      execute(stmt.body, locals);
    }
    return locals;
  }

  @Override
  public Object visitAssignExpr(Expr.Assign expr, Variables locals) {
    Object value = evaluate(expr.value, locals);

    if (expr.object != null) {
      Object object = evaluate(expr.object, locals);
      if (object instanceof VoxObject) {
        ((VoxObject)object).properties.put(expr.name.text, value);
      } else {
        throw new RuntimeError("Cannot add properties to primitive values.",
            expr.name);
      }
    } else {
      locals.assign(expr.name, value);
    }

    return value;
  }

  @Override
  public Object visitBinaryExpr(Expr.Binary expr, Variables locals) {
    Object left = evaluate(expr.left, locals);
    Object right = evaluate(expr.right, locals);

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
  public Object visitCallExpr(Expr.Call expr, Variables locals) {
    Object callable = evaluate(expr.callee, locals);

    List<Object> arguments = new ArrayList<>();
    for (Expr argument : expr.arguments) {
      arguments.add(evaluate(argument, locals));
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
  public Object visitGroupingExpr(Expr.Grouping expr, Variables locals) {
    return evaluate(expr.expression, locals);
  }

  @Override
  public Object visitLiteralExpr(Expr.Literal expr, Variables locals) {
    return expr.value;
  }

  @Override
  public Object visitLogicalExpr(Expr.Logical expr, Variables locals) {
    Object left = evaluate(expr.left, locals);

    if (expr.operator.type == TokenType.OR && Primitives.isTrue(left)) return left;
    if (expr.operator.type == TokenType.AND && !Primitives.isTrue(left)) return left;

    return evaluate(expr.right, locals);
  }

  @Override
  public Object visitPropertyExpr(Expr.Property expr, Variables locals) {
    Object object = evaluate(expr.object, locals);
    if (object instanceof VoxObject) {
      return ((VoxObject)object).getProperty(expr.name);
    }

    throw new RuntimeError("Cannot access properties on primitive values.",
        expr.name);
  }

  @Override
  public Object visitUnaryExpr(Expr.Unary expr, Variables locals) {
    Object right = evaluate(expr.right, locals);

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
  public Object visitVariableExpr(Expr.Variable expr, Variables locals) {
    return locals.lookUp(expr.name);
    // TODO: Talk about late binding:
    //
    //   if (false) variableThatIsNotDefined;
    //
    // No error in a late bound language, but error in eager.
  }
}
