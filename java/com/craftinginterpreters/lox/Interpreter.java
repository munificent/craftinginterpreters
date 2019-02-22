//> Evaluating Expressions interpreter-class
package com.craftinginterpreters.lox;

//> Functions import-array-list
import java.util.ArrayList;
//< Functions import-array-list
//> Resolving and Binding import-hash-map
import java.util.HashMap;
//< Resolving and Binding import-hash-map
//> Statements and State import-list
import java.util.List;
//< Statements and State import-list
//> Resolving and Binding import-map
import java.util.Map;
//< Resolving and Binding import-map

/* Evaluating Expressions interpreter-class < Statements and State interpreter
class Interpreter implements Expr.Visitor<Object> {
*/
//> Statements and State interpreter
class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
//< Statements and State interpreter
/* Statements and State environment-field < Functions global-environment
  private Environment environment = new Environment();

*/
//> Functions global-environment
  final Environment globals = new Environment();
  private Environment environment = globals;
//< Functions global-environment
//> Resolving and Binding locals-field
  private final Map<Expr, Integer> locals = new HashMap<>();
//< Resolving and Binding locals-field
//> Functions interpreter-constructor

  Interpreter() {
    globals.define("clock", new LoxCallable() {
      @Override
      public int arity() { return 0; }

      @Override
      public Object call(Interpreter interpreter,
                         List<Object> arguments) {
        return (double)System.currentTimeMillis() / 1000.0;
      }

      @Override
      public String toString() { return "<native fn>"; }
    });
  }
//< Functions interpreter-constructor
/* Evaluating Expressions interpret < Statements and State interpret
  void interpret(Expr expression) {
    try {
      Object value = evaluate(expression);
      System.out.println(stringify(value));
    } catch (RuntimeError error) {
      Lox.runtimeError(error);
    }
  }
*/
//> Statements and State interpret
  void interpret(List<Stmt> statements) {
    try {
      for (Stmt statement : statements) {
        execute(statement);
      }
    } catch (RuntimeError error) {
      Lox.runtimeError(error);
    }
  }
//< Statements and State interpret
//> evaluate
  private Object evaluate(Expr expr) {
    return expr.accept(this);
  }
//< evaluate
//> Statements and State execute
  private void execute(Stmt stmt) {
    stmt.accept(this);
  }
//< Statements and State execute
//> Resolving and Binding resolve
  void resolve(Expr expr, int depth) {
    locals.put(expr, depth);
  }
//< Resolving and Binding resolve
//> Statements and State execute-block
  void executeBlock(List<Stmt> statements, Environment environment) {
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
//< Statements and State execute-block
//> Statements and State visit-block
  @Override
  public Void visitBlockStmt(Stmt.Block stmt) {
    executeBlock(stmt.statements, new Environment(environment));
    return null;
  }
//< Statements and State visit-block
//> Classes interpreter-visit-class
  @Override
  public Void visitClassStmt(Stmt.Class stmt) {
//> Inheritance interpret-superclass
    Object superclass = null;
    if (stmt.superclass != null) {
      superclass = evaluate(stmt.superclass);
      if (!(superclass instanceof LoxClass)) {
        throw new RuntimeError(stmt.superclass.name,
            "Superclass must be a class.");
      }
    }

//< Inheritance interpret-superclass
    environment.define(stmt.name.lexeme, null);
//> Inheritance begin-superclass-environment

    if (stmt.superclass != null) {
      environment = new Environment(environment);
      environment.define("super", superclass);
    }
//< Inheritance begin-superclass-environment
//> interpret-methods

    Map<String, LoxFunction> methods = new HashMap<>();
    for (Stmt.Function method : stmt.methods) {
/* Classes interpret-methods < Classes interpreter-method-initializer
      LoxFunction function = new LoxFunction(method, environment);
*/
//> interpreter-method-initializer
      LoxFunction function = new LoxFunction(method, environment,
          method.name.lexeme.equals("init"));
//< interpreter-method-initializer
      methods.put(method.name.lexeme, function);
    }

/* Classes interpret-methods < Inheritance interpreter-construct-class
    LoxClass klass = new LoxClass(stmt.name.lexeme, methods);
*/
//> Inheritance interpreter-construct-class
    LoxClass klass = new LoxClass(stmt.name.lexeme,
        (LoxClass)superclass, methods);
//> end-superclass-environment

    if (superclass != null) {
      environment = environment.enclosing;
    }
//< end-superclass-environment

//< Inheritance interpreter-construct-class
//< interpret-methods
/* Classes interpreter-visit-class < Classes interpret-methods
    LoxClass klass = new LoxClass(stmt.name.lexeme);
*/
    environment.assign(stmt.name, klass);
    return null;
  }
//< Classes interpreter-visit-class
//> Statements and State visit-expression-stmt
  @Override
  public Void visitExpressionStmt(Stmt.Expression stmt) {
    evaluate(stmt.expression);
    return null; // [void]
  }
//< Statements and State visit-expression-stmt
//> Functions visit-function
  @Override
  public Void visitFunctionStmt(Stmt.Function stmt) {
/* Functions visit-function < Functions visit-closure
    LoxFunction function = new LoxFunction(stmt);
*/
/* Functions visit-closure < Classes construct-function
    LoxFunction function = new LoxFunction(stmt, environment);
*/
//> Classes construct-function
    LoxFunction function = new LoxFunction(stmt, environment, false);
//< Classes construct-function
    environment.define(stmt.name.lexeme, function);
    return null;
  }
//< Functions visit-function
//> Control Flow visit-if
  @Override
  public Void visitIfStmt(Stmt.If stmt) {
    if (isTruthy(evaluate(stmt.condition))) {
      execute(stmt.thenBranch);
    } else if (stmt.elseBranch != null) {
      execute(stmt.elseBranch);
    }
    return null;
  }
//< Control Flow visit-if
//> Statements and State visit-print
  @Override
  public Void visitPrintStmt(Stmt.Print stmt) {
    Object value = evaluate(stmt.expression);
    System.out.println(stringify(value));
    return null;
  }
//< Statements and State visit-print
//> Functions visit-return
  @Override
  public Void visitReturnStmt(Stmt.Return stmt) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value);

    throw new Return(value);
  }
