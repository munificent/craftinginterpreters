// Bound methods have identity equality.
class Foo {
  method() {
    print "method";
  }
  other() {
    print "other";
  }
}

var foo = Foo();
var method = foo.method;

// Setting a property shadows the instance method.
foo.method = foo.other;
foo.method(); // expect: other

// The old method handle still points to the original method.
method(); // expect: method
