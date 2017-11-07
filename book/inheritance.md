^title Inheritance
^part A Tree-Walk Interpreter

- last chapter, almost done with java interp
- had to put lot in last chapter because all tied together
- one separable part, inheritance

- classes reuse behavior across instances
- inheritance shares behavior across classes
- reflects commonality between classes
- in static type lang, establishes subtype rel
- [not priv inheritance in c++]

- used to be major part of oop langs and oop style
- today, less used
- powerful tool, dangerous

- part of every oop lang back to simula
- want for lox too

## superclasses and subclasses

- what is inheritance?
- relationship between classes
- class inherits from another
- lot of terms
- base and derived
- class inherited from called superclass, inheriting class subclass
- history murky
- simula got "subclass" for c.a.r. hoare's record handling paper
- used "prefix class" for superclass
- not sure who started using super, maybe smalltalk

- need way to declare superclass
- langs have very different syntax
- python put super in parans
- c++/c# uses colon
- java, php "extends"
- simula put superclass before sub
- don't want to add keyword like "extends", don't have colon in grammar
- take ruby and use "<"
- [kinda weird, aligns with subtyping
    - set of all objects of super larger than set of all objs of sub]

- replace rule

```lox
classDecl → "class" IDENTIFIER ( "<" IDENTIFIER )?
            "{" function* "}" ;
```

- superclass optional
- can not have superclass
- no root obj class in lox, so no super means class is has no super at all

^code superclass-ast (1 before, 1 after)

- super class is expr
- grammatically always ident
- eval at runtime by looking up var

^code parse-superclass (1 before, 1 after)

- matches grammar

^code construct-class-ast (2 before, 1 after)

- store in ast
- null if no super

- class ast has new subexpr, need to resolve

^code resolve-superclass (1 before, 2 after)

- usually global var, but could be nested local var

- push through runtime
- eval superclass expr if have one

^code interpret-superclass (1 before, 1 after)

- no static checking, so need to check at runtime that got class
- can't inherit from a number

- store in runtime rep too

^code interpreter-construct-class (3 before, 1 after)

- need new field

^code lox-class-superclass-field (1 before, 1 after)

- init in ctor

^code lox-class-constructor (1 after)

- ok, have superclass now

## inheriting methods

- have superclass
- what does it do?
- if method isn't found on class, look for it in superclass
- in lang with static type, also establish subtype rel, storage around fields,
- for lox, just methods

- easy to impl

^code find-method-recurse-superclass (3 before, 1 after)

- literally it

```lox
class Doughnut {
  cook() {
    print "Fry until golden brown.";
  }
}

class BostonCream < Doughnut {}

BostonCream().cook();
```

## super

- earlier said if method isn't found on class, look for it in superclass
- "if" brings up important point
- if method found on subclass, does not look in superclass
- could be in both
- subclass overrides superclass
- [aside about beta inner]

- like shadowing var, means superclass method hidden
- other want subclass method that refines but does not replace super
- need access to superclass one
- super expr

```lox
class Doughnut {
  cook() {
    print "Fry until golden brown.";
  }
}

class BostonCream < Doughnut {
  cook() {
    super.cook();
    print "Pipe full of custard and coat with chocolate.";
  }
}

BostonCream().cook();
```

- super followed by dotted method call looks up method with name but starts
  search at superclass

### syntax

- seems like `this` in that sort of like magic var based on surrounding context
- not identifier but prefix of prop access

```lox
primary → "true" | "false" | "null" | "this"
        | NUMBER | STRING | IDENTIFIER | "(" expression ")"
        | "super" "." IDENTIFIER ;
```

- no bare "super", need dot and name after
- don't need arg list -- can get handle to super method then invoke later

^code super-expr (1 before, 1 after)

- expr store keyword (for errors) and name of prop

^code parse-super (2 before, 2 after)

- add new clause in primary

### semantics

- need to be more precise
- whose superclass?
- naive answer superclass of "this", receiver called

```lox
class A {
  method() {
    print "A method";
  }
}

class B < A {
  method() {
    print "B method";
  }

  test() {
    super.method();
  }
}

class C < B {}

C().test();
```

**todo: illustrate**

- when call C().test(), this is instance of c
- test declared inside B
- should start search at B's superclass
- super call search starts at superclass of *containing class where super call*

- to interp super call at runtime, need access to superclass of surrounding
  class
- could look up by name using same identifier that appears after "<" in decl
- error prone -- could be shadowed

```lox
class A {}
class B < A {
  var A = "not the superclass";
  super.oops();
}
```

- also need to be able to find superclass even if declaration out of scope:

```lox
fun makeSublass() {
  class A {
    method() {
      print "You found me.";
    }
  }

  class B < A {
    method() {
      super.method();
    }
  }

  return B;
}

B().method();
```

- look a lot like closure
- method body needs to close over reference to superclass so can find it
- put "super" in env like magic variable, similar to "this"

