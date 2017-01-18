class Base {
  init(a) {
    this.a = a;
  }
}

class Derived < Base {
  init(a, b) {
    super.init(a);
    this.b = b;
  }
}

var derived = Derived("a", "b");
print derived.a; // expect: a
print derived.b; // expect: b