//< Functions visit-return
//> Statements and State visit-var
  @Override
  public Void visitVarStmt(Stmt.Var stmt) {
    Object value = null;
    if (stmt.initializer != null) {
      value = evaluate(stmt.initializer);
    }

    environment.define(stmt.name.lexeme, value);
    return null;
  }
//< Statements and State visit-var
//> Control Flow visit-while
  @Override
  public Void visitWhileStmt(Stmt.While stmt) {
    while (isTruthy(evaluate(stmt.condition))) {
      execute(stmt.body);
    }
    return null;
  }
//< Control Flow visit-while
//> Statements and State visit-assign
  @Override
  public Object visitAssignExpr(Expr.Assign expr) {
    Object value = evaluate(expr.value);

/* Statements and State visit-assign < Resolving and Binding resolved-assign
    environment.assign(expr.name, value);
*/
//> Resolving and Binding resolved-assign
    Integer distance = locals.get(expr);
    if (distance != null) {
      environment.assignAt(distance, expr.name, value);
    } else {
      globals.assign(expr.name, value);
    }

//< Resolving and Binding resolved-assign
    return value;
  }
//< Statements and State visit-assign
//> visit-binary
  @Override
  public Object visitBinaryExpr(Expr.Binary expr) {
    Object left = evaluate(expr.left);
    Object right = evaluate(expr.right); // [left]

    switch (expr.operator.type) {
//> binary-equality
      case BANG_EQUAL: return !isEqual(left, right);
      case EQUAL_EQUAL: return isEqual(left, right);
//< binary-equality
//> binary-comparison
      case GREATER:
//> check-greater-operand
        checkNumberOperands(expr.operator, left, right);
//< check-greater-operand
        return (double)left > (double)right;
      case GREATER_EQUAL:
//> check-greater-equal-operand
        checkNumberOperands(expr.operator, left, right);
//< check-greater-equal-operand
        return (double)left >= (double)right;
      case LESS:
//> check-less-operand
        checkNumberOperands(expr.operator, left, right);
//< check-less-operand
        return (double)left < (double)right;
      case LESS_EQUAL:
//> check-less-equal-operand
        checkNumberOperands(expr.operator, left, right);
//< check-less-equal-operand
        return (double)left <= (double)right;
//< binary-comparison
      case MINUS:
//> check-minus-operand
        checkNumberOperands(expr.operator, left, right);
//< check-minus-operand
        return (double)left - (double)right;
//> binary-plus
      case PLUS:
        if (left instanceof Double && right instanceof Double) {
          return (double)left + (double)right;
        } // [plus]

        if (left instanceof String && right instanceof String) {
          return (String)left + (String)right;
        }
//> string-wrong-type

        throw new RuntimeError(expr.operator,
            "Operands must be two numbers or two strings.");
//< string-wrong-type
//< binary-plus
      case SLASH:
//> check-slash-operand
        checkNumberOperands(expr.operator, left, right);
//< check-slash-operand
        return (double)left / (double)right;
      case STAR:
//> check-star-operand
        checkNumberOperands(expr.operator, left, right);
//< check-star-operand
        return (double)left * (double)right;
    }

    // Unreachable.
    return null;
  }
//< visit-binary
//> Functions visit-call
  @Override
  public Object visitCallExpr(Expr.Call expr) {
    Object callee = evaluate(expr.callee);

    List<Object> arguments = new ArrayList<>();
    for (Expr argument : expr.arguments) { // [in-order]
      arguments.add(evaluate(argument));
    }

//> check-is-callable
    if (!(callee instanceof LoxCallable)) {
      throw new RuntimeError(expr.paren,
          "Can only call functions and classes.");
    }

//< check-is-callable
    LoxCallable function = (LoxCallable)callee;
//> check-arity
    if (arguments.size() != function.arity()) {
      throw new RuntimeError(expr.paren, "Expected " +
          function.arity() + " arguments but got " +
          arguments.size() + ".");
    }

//< check-arity
    return function.call(this, arguments);
  }
