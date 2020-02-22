^title Superclasses
^part A Bytecode Virtual Machine

> You can choose your friends but you sho' can't choose your family, an' they're
> still kin to you no matter whether you acknowledge 'em or not, and it makes
> you look right silly when you don't.
>
> <cite>Harper Lee, <em>To Kill a Mockingbird</em></cite>

- last feature of clox
- one more chapter, no new behavior
- two features in this
  - inheriting methods from superclasses
  - calling superclass methods
- half follow jlox
- similar approach for resolving super
- faster impl for inherited method calls

## inheriting methods

- start with method inheritance since simplest
- recall class doesn't need to inherit
- if doesn't, no superclass
- specify superclass using `<` after class name followed by superclass

^code compile-superclass (2 before, 1 after)

- if find less than, should find identifier for class name after
- consume token
- then call `variable()`
- compiles previously consumed token as variable reference
- so will emit code to look up superclass and push onto stack
- most cases superclass is stored in global var, so ends up compiling to
  OP_GET_GLOBAL

- then call `namedVariable()` to load subclass back onto top of stack
- emit new instruction to wire up subclass relation
- like did for method declarations, inheritance is separate instruction that
  imperatively modifies new subclass object

- before get to runtime, one edge case to detect

^code inherit-self (1 before, 1 after)

