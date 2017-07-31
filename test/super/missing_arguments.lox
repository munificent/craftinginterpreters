class Base {
  foo(a, b) {
    print "Base.foo(" + a + ", " + b + ")";
  }
}

class Derived < Base {
  foo() {
    super.foo(1); // expect runtime error: Expected 2 arguments but got 1.
  }
}

Derived().foo();