//< Functions visit-call
//> Classes interpreter-visit-get
  @Override
  public Object visitGetExpr(Expr.Get expr) {
    Object object = evaluate(expr.object);
    if (object instanceof LoxInstance) {
      return ((LoxInstance) object).get(expr.name);
    }

    throw new RuntimeError(expr.name,
        "Only instances have properties.");
  }
//< Classes interpreter-visit-get
//> visit-grouping
  @Override
  public Object visitGroupingExpr(Expr.Grouping expr) {
    return evaluate(expr.expression);
  }
//< visit-grouping
//> visit-literal
  @Override
  public Object visitLiteralExpr(Expr.Literal expr) {
    return expr.value;
  }
//< visit-literal
//> Control Flow visit-logical
  @Override
  public Object visitLogicalExpr(Expr.Logical expr) {
    Object left = evaluate(expr.left);

    if (expr.operator.type == TokenType.OR) {
      if (isTruthy(left)) return left;
    } else {
      if (!isTruthy(left)) return left;
    }

    return evaluate(expr.right);
  }
//< Control Flow visit-logical
//> Classes interpreter-visit-set
  @Override
  public Object visitSetExpr(Expr.Set expr) {
    Object object = evaluate(expr.object);

    if (!(object instanceof LoxInstance)) { // [order]
      throw new RuntimeError(expr.name, "Only instances have fields.");
    }

    Object value = evaluate(expr.value);
    ((LoxInstance)object).set(expr.name, value);
    return value;
  }
//< Classes interpreter-visit-set
//> Inheritance interpreter-visit-super
  @Override
  public Object visitSuperExpr(Expr.Super expr) {
    int distance = locals.get(expr);
    LoxClass superclass = (LoxClass)environment.getAt(
        distance, "super");
//> super-find-this

    // "this" is always one level nearer than "super"'s environment.
    LoxInstance object = (LoxInstance)environment.getAt(
        distance - 1, "this");
//< super-find-this
//> super-find-method

    LoxFunction method = superclass.findMethod(expr.method.lexeme);
//> super-no-method

    if (method == null) {
      throw new RuntimeError(expr.method,
          "Undefined property '" + expr.method.lexeme + "'.");
    }

//< super-no-method
    return method.bind(object);
//< super-find-method
  }
//< Inheritance interpreter-visit-super
//> Classes interpreter-visit-this
  @Override
  public Object visitThisExpr(Expr.This expr) {
    return lookUpVariable(expr.keyword, expr);
  }
//< Classes interpreter-visit-this
//> visit-unary
  @Override
  public Object visitUnaryExpr(Expr.Unary expr) {
    Object right = evaluate(expr.right);

    switch (expr.operator.type) {
//> unary-bang
      case BANG:
        return !isTruthy(right);
//< unary-bang
      case MINUS:
//> check-unary-operand
        checkNumberOperand(expr.operator, right);
//< check-unary-operand
        return -(double)right;
    }

    // Unreachable.
    return null;
  }
//< visit-unary
//> Statements and State visit-variable
  @Override
  public Object visitVariableExpr(Expr.Variable expr) {
/* Statements and State visit-variable < Resolving and Binding call-look-up-variable
    return environment.get(expr.name);
*/
//> Resolving and Binding call-look-up-variable
    return lookUpVariable(expr.name, expr);
//< Resolving and Binding call-look-up-variable
  }
//> Resolving and Binding look-up-variable
  private Object lookUpVariable(Token name, Expr expr) {
    Integer distance = locals.get(expr);
    if (distance != null) {
      return environment.getAt(distance, name.lexeme);
    } else {
      return globals.get(name);
    }
  }
//< Resolving and Binding look-up-variable
//< Statements and State visit-variable
//> check-operand
  private void checkNumberOperand(Token operator, Object operand) {
    if (operand instanceof Double) return;
    throw new RuntimeError(operator, "Operand must be a number.");
  }
//< check-operand
//> check-operands
  private void checkNumberOperands(Token operator,
                                   Object left, Object right) {
    if (left instanceof Double && right instanceof Double) return;
    // [operand]
    throw new RuntimeError(operator, "Operands must be numbers.");
  }
//< check-operands
//> is-truthy
  private boolean isTruthy(Object object) {
    if (object == null) return false;
    if (object instanceof Boolean) return (boolean)object;
    return true;
  }
//< is-truthy
//> is-equal
  private boolean isEqual(Object a, Object b) {
    // nil is only equal to nil.
    if (a == null && b == null) return true;
    if (a == null) return false;

    return a.equals(b);
  }
//< is-equal
//> stringify
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
//< stringify
}
