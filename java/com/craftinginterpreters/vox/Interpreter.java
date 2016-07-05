//>= Interpreting ASTs
package com.craftinginterpreters.vox;
//>= Variables

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
//>= Interpreting ASTs

// Tree-walk interpreter.
/*== Interpreting ASTs
class Interpreter implements Expr.Visitor<Object> {
*/
//>= Variables
class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
//>= Interpreting ASTs
  private final ErrorReporter reporter;
//>= Variables

  // The top level global variables.
  private final Environment globals = new Environment(null);
  private Environment environment = globals;
//>= Closures

  private Map<Expr, Integer> locals;
//>= Interpreting ASTs

  Interpreter(ErrorReporter reporter) {
    this.reporter = reporter;
//>= Functions

    globals.define("print", Callable.wrap(this::print));
//>= Uhh
    globals.define("clock", Callable.wrap(this::clock));
//>= Interpreting ASTs
  }

/*== Interpreting ASTs
  void interpret(Expr expression) {
    try {
      print(evaluate(expression));
    } catch (RuntimeError error) {
      reporter.runtimeError(error.token.line, error.getMessage());
    }
  }
*/
/*== Variables
  void interpret(List<Stmt> statements) {
*/
//>= Closures
  void interpret(List<Stmt> statements, Map<Expr, Integer> locals) {
    this.locals = locals;

//>= Variables
    try {
      for (Stmt statement : statements) {
        execute(statement);
      }
    } catch (RuntimeError error) {
      reporter.runtimeError(error.token.line, error.getMessage());
    }
  }
//>= Interpreting ASTs

  private Object evaluate(Expr expr) {
    return expr.accept(this);
  }
//>= Variables

  private void execute(Stmt stmt) {
    stmt.accept(this);
  }
//>= Closures

  void executeIn(Stmt stmt, Environment environment) {
    Environment previous = this.environment;
    try {
      this.environment = environment;
      execute(stmt);
    } finally {
      this.environment = previous;
    }
  }
//>= Variables

  @Override
  public Void visitBlockStmt(Stmt.Block stmt) {
    Environment previous = environment;
    try {
      environment = environment.beginScope();

      for (Stmt statement : stmt.statements) {
        execute(statement);
      }
    } finally {
      environment = previous;
    }
    return null;
  }
