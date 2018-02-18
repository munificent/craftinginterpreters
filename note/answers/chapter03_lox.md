1.  I've, uh, written plenty. Look in /test/. Here's another:
    ~~~~
    class List {
      init(data, next) {
        this.data = data;
        this.next = next;
      }

      map(function) {
        var data = function(this.data);
        var next;
        if (this.next != nil) next = this.next.map(function);
        return List(data, next);
      }

      display() {
        var list = this;
        while (list != nil) {
          print(list.data);
          list = list.next;
        }
      }
    }

    var list = List(1, List(2, List(3, List(4, nil))));
    list.display();

    fun double(n) { return n * 2; }
    list = list.map(double);
    list.display();
    ~~~~

2.  Here's a few:

    1.  What happens if you access a global variable in a function before it is
        defined?

    2.  What does it mean to say something is "an error"? Runtime error?
        Compile time?

    3.  What kind of expressions are allowed when a superclass is specified?

    4.  What happens if you declare two classes or functions with the same name?

    5.  Can a class inherit from something that isn't a class?

    6.  Can you reassign to the name that is bound by a class or function
        declaration?

3.  The big ones are:

    1.  Lists/arrays. You can build your own linked lists, but there's no way to
        create a data structure that stores a contiguous series of values that
        can be accessed in constant time in user code. That has to be baked
        into the language or core library.

    2.  Some mechanism for handling runtime errors along the lines of exception
        handling.

    Also:

    3.  No break or continue for loops.

    4.  No switch.
