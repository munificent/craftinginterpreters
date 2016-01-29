// This is a regression test. When closing upvalues for discarded locals, it
// wouldn't make sure it discarded the upvalue for the correct stack slot.
//
// Here we create two locals that can be closed over, but only the first one
// actually is. When "b" goes out of scope, we need to make sure we don't
// prematurely close "a".
var closure

{
  var a = "a"

  {
    var b = "b"
    closure = Fn.new { a }
    if (false) Fn.new { b }
  }

  System.print(closure.call()) // expect: a
}
