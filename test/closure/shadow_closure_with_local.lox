{
  var foo = "closure";
  fun f() {
    {
      print foo; // expect: closure
      var foo = "shadow";
      print foo; // expect: shadow
    }
    print foo; // expect: closure
  }
  f();
}
