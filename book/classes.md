^title Classes
^part A Tree-Walk Interpreter

- classes
  - instances
    - constructors
      - initializers
    - properties
    - methods
      - this

- start with empty class declarations, just named useless thing
- instances and the ability to construct them (but no params or initializers)
  - now can make instances of classes, just don't do anything useful
- add properties
  - classes now let you make instances that are data bags
- methods
  - all instances of same class share behavior
  - method can't access object's own state!
- this
  - useful methods encapsulate behavior that acts on state
- initializer
  - encapsulate state from beginning

---

...

^code lox-instance

...

^code lox-instance-get-property

...

^code lox-class

**todo: decl say implements LoxCallable but doesn't until ctors below**

...

^code lox-class-find-method

...

^code match-class

...

^code parse-class-declaration

...

^code class-ast

...

^code function-types

...

^code class-type

...

^code resolver-visit-class

...

^code interpreter-visit-class

### this

...

^code this-ast

...

^code parse-this

...

^code resolver-begin-this-scope

...

^code resolver-end-this-scope

...

^code resolver-visit-this

...

^code interpreter-visit-this

...

^code bind-self

...

^code return-this

### properties

...

^code get-ast

...

^code set-ast

...

^code assign-set

...

^code parse-property

...

^code resolver-visit-get

...

^code resolver-visit-set

...

^code interpreter-visit-get

...

^code interpreter-visit-set

### constructors

...

^code is-initializer-field

...

^code lox-function-constructor

...

^code construct-function (1 before, 1 after)

...

^code resolver-initializer-type

...

^code return-in-initializer

...

^code lox-class-arity

...

^code lox-class-call

...

^code interpreter-method-initializer

...

^code initialize-is-initializer

---

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
