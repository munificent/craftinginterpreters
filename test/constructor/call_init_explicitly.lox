class Foo {
  init(arg) {
    print "Foo.init(" + arg + ")";
    this.field = "init";
  }
}

var foo = Foo("one"); // expect: Foo.init(one)
foo.field = "field";

var foo2 = foo.init("two"); // expect: Foo.init(two)
print foo2; // expect: Foo instance

// Make sure init() doesn't create a fresh instance.
print foo.field; // expect: init
