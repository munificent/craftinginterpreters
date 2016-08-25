var a = "outer";
{
  fun foo() {
    print a;
  }

  foo(); // expect: outer
  var a = "inner";
  foo(); // expect: outer
}
