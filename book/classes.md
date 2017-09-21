^title Classes
^part A Tree-Walk Interpreter

- lox now pretty ok procedural language
- if added couple of data structures, would be in same territory as early
  python, scheme (without macros), tcl, etc.
- this and next chapter kind of bonus

- what are classes
  - way to bundle values together and pass around
  - associate behavior with bundled values
  - everything else like and dislike about oop

- goal of oop to keep data structure and operations that affect it bundled
  together
- [may disagree with goal, ok but is what goal is]

- with classes, do this by having class declaration that
  - way to store and read fields on instances of class
  - constructors to create new instances of class with proper initial state
  - set of methods that operate on instances of that class and update state as
    needed

- really need all
  - without state on instances, obviously not useful. just namespaces for fns
  - without methods, objs just data bags, no encapsulation
  - without ctors, no way to create instances and no way to encapsulate initial
    state

- oop langs also inheritance, do later

- long chapter, won't get full picture until end, but try to organize reasonable
  so that thing works in middle

## class decl

- start with syntax
- name and set of methods
- no syntax for declaring fields
  - like other dynlang, can freely add

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

## creating inst

- when designing oop language, find ctor hardest most subtle
- lots of variation across langs
- birth magical moment where go from nothing to full fledged, initialized obj
- tricky figure out what guarantees get during middle of that process
- [exceptions in c++ to allow throw from ctor, def assign in java final fields]

- no syntax for create in lox
- class is directly callable

...

^code lox-class-callable (2 before, 1 after)

...

^code lox-class-call

...

^code lox-class-arity

...

^code lox-instance

## properties on instances

- can create but don't do anything
- fork in road -- state or behavior first
- state since syntactically methods depend on it

- key lang design q is can state be accessed outside of obj's own methods?
- can outside code directly read or modify?
- static langs use access control -- private public
- smalltalk and ruby say no
- ruby use "@" for fields and only accessible inside methods
- other dyn langs like js and python let freely

- do that
- every instance loose bag of props

- access using "."
- store using "." on lhs of assignment

- runtime rep hashmap on loxinstance

- [shadow classes and faster lookup]

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

## methods

- have instances as data struct, but not really objects
- behavior
- finally use method syntax parsed inside decl

- instance stores state, class stores behavior
- loxinstance has hashmap of props
- loxclass hashmap of methods

- no direct syntax for method calls
- already have "." and "()", so method call is just combination

```lox
object.method(argument);
```

- raises q:

```lox
var m = object.method;
m(argument);
```

- what happens when instance doesn't have property with name, but does have
  method?
- what happens if do "." but don't call it?

- other dir

```lox
class Box {}

fun notMethod(argument) {
  print "called function with " + argument;
}

var box = Box();
box.function = notMethod();
box.function("argument");
```

- what happens if store function in property and call it like method?
- want both to work
- former because users usually expect hoisting out subexpr to not change
  semantics

```lox
(object.method)() <-> object.method()
```

- latter because have first class fns and useful to store in fields

- don't have "this" yet, but will soon
- even trickier q:

```lox
class Person {
  sayName() {
    print this.name;
  }
}

var jane = Person();
jane.name "jane";

var method = jane.sayName;
method(); // ?
```

- if pull off method but don't call until later, does it "remember" this?

- super weird:

```lox
class Person {
  sayName() {
    print this.name;
  }
}

var jane = Person();
jane.name "jane";

var bill = Person();
bill.name = "bill";

bill.sayName = jane.sayName;
bill.sayName(); // ?
```

- pull method off one instance
- store as fn in prop on other
- then call it
- what?

- [lua and js different]

- answer to both is "yes", remembers this
- as soon as look up method on instance, before invoking, "this" is frozen and
  refers to that instance forever after

- follows python and c#
- generally what want

- means method semantics follow syntax
- don't "call method"
- instead, look up method (".") which returns fn, then invoke fn
- when get to "this", see how first step holds onto that
- for now, just worry about methods

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
  or other methods on "self"

- lot of options for accessing
- "self" or "this" common keywords

- [aside on implicit this]

- key challenge is how method gets and hangs onto "this"
- method is defined on class
- every instance of class uses same LoxFunction when calling method
- but each has different this
- almost like "this" is implicit arg passed to method
- [some langs very directly like this]

- bound fns make that harder
- need to "pass in" this but not call method yet

- promising approach
- already have closures, so envs are good way to associate state with fn that
  lives and gets passed around with fn
- trick is that "passing in this" happens when method is looked up ".", which
  is separate from when resulting fn is called

- so when exec "." where name is method, both look up method and bind this to
  object on lhs

- then, inside method, can use "this" expr to access that closed over value

- treating "this" like var also handles using this inside fn in method

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

## constructors and inits

- can do almost everything
- behavior and state are encapsulated
- methods help ensure object stays in good state
- ensure new obj is in initial good state?
- constructors

- two process -- allocate fresh instance, initialize
- first is magic - runtime provides, user can't usually impl
- [c++ placement new]
- second is up to user -- given bare obj, set up
- syntax?
- some lang use class name
- "init" like ruby and python
- not reserved

- when call class like fn
- creates instance
- if class has method init(), call
- forward any args

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

### notes

- challenge to add static methods? metaclasses?
- design note around prototypes versus classes?
- aside about ruby using "@" to encapsulate properties inside class
