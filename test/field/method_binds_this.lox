class Foo {
  sayName(a) {
    print this.name;
    print a;
  }
}

var foo1 = Foo();
foo1.name = "foo1";

var foo2 = Foo();
foo2.name = "foo2";

// Store the method reference on another object.
foo2.fn = foo1.sayName;
// Still retains original receiver.
foo2.fn(1);
// expect: foo1
// expect: 1
