class Base {
  foo() {
    print "Base.foo()";
  }
}

class Derived < Base {
  bar() {
    print "Derived.bar()";
    super.foo();
  }
}

Derived().bar();
// expect: Derived.bar()
// expect: Base.foo()