//>= Classes

  @Override
  public Void visitClassStmt(Stmt.Class stmt) {
    environment.declare(stmt.name);

    Map<String, VoxFunction> methods = new HashMap<>();
    Object superclass = null;
    if (stmt.superclass != null) {
      superclass = evaluate(stmt.superclass);
      if (!(superclass instanceof VoxClass)) {
        throw new RuntimeError(stmt.name,
            "Superclass must be a class.");
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
//>= Variables

  @Override
  public Void visitExpressionStmt(Stmt.Expression stmt) {
    evaluate(stmt.expression);
    return null;
  }
//>= Uhh

  @Override
  public Void visitForStmt(Stmt.For stmt) {
    return null;
  }
//>= Functions

  @Override
  public Void visitFunctionStmt(Stmt.Function stmt) {
    environment.declare(stmt.name);
    VoxFunction function = new VoxFunction(stmt, environment, false);
    environment.set(stmt.name, function);
    return null;
  }
//>= Control Flow

  @Override
  public Void visitIfStmt(Stmt.If stmt) {
    if (isTrue(evaluate(stmt.condition))) {
      executeIn(stmt.thenBranch, environment.beginScope());
    } else if (stmt.elseBranch != null) {
      executeIn(stmt.elseBranch, environment.beginScope());
    }
    return null;
  }
//>= Functions

  @Override
  public Void visitReturnStmt(Stmt.Return stmt) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value);

    throw new Return(value);
  }
//>= Variables

  @Override
  public Void visitVarStmt(Stmt.Var stmt) {
    Object value = null;
    if (stmt.initializer != null) {
      value = evaluate(stmt.initializer);
    }
    environment.define(stmt.name.text, value);
    return null;
  }
//>= Control Flow

  @Override
  public Void visitWhileStmt(Stmt.While stmt) {
    while (isTrue(evaluate(stmt.condition))) {
      executeIn(stmt.body, environment.beginScope());
    }
    return null;
  }
//>= Variables

  @Override
  public Object visitAssignExpr(Expr.Assign expr) {
    Object value = evaluate(expr.value);

/*== Variables
    // TODO: Handle late bound locals.
    globals.set(expr.name, value);
*/
//>= Closures
    Integer distance = locals.get(expr);
    if (distance != null) {
      environment.setAt(distance, expr.name, value);
    } else {
      globals.set(expr.name, value);
    }
//>= Variables

    return value;
  }
//>= Interpreting ASTs

  @Override
  public Object visitBinaryExpr(Expr.Binary expr) {
    Object left = evaluate(expr.left);
    Object right = evaluate(expr.right);

    switch (expr.operator.type) {
      case BANG_EQUAL: return !isEqual(left, right);
      case EQUAL_EQUAL: return isEqual(left, right);

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

        if (left instanceof String && right instanceof String) {
          return (String)left + (String)right;
        }

        throw new RuntimeError(expr.operator,
            "Operands must be two numbers or two strings.");

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
//>= Functions

  @Override
  public Object visitCallExpr(Expr.Call expr) {
    Object callee = evaluate(expr.callee);

    List<Object> arguments = new ArrayList<>();
    for (Expr argument : expr.arguments) {
      arguments.add(evaluate(argument));
    }

    if (!(callee instanceof Callable)) {
      throw new RuntimeError(expr.paren,
          "Can only call functions and classes.");
    }

    Callable function = (Callable)callee;
    if (arguments.size() < function.requiredArguments()) {
      throw new RuntimeError(expr.paren, "Not enough arguments.");
    }

    return function.call(this, arguments);
  }
//>= Classes

  @Override
  public Object visitGetExpr(Expr.Get expr) {
    Object object = evaluate(expr.object);
    if (object instanceof VoxInstance) {
      return ((VoxInstance) object).getProperty(expr.name);
    }

    throw new RuntimeError(expr.name,
        "Only instances have properties.");
  }
//>= Interpreting ASTs

  @Override
  public Object visitGroupingExpr(Expr.Grouping expr) {
    return evaluate(expr.expression);
  }

  @Override
  public Object visitLiteralExpr(Expr.Literal expr) {
    return expr.value;
  }
//>= Control Flow

  @Override
  public Object visitLogicalExpr(Expr.Logical expr) {
    Object left = evaluate(expr.left);

    if (expr.operator.type == TokenType.OR && isTrue(left)) {
      return left;
    }

    if (expr.operator.type == TokenType.AND && !isTrue(left)) {
      return left;
    }

    return evaluate(expr.right);
  }
//>= Classes

  @Override
  public Object visitSetExpr(Expr.Set expr) {
    Object value = evaluate(expr.value);
    Object object = evaluate(expr.object);

    if (object instanceof VoxInstance) {
      ((VoxInstance)object).fields.put(expr.name.text, value);
      return value;
    }

    throw new RuntimeError(expr.name, "Only instances have fields.");
  }
//>= Inheritance

  @Override
  public Object visitSuperExpr(Expr.Super expr) {
    VoxClass methodClass = (VoxClass) environment.get("class",
        expr.keyword);
    VoxClass superclass = methodClass.superclass;

    VoxInstance receiver = (VoxInstance) environment.get("this",
        expr.keyword);

    VoxFunction method = superclass.findMethod(receiver,
        expr.method.text);
    if (method == null) {
      throw new RuntimeError(expr.method,
          "Undefined property '" + expr.method.text + "'.");
    }

    return method;
  }
//>= Classes

  @Override
  public Object visitThisExpr(Expr.This expr) {
    return environment.get(expr.name);
  }
//>= Interpreting ASTs

  @Override
  public Object visitUnaryExpr(Expr.Unary expr) {
    Object right = evaluate(expr.right);

    switch (expr.operator.type) {
      case BANG:
        return !isTrue(right);
      case MINUS:
        checkNumberOperand(expr.operator, right);
        return -(double)right;
    }

    // Unreachable.
    return null;
  }
//>= Variables

  @Override
  public Object visitVariableExpr(Expr.Variable expr) {
//>= Closures
    Integer distance = locals.get(expr);
    if (distance != null) {
      return environment.getAt(distance, expr.name);
    } else {
//>= Variables
      return globals.get(expr.name);
//>= Closures
    }
//>= Variables
  }
//>= Interpreting ASTs

  private void checkNumberOperands(Token operator,
                                   Object left, Object right) {
    if (left instanceof Double && right instanceof Double) {
      return;
    }

    throw new RuntimeError(operator, "Operands must be numbers.");
  }

  private void checkNumberOperand(Token operator, Object left) {
    if (left instanceof Double) {
      return;
    }

    throw new RuntimeError(operator, "Operand must be a number.");
  }

  private Object print(Object argument) {
    System.out.println(stringify(argument));
    return argument;
  }
//>= Uhh

  private Object clock() {
    return (double)System.currentTimeMillis() / 1000.0;
  }
//>= Interpreting ASTs

  private boolean isTrue(Object object) {
    if (object == null) return false;
    if (object instanceof Boolean) return (boolean)object;
    return true;
  }

  private boolean isEqual(Object a, Object b) {
    // nil is only equal to nil.
    if (a == null && b == null) return true;
    if (a == null) return false;

    return a.equals(b);
  }

  private String stringify(Object object) {
    if (object == null) return "nil";

    // Hack. Work around Java adding ".0" to integer-valued doubles.
    if (object instanceof Double) {
      String text = object.toString();
      if (text.endsWith(".0")) {
        text = text.substring(0, text.length() - 2);
      }
      return text;
    }

    return object.toString();
  }
}
