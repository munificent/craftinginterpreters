class Foo {
  sayName() {
    print this.name;
  }
}

var foo1 = Foo();
foo1.name = "foo1";

var foo2 = Foo();
foo2.name = "foo2";

// Store the method reference on another object.
foo2.fn = foo1.sayName;
// Still retains original receiver.
foo2.fn(); // expect: foo1
