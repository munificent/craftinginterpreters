package com.craftinginterpreters.vox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

// Tree-walk interpreter.
class Interpreter implements Stmt.Visitor<Local, Local>,
    Expr.Visitor<Object, Local> {
  private final ErrorReporter errorReporter;

  // The top level global variables.
  private final Map<String, Object> globals = new HashMap<>();

  private final VoxClass objectClass;

  Interpreter(ErrorReporter errorReporter) {
    this.errorReporter = errorReporter;

    // TODO: Methods.
    objectClass = new VoxClass("Object", null, null, new HashMap<>());

    globals.put("print", Callable.wrap(Primitives::print));
    globals.put("clock", Callable.wrap(Primitives::clock));
  }

  void run(String source) {
    Scanner scanner = new Scanner(source);
    Parser parser = new Parser(scanner, errorReporter);
    List<Stmt> statements = parser.parseProgram();

    // Don't run if there was a parse error.
    if (errorReporter.hadError) return;

    for (Stmt statement : statements) {
      execute(statement, null);
    }
  }

  private Object evaluate(Expr expr, Local locals) {
    return expr.accept(this, locals);
  }

  Local execute(Stmt stmt, Local locals) {
    return stmt.accept(this, locals);
  }

  @Override
  public Local visitBlockStmt(Stmt.Block stmt, Local locals) {
    Local before = locals;
    locals = new Local(locals, "", null);
    for (Stmt statement : stmt.statements) {
      locals = execute(statement, locals);
    }
    return before;
  }

  @Override
  public Local visitClassStmt(Stmt.Class stmt, Local locals) {
    locals = declareVariable(locals, stmt.name);

    Map<String, VoxFunction> methods = new HashMap<>();
    Object superclass = objectClass;
    if (stmt.superclass != null) {
      superclass = evaluate(stmt.superclass, locals);
      if (!(superclass instanceof VoxClass)) {
        throw new RuntimeError(Primitives.stringify(superclass) + " is not a class.",
            stmt.name);
      }
    }

    VoxFunction constructor = null;
    for (Stmt.Function method : stmt.methods) {
      VoxFunction function = new VoxFunction(method, locals);

      if (method.name.text.equals(stmt.name.text)) {
        constructor = function;
      } else {
        methods.put(method.name.text, function);
      }
    }

    VoxClass voxClass = new VoxClass(stmt.name.text,
        (VoxClass)superclass, constructor, methods);
    defineVariable(locals, stmt.name, voxClass);
    return locals;
  }

  @Override
  public Local visitExpressionStmt(Stmt.Expression stmt, Local locals) {
    evaluate(stmt.expression, locals);
    return locals;
  }

  @Override
  public Local visitForStmt(Stmt.For stmt, Local locals) {
    return locals;
  }

  @Override
  public Local visitFunctionStmt(Stmt.Function stmt, Local locals) {
    locals = declareVariable(locals, stmt.name);
    VoxFunction function = new VoxFunction(stmt, locals);
    defineVariable(locals, stmt.name, function);
    return locals;
  }

  @Override
  public Local visitIfStmt(Stmt.If stmt, Local locals) {
    Local before = locals;
    locals = new Local(locals, "", null);

    if (Primitives.isTrue(evaluate(stmt.condition, locals))) {
      execute(stmt.thenBranch, locals);
    } else if (stmt.elseBranch != null) {
      execute(stmt.elseBranch, locals);
    }

    return before;
  }

  @Override
  public Local visitReturnStmt(Stmt.Return stmt, Local locals) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value, locals);

    throw new Return(value);
  }

  @Override
  public Local visitVarStmt(Stmt.Var stmt, Local locals) {
    locals = declareVariable(locals, stmt.name);
    Object value = evaluate(stmt.initializer, locals);
    defineVariable(locals, stmt.name, value);
    return locals;
  }

  @Override
  public Local visitWhileStmt(Stmt.While stmt, Local locals) {
    Local before = locals;
    locals = new Local(locals, "", null);

    while (Primitives.isTrue(evaluate(stmt.condition, locals))) {
      execute(stmt.body, locals);
    }

    return before;
  }

  @Override
  public Object visitAssignExpr(Expr.Assign expr, Local locals) {
    Object value = evaluate(expr.value, locals);

    if (expr.object != null) {
      Object object = evaluate(expr.object, locals);
      if (object instanceof VoxObject) {
        ((VoxObject)object).fields.put(expr.name.text, value);
      } else {
        throw new RuntimeError("Only instances have fields.",
            expr.name);
      }
    } else {
      assign(expr.name, value, locals);
    }

    return value;
  }

  @Override
  public Object visitBinaryExpr(Expr.Binary expr, Local locals) {
    Object left = evaluate(expr.left, locals);
    Object right = evaluate(expr.right, locals);

    // TODO: Type check arithmetic operators.
    switch (expr.operator.type) {
      case BANG_EQUAL: return !Primitives.isEqual(left, right);
      case EQUAL_EQUAL: return Primitives.isEqual(left, right);
      case GREATER: return (double)left > (double)right;
      case GREATER_EQUAL: return (double)left >= (double)right;
      case LESS: return (double)left < (double)right;
      case LESS_EQUAL: return (double)left <= (double)right;
      case MINUS: return (double)left - (double)right;
      case PLUS:
        if (left instanceof Double && right instanceof Double) {
          return (double)left + (double)right;
        }

        if (left instanceof String || right instanceof String) {
          return Primitives.stringify(left) + Primitives.stringify(right);
        }

        throw new RuntimeError(
            "Can only add two strings or two numbers.",
            expr.operator);

      case SLASH: return (double)left / (double)right;
      case STAR: return (double)left * (double)right;
    }

    // Unreachable.
    return null;
  }

  @Override
  public Object visitCallExpr(Expr.Call expr, Local locals) {
    Object callable = evaluate(expr.callee, locals);

    List<Object> arguments = new ArrayList<>();
    for (Expr argument : expr.arguments) {
      arguments.add(evaluate(argument, locals));
    }

    if (!(callable instanceof Callable)) {
      throw new RuntimeError(
          "Can only call functions and classes.", expr.paren);
    }

    Callable function = (Callable)callable;
    if (arguments.size() < function.requiredArguments()) {
      throw new RuntimeError("Not enough arguments.", expr.paren);
    }

    return function.call(this, arguments);
  }

  @Override
  public Object visitGroupingExpr(Expr.Grouping expr, Local locals) {
    return evaluate(expr.expression, locals);
  }

  @Override
  public Object visitLiteralExpr(Expr.Literal expr, Local locals) {
    return expr.value;
  }

  @Override
  public Object visitLogicalExpr(Expr.Logical expr, Local locals) {
    Object left = evaluate(expr.left, locals);

    if (expr.operator.type == TokenType.OR && Primitives.isTrue(left)) return left;
    if (expr.operator.type == TokenType.AND && !Primitives.isTrue(left)) return left;

    return evaluate(expr.right, locals);
  }

  @Override
  public Object visitPropertyExpr(Expr.Property expr, Local locals) {
    Object object = evaluate(expr.object, locals);
    if (object instanceof VoxObject) {
      return ((VoxObject)object).getField(expr.name);
    }

    throw new RuntimeError("Only instances have fields.",
        expr.name);
  }

  @Override
  public Object visitThisExpr(Expr.This expr, Local locals) {
    return lookUp(expr.name, locals);
  }

  @Override
  public Object visitUnaryExpr(Expr.Unary expr, Local locals) {
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
  public Object visitVariableExpr(Expr.Variable expr, Local locals) {
    return lookUp(expr.name, locals);
    // TODO: Talk about late binding:
    //
    //   if (false) variableThatIsNotDefined;
    //
    // No error in a late bound language, but error in eager.
  }

  private Local declareVariable(Local locals, Token name) {
    // Globals don't need to be explicitly declared.
    if (locals == null) return null;

    return new Local(locals, name.text, null);
  }

  private void defineVariable(Local locals, Token name,
                              Object value) {
    if (locals == null) {
      globals.put(name.text, value);
    } else {
      locals.value = value;
    }
  }

  private Object lookUp(Token name, Local locals) {
    Local local = findVariable(locals, name);
    if (local != null) {
      return local.value;
    }

    return globals.get(name.text);
  }

  private void assign(Token name, Object value, Local locals) {
    Local local = findVariable(locals, name);
    if (local != null) {
      local.value = value;
      return;
    }

    globals.put(name.text, value);
  }

  private Local findVariable(Local locals, Token name) {
    while (locals != null) {
      if (locals.name.equals(name.text)) return locals;
      locals = locals.previous;
    }

    if (!globals.containsKey(name.text)) {
      throw new RuntimeError("Undefined variable '" + name.text + "'.", name);
    }

    // It's global.
    return null;
  }
}
