^title Classes
^part A Tree-Walk Interpreter



#### dependencies

- classes
    - instances
        - constructors
            - initializers
        - properties
        - methods
            - this

#### rough outline

- what are classes
  - way to bundle values together and pass around
  - associate behavior with bundled values
  - everything else like and dislike about oop

- class decls
  - class decl syntax
    - name, bag of methods
  - ast, parse

- start with empty class declarations, just named useless thing
- instances and the ability to construct them (but no params or initializers)
  - now can make instances of classes, just don't do anything useful
- add properties
  - classes now let you make instances that are data bags
  - these before methods since props are fundamental -- methods are bound fns
- methods
  - all instances of same class share behavior
  - method can't access object's own state!
- this
  - useful methods encapsulate behavior that acts on state
- initializer
  - encapsulate state from beginning
  - encapsulate chapter -- method uses this to bind fields

#### misc stuff

- talk about bound methods
- no syntax for method call

---

## class decls

^code class-ast (1 before, 1 after)

...

^code match-class (1 before, 1 after)

...

^code parse-class-declaration

...

^code resolver-visit-class

...

^code interpreter-visit-class

...

^code lox-class

## creating instances

...

^code lox-class-callable (2 before, 1 after)

...

^code lox-class-call

...

^code lox-class-arity

...

^code lox-instance

## properties on instances

...

^code get-ast (1 before, 1 after)

...

^code parse-property (1 before, 1 after)

...

^code resolver-visit-get

...

^code interpreter-visit-get

...

^code lox-instance-get-property

...

^code lox-instance-fields (1 before, 2 after)

...

^code set-ast (1 before, 1 after)

...

^code assign-set (1 before, 1 after)

...

^code resolver-visit-set

...

^code interpreter-visit-set

...

^code lox-instance-set-property

## methods on classes

[no method syntax needed because "." and call]

...

^code resolve-methods (1 before, 1 after)

[store METHOD in local because will change later]

...

^code function-type-method (1 before, 1 after)

...

^code interpret-methods (1 before, 1 after)

...

^code lox-class-methods (1 before, 3 after)

...

^code lox-instance-get-method (3 before, 2 after)

...

^code lox-class-find-method

[pass in instance but don't use yet]

## this

- can define methods on classes and store data, but methods can't access data
- [aside on implicit this]

- talk about how "this" is stored in environments
- could be implicit parameter to method
- makes closures and tear-offs harder

...

^code this-ast (1 before, 1 after)

...

^code parse-this (2 before, 2 after)

...

^code resolver-visit-this

...

^code resolver-begin-this-scope (1 before, 1 after)

...

^code resolver-end-this-scope (1 before, 1 after)

...

- need to bind this before method is even called

```lox
class Foo {
  bar() {
    print this;
  }
}

var method = Foo().bar;
method();
```

...

^code lox-class-find-method-bind (1 before, 1 after)

...

^code bind-self

- can't collide with variable name because "this" is *reserved* word

...

^code interpreter-visit-this

### this outside of methods

...

```lox
print this;
```

...

```lox
fun notAMethod() {
  print this;
}
```

...

^code class-type (1 before, 1 after)

- define other cases later

...

^code set-current-class (1 before, 2 after)

...

^code restore-current-class (3 before, 1 after)

...

^code this-outside-of-class (1 before, 1 after)

## constructors and initializers

- initializers
  - implicitly called after obj constructed
  - get passed ctor parameters
  - implicitly return this
  - (cannot return other value)

^code lox-class-call-initializer (1 before, 1 after)

...

^code lox-initializer-arity (1 before, 1 after)

- what happens if tear off init?

```lox
class Foo {
  init() {
    print this;
  }
}

var closure = Foo().init;
closure();
print closure();
```

- what does it return?
- for compat with clox, return this

...

^code return-this (2 before, 1 after)

- can't just check name because can have fun named "init" outside of class

^code is-initializer-field (1 before, 2 after)

- need to fix callsites where create fn

^code construct-function (1 before, 1 after)

...

^code interpreter-method-initializer (1 before, 1 after)

...

^code lox-function-bind-with-initializer (1 before, 1 after)

- want error in

```lox
class Foo {
  init() {
    return "something else";
  }
}
```

...

^code function-type-initializer (1 before, 1 after)

...

^code resolver-initializer-type (1 before, 1 after)

...

^code return-in-initializer (1 before, 1 after)

---

### notes

- challenge to add static methods? metaclasses?
- design note around prototypes versus classes?
- aside about ruby using "@" to encapsulate properties inside class

--

When talking about instances, talk about shadow classes and other more
efficient ways of storing fields than a hash map.

--

When explaining "this", go through closing over it in an inner function.

--

When talking about bound methods, explain how users generally expect
parenthesizing or hoisting a subexpression to not affect a behavior. Bound
methods ensure that:

    foo.bar(arg);

Means the same as:

    (foo.bar)(arg);

And:

    var bar = foo.bar;
    bar(arg);
