//>= Functions
package com.craftinginterpreters.vox;

import java.util.List;

class VoxFunction implements Callable {
  private final Stmt.Function declaration;
//>= Blocks and Binding
  private final Environment closure;
//>= Classes
  private final boolean isInitializer;
//>= Functions

/*== Functions
  VoxFunction(Stmt.Function declaration) {
*/
/*== Blocks and Binding
VoxFunction(Stmt.Function declaration, Environment closure) {
*/
//>= Classes
  VoxFunction(Stmt.Function declaration, Environment closure,
              boolean isInitializer) {
//>= Functions
    this.declaration = declaration;
//>= Blocks and Binding
    this.closure = closure;
//>= Classes
    this.isInitializer = isInitializer;
//>= Functions
  }

//>= Classes
  VoxFunction bind(VoxInstance self, VoxClass superclass) {
    Environment environment = closure.enterScope();
    environment.define("this", self);
    environment.define("super", superclass);
    return new VoxFunction(declaration, environment, isInitializer);
  }

//>= Functions
  @Override
  public String toString() {
    return declaration.name.text;
  }

  @Override
  public int requiredArguments() {
    return declaration.parameters.size();
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    Object result = null;

    try {
/*== Functions
      Environment environment = interpreter.globals.enterScope();
*/
//>= Blocks and Binding
      Environment environment = closure.enterScope();
//>= Functions
      for (int i = 0; i < declaration.parameters.size(); i++) {
        environment.define(declaration.parameters.get(i).text,
            arguments.get(i));
      }

      interpreter.executeBody(declaration.body, environment);
    } catch (Return returnValue) {
      result = returnValue.value;
    }

/*>= Functions <= Blocks and Binding
    return result;
*/
//>= Classes
    return isInitializer ? closure.getAt(0, "this") : result;
//>= Functions
  }
}
