fun f() {
  for (;;) {
    var i = "i";
    fun g() { print i; }
    return g;
  }
}

var h = f();
h(); // expect: i
