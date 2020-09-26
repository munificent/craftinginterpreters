1.  Metaclasses are so cool, I almost wish the book itself discussed them
    properly, but there are only so many pages. The idea is that a class object
    is itself an instance, which means it must have its own class -- a
    metaclass. That metaclass defines the methods that are available on the
    class object -- what you'd think of as the "static" methods in a language
    like Java.

    Before we get to metaclasses, we need to push the new syntax through. In
    AstGenerator, add a new field to Stmt.Class:

    ```java
    "Class : Token name, List<Stmt.Function> methods, List<Stmt.Function> classMethods",
    ```

    When parsing a class, we separate out the class methods (prefixed with
    "class") into a separate list:

    ```java
    private Stmt classDeclaration() {
      Token name = consume(IDENTIFIER, "Expect class name.");

      List<Stmt.Function> methods = new ArrayList<>();
      List<Stmt.Function> classMethods = new ArrayList<>();
      consume(LEFT_BRACE, "Expect '{' before class body.");

      while (!check(RIGHT_BRACE) && !isAtEnd()) {
        boolean isClassMethod = match(CLASS);
        (isClassMethod ? classMethods : methods).add(function("method"));
      }

      consume(RIGHT_BRACE, "Expect '}' after class body.");

      return new Stmt.Class(name, methods, classMethods);
    }
    ```

    In the resolver, we need to make sure to resolve the class methods too:

    ```java
    for (Stmt.Function method : stmt.classMethods) {
      beginScope();
      scopes.peek().put("this", true);
      resolveFunction(method, FunctionType.METHOD);
      endScope();
    }
    ```

    They are resolved mostly like methods. They even have a "this" variable,
    which will be the class itself.

    Now we're ready for metaclasses. Change the declaration of LoxClass to:

    ```java
    class LoxClass extends LoxInstance implements LoxCallable {
      final String name;
      private final Map<String, LoxFunction> methods;

      LoxClass(LoxClass metaclass, String name,
            Map<String, LoxFunction> methods) {
        super(metaclass);
        this.name = name;
        this.methods = methods;
      }

      // ...
    }
    ```

    LoxClass now extends LoxInstance. Every class object is also itself an
    instance of a class, its metaclass. When we interpret a class declaration,
    we create two LoxClasses:

    ```java
    public Void visitClassStmt(Stmt.Class stmt) {
      environment.define(stmt.name.lexeme, null);
      Map<String, LoxFunction> classMethods = new HashMap<>();
      for (Stmt.Function method : stmt.classMethods) {
        LoxFunction function = new LoxFunction(method, environment, false);
        classMethods.put(method.name.lexeme, function);
      }

      LoxClass metaclass = new LoxClass(null,
          stmt.name.lexeme + " metaclass", classMethods);

      Map<String, LoxFunction> methods = new HashMap<>();
      for (Stmt.Function method : stmt.methods) {
        LoxFunction function = new LoxFunction(method, environment,
            method.name.lexeme.equals("init"));
        methods.put(method.name.lexeme, function);
      }

      LoxClass klass = new LoxClass(metaclass, stmt.name.lexeme, methods);
      environment.assign(stmt.name, klass);
      return null;
    }
    ```

    First, we create a metaclass containing all of the class methods. It has
    null for its metametaclass to stop the infinite regress. Then we create the
    main class like we did previously. The only difference is that we pass in
    the metaclass as its class.

    That's it. There are no other interpreter changes. Now that LoxClass is an
    instance of LoxInstance, the existing code for property gets now applies to
    class objects. On the last line of:

    ```lox
    class Math {
      class square(n) {
        return n * n;
      }
    }

    print Math.square(3); // Prints "9".
    ```

    The `.square` expression looks at the object on the left. It's a
    LoxInstance. We call `.get()` on that. That fails to find a field named
    "square" so it looks for a method on the object's class with that name. The
    object's class is the metaclass, and the method is found there. You can
    even put fields on classes now:

    ```lox
    Math.pi = 3.141592653;
    print Math.pi;
    ```

