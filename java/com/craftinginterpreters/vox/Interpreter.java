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

  private final VoxClass classClass;
  private final VoxClass functionClass;

  Interpreter(ErrorReporter errorReporter) {
    this.errorReporter = errorReporter;

    // TODO: Methods.
    VoxClass objectClass = new VoxClass("Object", null, new HashMap<>());
    classClass = new VoxClass("Class", null, new HashMap<>());
    functionClass = new VoxClass("Function", null, new HashMap<>());
    objectClass.setClass(classClass);
    classClass.setClass(classClass);
    functionClass.setClass(classClass);

    globals.put("print", Callable.wrap(Primitives::print));
    globals.put("clock", Callable.wrap(Primitives::clock));
    // TODO: What happens if a user tries to directly construct Class or
    // Function objects?
  }

  void run(String source) {
    Lexer lexer = new Lexer(source);
    Parser parser = new Parser(lexer, errorReporter);
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
    locals = beginBlock(locals);
    for (Stmt statement : stmt.statements) {
      locals = execute(statement, locals);
    }

    return before;
  }

  @Override
  public Local visitClassStmt(Stmt.Class stmt, Local locals) {
    Map<String, VoxFunction> methods = new HashMap<>();

    // TODO: Superclass.

    // TODO: Show what happens if don't define before creating class.
    locals = define(locals, stmt.name, null);

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
    assign(locals, stmt.name, voxClass);
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
    // TODO: Show what happens if don't define before creating fn.
    locals = define(locals, stmt.name, null);
    VoxFunction function = new VoxFunction(stmt, locals);
    function.setClass(functionClass);
    assign(locals, stmt.name, function);
    return locals;
  }

  @Override
  public Local visitIfStmt(Stmt.If stmt, Local locals) {
    locals = beginBlock(locals);
    if (Primitives.isTrue(evaluate(stmt.condition, locals))) {
      execute(stmt.thenBranch, locals);
    } else if (stmt.elseBranch != null) {
      execute(stmt.elseBranch, locals);
    }
    return locals;
  }

  @Override
  public Local visitReturnStmt(Stmt.Return stmt, Local locals) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value, locals);

    throw new Return(value);
  }

  @Override
  public Local visitVarStmt(Stmt.Var stmt, Local locals) {
    Object value = evaluate(stmt.initializer, locals);
    return define(locals, stmt.name, value);
  }

  @Override
  public Local visitWhileStmt(Stmt.While stmt, Local locals) {
    locals = beginBlock(locals);
    while (Primitives.isTrue(evaluate(stmt.condition, locals))) {
      execute(stmt.body, locals);
    }
    return locals;
  }

  @Override
  public Object visitAssignExpr(Expr.Assign expr, Local locals) {
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
      assign(locals, expr.name, value);
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
  public Object visitCallExpr(Expr.Call expr, Local locals) {
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
      return ((VoxObject)object).getProperty(expr.name);
    }

    throw new RuntimeError("Cannot access properties on primitive values.",
        expr.name);
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
    return lookUp(locals, expr.name);
    // TODO: Talk about late binding:
    //
    //   if (false) variableThatIsNotDefined;
    //
    // No error in a late bound language, but error in eager.
  }

  private Local beginBlock(Local outer) {
    return new Local(outer, null, null);
  }

  private Local define(Local locals, Token name, Object value) {
    if (locals != null) {
      return new Local(locals, name.text, value);
    }

    globals.put(name.text, value);
    return null;
  }

  private Object lookUp(Local locals, Token name) {
    Local local = locals;
    while (local != null) {
      if (name.text.equals(local.name)) return local.value;
      local = local.previous;
    }

    if (globals.containsKey(name.text)) return globals.get(name.text);

    throw new RuntimeError("Undefined variable '" + name.text + "'.", name);
  }

  private void assign(Local locals, Token name, Object value) {
    Local local = locals;
    while (local != null) {
      if (name.text.equals(local.name)) {
        local.value = value;
        return;
      }
      local = local.previous;
    }

    if (globals.containsKey(name.text)) {
      globals.put(name.text, value);
      return;
    }

    throw new RuntimeError("Undefined variable '" + name.text + "'.", name);
  }
}
