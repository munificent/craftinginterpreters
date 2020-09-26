1.  Consider:

    ```lox
    fun foo() {
      if (itsTuesday) foo();
    }
    ```

    The function does call itself inside it's definition. But it relies on some
    initial outer call to kick off the recursion. Some outside code must refer
    to "foo" by name first. That can't happen until the function declaration
    statement itself has finished executing. By then, "foo" is fully defined
    and is safe to use.

2.  In C, the variable is put in scope before its initializer, which means that
    the initializer refers to the variable being initialized. Since C does not
    require any clearing of uninitialized memory, it means you could potentially
    access garbage data.

    Java does not allow one local variable to shadow another so it's an error
    because of that if the outer variable is also local. The outer variable
    could be a field on the surrounding class. In that case, like C, the local
    variable is in scope in its own initializer. However, Java makes it an error
    to refer to a variable that may not have been initialized, so this falls
    under that case and is an error.

    Obviously, C's approach is crazy talk. Java is fine and takes advantage of
    definite assignment analysis, which is useful for other things (like
    ensuring final fields are initialized before the constructor body
    completes). I like when languages get a lot of mileage out of a single
    concept.

3.  The basic idea is that instead of storing just a boolean state for each
    local variable as we resolve the code, we'll allow a variable to be in one
    of three states:

    1. It has been declared but not yet defined.
    2. It has been defined but not yet read.
    3. It has been read.

    Any variable that goes out of scope when in the defined-but-not-yet-read
    state is an error. The annoying part is that we can't detect the error until
    the variable goes out of scope, but we want to report it on the line that
    the variable was declared. So we also need to keep track of the token from
    the variable declaration. We'll bundle that and the three-state enum into
    a little class inside the Resolver class:

    ```java
      private static class Variable {
        final Token name;
        VariableState state;

        private Variable(Token name, VariableState state) {
          this.name = name;
          this.state = state;
        }
      }

      private enum VariableState {
        DECLARED,
        DEFINED,
        READ
      }
    ```

    Then we change the scope stack to use that instead of Boolean:

    ```java
      private final Stack<Map<String, Variable>> scopes = new Stack<>();
    ```

    When we resolve a local variable, we mark it used. However, we don't want
    to consider assigning to a local variable to be a "use". Writing to a
    variable that's never read is still pointless. So we change resolveLocal()
    to:

    ```java
      private void resolveLocal(Expr expr, Token name, boolean isRead) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
          if (scopes.get(i).containsKey(name.lexeme)) {
            interpreter.resolve(expr, scopes.size() - 1 - i);

            // Mark it used.
            if (isRead) {
              scopes.get(i).get(name.lexeme).state = VariableState.READ;
            }
            return;
          }
        }

        // Not found. Assume it is global.
      }
    ```

    Every call to resolveLocal() needs to pass in that flag. In
    visitVariableExpr(), it's true:

    ```java
        resolveLocal(expr, expr.name, true);
    ```

    In visitAssignExpr(), it's false:

    ```java
        resolveLocal(expr, expr.name, false);
    ```

    Next, we update the existing code that touches scopes to use the new
    Variable class:

    ```java
      public Void visitVariableExpr(Expr.Variable expr) {
        if (!scopes.isEmpty() &&
            scopes.peek().containsKey(expr.name.lexeme) &&
            scopes.peek().get(expr.name.lexeme).state == VariableState.DECLARED) {
          Lox.error(expr.name,
              "Can't read local variable in its own initializer.");
        }

        resolveLocal(expr, expr.name, true);
        return null;
      }

      private void beginScope() {
        scopes.push(new HashMap<String, Variable>());
      }

      private void declare(Token name) {
        if (scopes.isEmpty()) return;

        Map<String, Variable> scope = scopes.peek();
        if (scope.containsKey(name.lexeme)) {
          Lox.error(name,
              "Already variable with this name in this scope.");
        }

        scope.put(name.lexeme, new Variable(name, VariableState.DECLARED));
      }

      private void define(Token name) {
        if (scopes.isEmpty()) return;
        scopes.peek().get(name.lexeme).state = VariableState.DEFINED;
      }
    ```

    Finally, when a scope is popped, we check its variables to see if any were
    not read:

    ```java
      private void endScope() {
        Map<String, Variable> scope = scopes.pop();

        for (Map.Entry<String, Variable> entry : scope.entrySet()) {
          if (entry.getValue().state == VariableState.DEFINED) {
            Lox.error(entry.getValue().name, "Local variable is not used.");
          }
        }
      }
    ```

4. This challenge is a real challenge and involves even more code changes.
   I went ahead and made a copy of the interpreter with the relevant changes
   in the "4" directory here.
