class A {
  say() {
    print "A";
  }
}

class B < A {
  test() {
    super.say();
  }

  say() {
    print "B";
  }
}

class C < B {
  say() {
    print "C";
  }
}

C().test(); // expect: A
