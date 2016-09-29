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
