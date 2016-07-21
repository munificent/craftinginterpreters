class A {
  foo() {
    print "A.foo()";
  }
}

class B < A {}

class C < B {
  foo() {
    print "C.foo()";
    super.foo();
  }
}

C().foo();
// expect: C.foo()
// expect: A.foo()
