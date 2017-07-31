class Base {
  foo(a, b) {
    print "Base.foo(" + a + ", " + b + ")";
  }
}

class Derived < Base {
  foo() {
    print "Derived.foo()"; // expect: Derived.foo()
    super.foo("a", "b", "c", "d"); // expect runtime error: Expected 2 arguments but got 4.
  }
}

Derived().foo();
