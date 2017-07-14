“Once in a while you find yourself in an odd situation. You get into it by degrees and in the most natural way but, when you are right in the midst of it, you are suddenly astonished and ask yourself how in the world it all came about.”
― Thor Heyerdahl

--

When talking about early and late binding, use an example like this:

if (false) {
  var a = 1;
  var a = 2; // error here?
}

--

Talk about late and early binding. Motivating example:

    var a = "global";
    fun outer() {
      fun inner() {
        print(a);
      }

      inner();
      var a = "outer";
      inner();
    }

This should print "global" twice, not "global" then "outer".

--

Was aside in statements, moved here:

- [once have closures, even more interesting question about whether redefining
  existing var or defining new one]

    var a = "before";
    fun f() {
      print a;
    }
    var a = "after";
    f(); // ???

  - equivalent prog in scheme prints "after"
  - are redefining existing var
  - var decl is treated exactly like assignment (at top level) if var exists
  - in ml, prints "before"
  - second "var a" introduces new a that is only visible to later code
  - existing code, like f() still sees orig