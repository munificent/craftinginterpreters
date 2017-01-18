//> Evaluating Expressions not-yet
package com.craftinginterpreters.lox;

//> Functions not-yet
import java.util.ArrayList;
//< Functions not-yet
//> Classes not-yet
import java.util.HashMap;
//< Classes not-yet
//> Statements and State not-yet
import java.util.List;
//< Statements and State not-yet
//> Resolving and Binding not-yet
import java.util.Map;
//< Resolving and Binding not-yet
//> Statements and State not-yet

//< Statements and State not-yet
// Tree-walk interpreter.
/* Evaluating Expressions not-yet < Statements and State not-yet
class Interpreter implements Expr.Visitor<Object> {
*/
//> Statements and State not-yet
class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
//< Statements and State not-yet
/* Statements and State not-yet < Functions not-yet

  private Environment environment = new Environment();
*/
//> Functions not-yet

  final Environment globals = new Environment();
  private Environment environment = globals;
//< Functions not-yet
//> Resolving and Binding not-yet

  private Map<Expr, Integer> locals;
//< Resolving and Binding not-yet

//> Functions not-yet
  Interpreter() {
    globals.define("clock", new NativeFunction(0, this::clock));
  }
//< Functions not-yet

/* Evaluating Expressions not-yet < Statements and State not-yet
  void interpret(Expr expression) {
    try {
      print(evaluate(expression));
    } catch (RuntimeError error) {
      Lox.runtimeError(error);
    }
  }
*/
/* Statements and State not-yet < Resolving and Binding not-yet
  void interpret(List<Stmt> statements) {
*/
//> Statements and State not-yet
//> Resolving and Binding not-yet
  void interpret(List<Stmt> statements, Map<Expr, Integer> locals) {
    this.locals = locals;

//< Resolving and Binding not-yet
    try {
      for (Stmt statement : statements) {
        execute(statement);
      }
    } catch (RuntimeError error) {
      Lox.runtimeError(error);
    }
  }
//< Statements and State not-yet

  private Object evaluate(Expr expr) {
    return expr.accept(this);
  }
//> Statements and State not-yet

  private void execute(Stmt stmt) {
    stmt.accept(this);
  }

  void executeBody(List<Stmt> statements, Environment environment) {
    Environment previous = this.environment;
    try {
      this.environment = environment;

      for (Stmt statement : statements) {
        execute(statement);
      }
    } finally {
      this.environment = previous;
    }
  }

  @Override
  public Void visitBlockStmt(Stmt.Block stmt) {
    executeBody(stmt.statements, environment.enterScope());
    return null;
  }
//> Classes not-yet

  @Override
  public Void visitClassStmt(Stmt.Class stmt) {
    environment.declare(stmt.name);

    Map<String, LoxFunction> methods = new HashMap<>();
//> Inheritance not-yet
    Object superclass = null;
    if (stmt.superclass != null) {
      superclass = evaluate(stmt.superclass);
      if (!(superclass instanceof LoxClass)) {
        throw new RuntimeError(stmt.name,
            "Superclass must be a class.");
      }

      environment = environment.enterScope();
      environment.define("super", superclass);
    }

//< Inheritance not-yet
    for (Stmt.Function method : stmt.methods) {
      LoxFunction function = new LoxFunction(method, environment,
          method.name.lexeme.equals("init"));
        methods.put(method.name.lexeme, function);
    }

/* Classes not-yet < Inheritance not-yet
    LoxClass klass = new LoxClass(stmt.name.lexeme, methods);
*/
//> Inheritance not-yet
    LoxClass klass = new LoxClass(stmt.name.lexeme,
        (LoxClass)superclass, methods);

    if (superclass != null) {
      environment = environment.enclosing;
    }

//< Inheritance not-yet
    environment.assign(stmt.name, klass);
    return null;
  }
//< Classes not-yet

  @Override
  public Void visitExpressionStmt(Stmt.Expression stmt) {
    evaluate(stmt.expression);
    return null;
  }
//> Functions not-yet

  @Override
  public Void visitFunctionStmt(Stmt.Function stmt) {
    environment.declare(stmt.name);
/* Functions not-yet < Classes not-yet
    LoxFunction function = new LoxFunction(stmt, environment);
*/
//> Classes not-yet
    LoxFunction function = new LoxFunction(stmt, environment, false);
//< Classes not-yet
    environment.assign(stmt.name, function);
    return null;
  }
//< Functions not-yet
//> Control Flow not-yet

  @Override
  public Void visitIfStmt(Stmt.If stmt) {
    if (isTrue(evaluate(stmt.condition))) {
      execute(stmt.thenBranch);
    } else if (stmt.elseBranch != null) {
      execute(stmt.elseBranch);
    }
    return null;
  }