2.  The first implementation detail we have to figure out is how our AST
    distinguishes a getter declaration from the declaration of a method that
    takes no parameters. This is kind of cute, but we'll use a *null*
    parameter list to indicate the former and an *empty* for the latter. So,
    when parsing a method (and only a method, there are no getter *functions*),
    we allow the parameter list to be omitted:

    ```java
    private Stmt.Function function(String kind) {
      Token name = consume(IDENTIFIER, "Expect " + kind + " name.");

      List<Token> parameters = null;

      // Allow omitting the parameter list entirely in method getters.
      if (!kind.equals("method") || check(LEFT_PAREN)) {
        consume(LEFT_PAREN, "Expect '(' after " + kind + " name.");
        parameters = new ArrayList<>();
        if (!check(RIGHT_PAREN)) {
          do {
            if (parameters.size() >= 8) {
              error(peek(), "Can't have more than 8 parameters.");
            }

            parameters.add(consume(IDENTIFIER, "Expect parameter name."));
          } while (match(COMMA));
        }
        consume(RIGHT_PAREN, "Expect ')' after parameters.");
      }

      consume(LEFT_BRACE, "Expect '{' before " + kind + " body.");
      List<Stmt> body = block();
      return new Stmt.Function(name, parameters, body);
    }
    ```

    Now we need to make sure the rest of the interpreter doesn't choke on a
    null parameter list. We check for it when resolving:

    ```java
    private void resolveFunction(Stmt.Function function, FunctionType type) {
      FunctionType enclosingFunction = currentFunction;
      currentFunction = type;

      beginScope();
      if (function.parameters != null) {
        for (Token param : function.parameters) {
          declare(param);
          define(param);
        }
      }
      resolve(function.body);
      endScope();
      currentFunction = enclosingFunction;
    }
    ```

    And when calling a LoxFunction:

    ```java
    public Object call(Interpreter interpreter, List<Object> arguments) {
      Environment environment = new Environment(closure);
      if (declaration.parameters != null) {
        for (int i = 0; i < declaration.parameters.size(); i++) {
          environment.define(declaration.parameters.get(i).lexeme,
              arguments.get(i));
        }
      }

      // ...
    }
    ```

    Now all that's left is to interpret getters. The only difference compared to
    methods is that the getter body is executed eagerly as soon as the property
    is accessed instead of waiting for a later call expression to invoke it.

    This isn't maybe the most elegant implementation, but it gets it done:

    ```java
    public Object visitGetExpr(Expr.Get expr) {
      Object object = evaluate(expr.object);
      if (object instanceof LoxInstance) {
        Object result = ((LoxInstance) object).get(expr.name);
        if (result instanceof LoxFunction &&
            ((LoxFunction) result).isGetter()) {
          result = ((LoxFunction) result).call(this, null);
        }

        return result;
      }

      throw new RuntimeError(expr.name,
          "Only instances have properties.");
    ```

    After looking up the property, we see if the resulting object is a getter.
    If so, we invoke it right now and use the result of that. This relies on
    one little helper in LoxFunction:

    ```java
    public boolean isGetter() {
      return declaration.parameters == null;
    }
    ```

    And that's it.

3.  Python and JavaScript allow you to freely access the fields on an object
    from outside of the methods on that object. Ruby and Smalltalk encapsulate
    instance state. Only methods on the class can access the raw fields, and it
    is up to the class to decide which state is exposed using getters and
    setters. Most statically typed languages offer access control modifiers
    like `private` and `public` to explicitly control on a per-member basis
    which parts of a class are externally accesible.

    What are the trade-offs between these approaches and why might a language
    might prefer one or the other?

    The decision to encapsulate at all or not is the classic
    trade-off between whether you want to make things easier for the class
    *consumer* or the class *maintainer*. By making everything public and
    freely externally visible and modifier, a downstream user of a class has
    more freedom to pop the hood open and muck around in the class's internals.

    However, that access tends to increasing coupling between the class and its
    users. That increased coupling makes the class itself more brittle, similar
    to the "fragile base class problem". If users are directly accessing
    properties that the class author considered implementation details, they
    lose the freedom to tweak that implementation without breaking those users.
    The class can end up harder to change. That's more painful for the
    maintainer, but also has a knock-on effect to the consumer -- if the class
    evolves more slowly, they get fewer newer features for free from the
    upstream maintainer.

    On the other hand, free external access to class state is a simpler, easier
    user experience when the class maintainer and consumer are the same person.
    If you're banging out a small script, it's handy to be able to just push
    stuff around without having to go through a lot of ceremony and boilerplate.
    At small scales, most language features that build fences in the program are
    more annoying than they are useful.

    As the program scales up, though, those fences become increasingly important
    since no one person is able to hold the entire program in their head.
    Boundaries in the code let you make productive changes while only knowing a
    single region of the program.

    Assuming you do want some sort of access control over properties, the next
    question is how fine-grained. Java has four different access control levels.
    That's four concepts the user needs to understand. Every time you add a
    member to a class, you need to pick one of the four, and need to have the
    expertise and forethought to choose wisely. This adds to the cognitive load
    of the language and adds some mental friction when programming.

    However, at large scales, each of those access control levels (except maybe
    package private) has proven to be useful. Having a few options gives class
    maintainers precise control over what extension points the class user has
    access to. While the class author has to do the mental work to pick a
    modifier, the class *consumer* gets to benefit from that. The modifier
    chosen for each member clearly communicates to the class user how the class
    is intended to be used. If you're subclassing a class and looking at a sea
    of methods, trying to figure out which one to override, the fact that one
    is protected while the others are all private or public makes your choice
    much easier -- it's a clear sign that that method is for the subclass's
    use.