**todo: illustrate**

- since "super" reserved word, can't be shadowed by var

- resolver creates scope around class body

^code begin-super-scope (1 before, 1 after)

- ends when end of class

^code end-super-scope (2 before, 1 after)

- only do this if actually is superclass

- at runtime need to look up "super" in env
- need to resolve it, so super expr treat "super" like var

^code resolve-super-expr

- interp follows suit

^code begin-superclass-environment (3 before, 1 after)

- if superclass, create env for it and store it

^code end-superclass-environment (2 before, 2 after)

- discard env after class body
- methods will have this env as their closure

- ready to handle super expr itself

^code interpreter-visit-super

- little complex
- two exprs in one - left and right of dot
- first, look up superclass by finding "super" var

- then handle property access
- don't need to handle fields, not inherited
- look up method and bind to this

- need to find "this"
- need to bind receiver
- locals map is keyed off tokens, don't have "this" token
- instead, note that env where this is bound is always just inside one where
  "super" is
- kinda hacky, clox better

- then find method
- returns method with this bound to obj
- can fail if not found

- try boston cream example

### invalid uses of super

- what can go wrong?

```lox
class Eclair {
  cook() {
    super.cook();
    print "Pipe full of crème pâtissière.";
  }
}
```

- no superclass
- at runtime, no env with "super"
- code for interp super assumes superclass is found
- crashes interp

- could check for no super at runtime
- but know statically that Eclair no super, no super clause
- simpler example:

```lox
super.notEvenInAClass();
```

- like this and return, can statically tell where allowed to use
- report static error
- add new "type" of class

^code class-type-subclass (1 before, 1 after)

- use that to track whether surrounding class has superclass or not

- when resolving class decl, if class has super, use new type instead of CLASS

^code set-current-subclass (1 before, 1 after)

- then when resolve super, check type

^code invalid-super (1 before, 1 after)

## Conclusion

- feel like should say something
- reached end of part ii, and first interpreter
- real accomplishment
- xxx lines of code, impl:

    - lexer
    - ast
    - parser
    - static resolution pass
    - interpreter
    - bools, numbers, strings
    - prefix, infix, expressions
    - control flow
    - variables
    - block scope
    - functions
    - classes with ctors, fields, methods
    - inheritance super calls

- did it all ourselves, no dependencies except java stdlib
- few people know how to impl langs
- real accomplishment

- not done yet
- no deps like parser library, but still dep on jvm
- get mem mgmt and object representation for free
- also, interp not fast
- next part, fill in those gaps

<div class="challenges">

## Challenges

1.  Lox only supports *single inheritance* -- a class may have a single
    superclass and that's the only way to reuse methods across classes. Other
    languages have explored a variety of ways to more freely reuse and share
    capabilities between classes: mixins, traits, multiple inheritance, virtual
    inheritance, extension methods, etc.

    If you were to add one to Lox, which would you pick and why? If you're
    feeling courageous (and you should be at this point), go ahead and add it.

1.  In Lox, as in most other object-oriented languages when looking up a method,
    we start at the bottom of the class hierarchy and work our way up -- a
    subclass's method is preferred over a superclass method of the same name.
    This means subclasses override superclass methods. In order to get to the
    superclass method from within an overriding method, you use `super`.

    The language BETA takes an opposite approach. When you call a method, it
    starts at the top of the class hierarchy and works down. A superclass method
    wins over a subclass method. In order to get to the subclass method, the
    superclass method can call `inner`, which is sort of like the inverse of
    `super`. It chains to the next method down the hierarchy. If the superclass
    method doesn't call `inner`, then the subclass has no way of overriding or
    modifying the superclass's behavior.

    Take out Lox's current overriding and `super` behavior and replace it with
    BETA's semantics. In short:

    * When calling a method on a class, prefer the method *highest* on the
      class's inheritance chain.

    * Inside the body of a method, a call to `inner` looks for a method with the
      same name in the nearest subclass along the inheritance chain between the
      class containing the `inner` and the class of `this`. If there is no
      matching method, the `inner` call does nothing.

    For example:

        :::lox
        class Doughnut {
          cook() {
            print "Fry until golden brown.";
            inner();
            print "Place in a nice box."
          }
        }

        class BostonCream < Doughnut {
          cook() {
            print "Pipe full of custard and coat with chocolate.";
          }
        }

        BostonCream().cook();

    This should print:

        :::text
        Fry until golden brown.
        Pipe full of custard and coat with chocolate.
        Place in a nice box.

    Note how `inner()` lets the subclass refine the superclass's method while
    giving the superclass control over *when* the refinement happens.

1.  In the chapter where I introduced Lox, [I challenged you][challenge] to
    come up with a couple of features you think the language is missing. Now
    that you know how to build an interpreter, implement one.

[challenge]: the-lox-language.html#challenges

</div>