//< Control Flow not-yet

  @Override
  public Void visitPrintStmt(Stmt.Print stmt) {
    Object value = evaluate(stmt.expression);
    System.out.println(stringify(value));
    return null;
  }

//> Functions not-yet

  @Override
  public Void visitReturnStmt(Stmt.Return stmt) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value);

    throw new Return(value);
  }
//< Functions not-yet

  @Override
  public Void visitVarStmt(Stmt.Var stmt) {
    Object value = null;
    if (stmt.initializer != null) {
      value = evaluate(stmt.initializer);
    }

    environment.define(stmt.name.lexeme, value);
    return null;
  }
//> Control Flow not-yet

  @Override
  public Void visitWhileStmt(Stmt.While stmt) {
    while (isTrue(evaluate(stmt.condition))) {
      execute(stmt.body);
    }
    return null;
  }
//< Control Flow not-yet

  @Override
  public Object visitAssignExpr(Expr.Assign expr) {
    Object value = evaluate(expr.value);

/* Statements and State not-yet < Resolving and Binding not-yet
    environment.assign(expr.name, value);
*/
//> Resolving and Binding not-yet
    Integer distance = locals.get(expr);
    if (distance != null) {
      environment.assignAt(distance, expr.name, value);
    } else {
      globals.assign(expr.name, value);
    }
//< Resolving and Binding not-yet

    return value;
  }
//< Statements and State not-yet

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
//> Functions not-yet

  @Override
  public Object visitCallExpr(Expr.Call expr) {
    Object callee = evaluate(expr.callee);

    List<Object> arguments = new ArrayList<>();
    for (Expr argument : expr.arguments) {
      arguments.add(evaluate(argument));
    }

    if (!(callee instanceof Callable)) {
      // TODO: Change error message to not mention classes explicitly
      // since this shows up before classes are implemented.
      throw new RuntimeError(expr.paren,
          "Can only call functions and classes.");
    }

    Callable function = (Callable)callee;
    if (arguments.size() < function.requiredArguments()) {
      throw new RuntimeError(expr.paren, "Not enough arguments.");
    }

    return function.call(this, arguments);
  }
//< Functions not-yet
//> Classes not-yet

  @Override
  public Object visitGetExpr(Expr.Get expr) {
    Object object = evaluate(expr.object);
    if (object instanceof LoxInstance) {
      return ((LoxInstance) object).getProperty(expr.name);
    }

    throw new RuntimeError(expr.name,
        "Only instances have properties.");
  }
//< Classes not-yet

  @Override
  public Object visitGroupingExpr(Expr.Grouping expr) {
    return evaluate(expr.expression);
  }

  @Override
  public Object visitLiteralExpr(Expr.Literal expr) {
    return expr.value;
  }
//> Control Flow not-yet

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
//< Control Flow not-yet
//> Classes not-yet

  @Override
  public Object visitSetExpr(Expr.Set expr) {
    Object value = evaluate(expr.value);
    Object object = evaluate(expr.object);

    if (object instanceof LoxInstance) {
      ((LoxInstance)object).fields.put(expr.name.lexeme, value);
      return value;
    }

    throw new RuntimeError(expr.name, "Only instances have fields.");
  }
//< Classes not-yet
//> Inheritance not-yet

  @Override
  public Object visitSuperExpr(Expr.Super expr) {
    int distance = locals.get(expr);
    LoxClass superclass = (LoxClass)environment.getAt(distance, "super");

    // "this" is always one level nearer than "super"'s environment.
    LoxInstance receiver = (LoxInstance)environment.getAt(distance - 1, "this");

    LoxFunction method = superclass.findMethod(receiver, expr.method.lexeme);
    if (method == null) {
      throw new RuntimeError(expr.method,
          "Undefined property '" + expr.method.lexeme + "'.");
    }

    return method;
  }
//< Inheritance not-yet
//> Classes not-yet

  @Override
  public Object visitThisExpr(Expr.This expr) {
    return lookUpVariable(expr.keyword, expr);
  }
//< Classes not-yet

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
//> Statements and State not-yet

  @Override
  public Object visitVariableExpr(Expr.Variable expr) {
/* Statements and State not-yet < Resolving and Binding not-yet
    return environment.get(expr.name);
*/
//> Resolving and Binding not-yet
    return lookUpVariable(expr.name, expr);
//< Resolving and Binding not-yet
  }
//> Resolving and Binding not-yet

  private Object lookUpVariable(Token name, Expr expr) {
    Integer distance = locals.get(expr);
    if (distance != null) {
      return environment.getAt(distance, name.lexeme);
    } else {
      return globals.get(name);
    }
  }
//< Resolving and Binding not-yet
//< Statements and State not-yet

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
//> Functions not-yet

  private Object clock(List<Object> arguments) {
    return (double)System.currentTimeMillis() / 1000.0;
  }
//< Functions not-yet

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