- class cannot inherit from self
- [interestingly, if didn't detect this error, would get valid code that could
  run and would create cycle in inheritance chain
  coincidentally not actually that harmful in clox
  don't think would cause any infinite loops]

- all for now in compiler

### inheriting at runtime

- new instruction

^code inherit-op (1 before, 1 after)

- no operands
- everything uses is on stack
- so disassemble is simple

^code disassemble-inherit (1 before, 1 after)

- interp is where interesting work happens

^code interpret-inherit (1 before, 2 after)

- top of stack has superclass with new subclass on top

**todo: show stack layout**

- this where implementation different from jlox
- in jlox, each class store pointer to superclass
- when calling method, look on class
- if not found, recurse up superclass chain
- happens at invocation time for each call
- means inherited calls are slower, and farther up inheritance chain, slower
  call is

- approach here much faster
- inherit methods much more literally
- when subclass is created, copies every single method from superclass table
  into own method table

- [sometimes hear called "copy-down inheritance"
  essentially optimization
  like every opt, lang must fit within certain restrictions for opt to apply
  in this case, require that superclass cannot add or change methods after
  subclass created
  if could modify superclass method set after subclass copies them, subclass
  will not pick up changes
  python, ruby, and js do allow class to be "opened" and have methods
  imperatively modified later during exec
  could not do this opt
  lox more restricted]

- means no other runtime support needed to call inherited methods
- already right in class's method table
- just as fast as non-inherited

- what about overrides?
- work too
- since compile superclass clause before class's own methods, execute
  `OP_INHERIT` instr before any `OP_METHOD` ones
- so inherited methods get copied to subclass's table
- then if subclass overrides method, new method just overwrites entry in table
- simple and fast, how i like it

- but not correct
- nothing preventing class from trying to inherit from value not class
- need runtime check for that

^code inherit-non-class (1 before, 2 after)

- if value stored in superclass clause's variable isn't class, runtime error
- all we need for inheriting methods

## storing superclass

- note when added method inheritance loxclass does not actually store point
  to superclass at all
- copies down methods then discards superclass
- fine for method calls where overridden wins

- for super calls need to access method on superclass even when sub table
  replaces
- need to get to superclass somehow

- recall earlier example from jlox

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

- inside body of test method, receiver is instance of C
- call `method()` on that inherited from B
- body of B's `method()` has super call
- should print "A method"
- in other words, in super call, "super" means "superclass of class where this
  method is declared"
- class where method is declared is static property of program
- so super calls statically resolved
- don't need to look up on instance
- instead, more like lexical property of where method containing super call
  lives

- in jlox took advantage of that by storing ref to super class in environment
- as if interpreter turned above program to:

```lox
class A {
  method() {
    print "A method";
  }
}

var Bs_super = A;
class B < A {
  method() {
    print "B method";
  }

  test() {
    runtimeSuperCall(Bs_super, "method");
  }
}

var Cs_super = B;
class C < B {}

C().test();
```

- store ref to each class's superclass in hidden variable
- when perform supercall, look up superclass in that var
- then tell runtime to look up method on that class

- same approach for clox
- but where jlox env, clox stack
- otherwise about same

- after compiling superclass clause, superclass obj on top of stack
- now emit declare slot as local variable

^code superclass-variable (2 before, 2 after)

- create new local scope for var so multiple subclasses in same scope each have
  own var for super
- name "super" because reserved word
- ensures user can't step on name

- when compile `this`, treated `this` as variable and used lexeme of token to
  get name
- don't have `super` token here so instead helper

^code synthetic-token

- makes temp token with given constant string as lexeme
- [don't need to manage mem for lexeme since only use this with c constant
  string]

- since create local scope for `super`, need to end scope at end of class

^code end-superclass-scope (1 before, 2 after)

- only create scope if is superclass clause, so need bool to track
- could use local, but will need to know whether surrounding class is subclass
  later for supercalls, so add field to class compiler

^code has-superclass (3 before, 1 after)

- when first compile class, init false

^code init-has-superclass (1 before, 1 after)

- if see superclass clause, know in subclass

^code set-has-superclass (1 before, 1 after)

- now compiler create local var named `super` to store superclass
- can later use this to look up ref to superclass classobj
- existing machinery for capturing local vars in closures works for this too
- things like super call inside local fn inside method work
- remains is super calls themselves

## super calls

- new syntax
- [last bit of syntax to add to parser!]
- super call start with reserved word

^code table-super (1 before, 1 after)

- pratt parser calls

^code super

- unlike `this`, `super` token itself not standalone expr
- [if it were, what object would it resolve to?]
- instead, dot and identifier after it fundamental part of syntax
- no super expr
- not even actually super call
- really just super access, since can get ref to superclass method and then call
  later

```lox
var closure = super.method();
closure();
```

- so compiler consume dot then identifier for super method being accessed
- [only access methods, fields not inherited, so no "super field"]
- method looked up by name dynamically, so use `identifierConstant()` to store
  method name in chunk const table for runtime

- then emit code to look up and load current instance to top of stack
- even though super method not looked up on this, once found method still
  invoke on this
- receiver will be same instance as in current method
- so load onto stack

- then emit similar code to load superclass obj and push onto top of stack
- finally emit instr to look up given method on superclass and invoke on
  instance
- use index of method name as operand

**todo: show stack**

- before interp, as usual, couple of compile errors to report

^code super-errors (1 before, 1 after)

- can only use `super` call inside class and inside class that has superclass
- if use outside of class decl or in class that doesn't inherit, error

### executing super accesses

- now over runtime
- new instr

^code get-super-op (1 before, 1 after)

- single operand for method name, so dis like other similar ops

^code disassemble-get-super (1 before, 1 after)

- interp is similar to normal property access on instance

^code interpret-get-super (1 before, 2 after)

- as before, read property -- just method -- name from constant table
- then call `bindMethod()` to look up and create ObjBoundMethod that binds to
  current receiver

- one obvious difference is don't first look for field with given name
- no fields for super exprs

- more useful difference is that for normal prop access, got class from
  ObjInstance itself and started lookup there
- here, pop superclass off stack and do method lookup on that
- skips over any methods subclass may override
- otherwise same
- if `bindMethod()` succeeds, pops instance and stores bound method obj on
  stack

### faster super calls

- suffers from same perf problem last chapter in method calls
- every super call heap allocates objboundmethod
- populates fields
- most of time, very next instr is `OP_CALL` which unpacks objboundmethod
  invokes, and discards
- even more true of super calls than method calls since super calls never
  access fields
- only methods, and rarely want to capture and store in closure
- [do need to support capturing to closure though since language allows]

- go ahead and do same optimization
- when compiling super call, if see arg list after ident, emit superinstr
  that combines access and call

^code super-invoke (1 before, 1 after)

- replace existing two lines that emit load super and emit OP_GET_SUPER
- instead, check for paren first
- if found, compile arg list
- then load super and emit new `OP_SUPER_INVOKE` instr for faster invoke
- instr takes two operands -- number of args then index of method name in table

- if no paren, emit same code for super access

- new instr

^code super-invoke-op (1 before, 1 after)

- and dis

^code disassemble-super-invoke (1 before, 1 after)

- has same structure as `OP_INVOKE` so use same helper
- interp

^code interpret-super-invoke (2 before, 1 after)

- very similiar to `OP_INVOKE` mixed with bit of `OP_GET_SUPER`
- read arg count op and look up method name
- then like `OP_GET_SUPER`, pop superclass off stack instead of getting
  instance's own class
- hand off to `invokeFromClass()`
- looks up given method on given class -- this time superclass
- if failed to find, report runtime error
- otherwise, pushes new callframe so refresh interp's local copy of current
  frame

- did it!
- very last feature of lox in clox bytecode vm
- as with jlox, have full-featured language
- rich set of expressions and precedence
- complete set of control flow statements
- variables, functions, closures, classes, fields, methods, inheritance

- even more impressive, have fairly efficient implementation in low-level,
  portable c
- fast enough for real world use
- [comparable to ruby and python]
- and understand exactly how every single piece works
- how values organized in memory
- how dynamic typing rep
- how gc allocates and reclaims memory
- how use vm's stack to minimize heap alloc

- really leveled up your understanding of how programming languages work
- in turn how programming itself works
- even if never write own language, will always have greater clarity on how
  languages do use behave under the hood
- not just taxi driver now, mechanic too

- can stop here
- vm is complete
- built a car
- but if want to have fun tuning it for even greater speed on racetrack, one
  more chapter
- no new features, but add couple of classic optimizations to push even faster

## challenges

- with inheritance, need to ensure instance's fields all fully initialzed so
  obj in valid state according to all class's along inheritance chain
  - easy part is defining `init()` method in each class that calls `super.init()`
    so every class can do init
  - harder part is fields
  - nothing preventing subclass and superclass for accidentally using same
    field name
  - can accidentally step on each other's state and leave in broken state
  - if lox your language, what would do? ok with this problem? change language?
    how?

- copy down inheritance only works in lox because can't add or changes methods
  after class declared
  - ensures subclass's copy never get out of sync
  - other langs do allow mod
  - how would you implement inheritance in clox if superclass methods could
    change after subclass declared?

- in jlox chapter on inheritance, explained beta language approach to overriding
  - repeat here

    In Lox, as in most other object-oriented languages, when looking up a
    method, we start at the bottom of the class hierarchy and work our way up --
    a subclass's method is preferred over a superclass's. In order to get to the
    superclass method from within an overriding method, you use `super`.

    The language [BETA][] takes the [opposite approach][inner]. When you call a
    method, it starts at the *top* of the class hierarchy and works *down*. A
    superclass method wins over a subclass method. In order to get to the
    subclass method, the superclass method can call `inner`, which is sort of
    like the inverse of `super`. It chains to the next method down the
    hierarchy.

    The superclass method controls when and where the subclass is allowed to
    refine its behavior. If the superclass method doesn't call `inner` at all,
    then the subclass has no way of overriding or modifying the superclass's
    behavior.

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
            print "Place in a nice box.";
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

    - this time, because care more about perf, try to design system make
      inner calls efficient too
