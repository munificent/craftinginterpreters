^title Classes
^part A Tree-Walk Interpreter

We're through eleven chapters, and the interpreter sitting on your machine is
pretty close to a full-fledged scripting language. It could use a couple of
built-in data structures like lists and maps, and it certainly needs a core
library for file IO, user input etc. But the language itself is adequate. We've
got a little procedural language in the same vein as BASIC, TCL, Scheme (minus
macros), and early versions of Python and Lua.

If this was the 80's, we'd be done. But, today, most popular languages support
"object-oriented programming". Adding that to Lox will give users a familiar set
of features for writing larger programs. Even if you personally don't <span
name="hate">like</span> OOP, this chapter and the next will help you understand
how some language creators design and implement object systems.

<aside name="hate">

If you *really* hate classes, though, you can skip these chapters. They are
fairly separate from the rest of the book. Personally, I find it's good to try
to learn more about the things I despise. It usually turns out my opinion
derived its intensity from ignorance -- reality is rarely so one-sided.

</aside>

### OOP and classes

There are three broad paths to object-oriented programming: classes,
[prototypes][], and <span name="multimethods">[multimethods][]</span>. Classes
came first and are the most popular style. With the rise of JavaScript (and to a
lesser extent [Lua][]), prototypes are more widely known than they used to be.
We'll talk more about the difference below. For Lox, we're taking the well-worn
path and doing classes.

[prototypes]: http://gameprogrammingpatterns.com/prototype.html
[multimethods]: https://en.wikipedia.org/wiki/Multiple_dispatch
[lua]: https://www.lua.org/pil/13.4.1.html

<aside name="multimethods">

Multimethods are the one you're least likely to be familiar with. I'd love to
talk more about them -- I designed [a hobby language][magpie] around them once
and they are super rad -- but there are only so many pages I can fit in. If
you'd like to learn more, take a look a CLOS (the object system in Common Lisp),
Dylan, Julia, and Perl 6.

[magpie]: http://magpie-lang.org/

</aside>

Since you've written about a thousand lines of Java code with me already, I'm
assuming you don't need a detailed introduction to object-orientation. The main
goal is to let users bundle data together with operations that act on it. We do
that by letting users declare a **class** that:

1. Exposes a **constructor** to create and initialize new **instances** of the
   class.

1. Provides a way to store and access **fields** on instances.

1. Defines a set of **methods** shared by all instances of the class that
   operate on their state.

That's about as minimal as you can get. Most object-oriented languages, all the
way back to Simula, also do inheritance to let you share behavior across
*classes*. We'll add that in the next chapter. Even kicking that out, we still
have a lot to get through. This is a big chapter and everything doesn't quite
come together until we have all three of the above pieces, so gather your
stamina.

## Class Declarations

Like we do, we're gonna start with syntax. Classes introduce a new named declaration, so they go as another clause in that grammar rule:

```lox
declaration    → classDecl
               | funDecl
               | varDecl
               | statement ;
```

That branches out to this new rule:

```lox
classDecl      → "class" IDENTIFIER ( "<" IDENTIFIER )? "{" function* "}" ;
```

That's the only *new* grammar. It relies on the same `function` rule we defined
earlier:

```lox
function       → IDENTIFIER "(" parameters? ")" block ;
parameters     → IDENTIFIER ( "," IDENTIFIER )* ;
```

In case you forgot, we also use that for named function declarations. While
those use a `funDecl` rule that requires a preceding `fun` keyword, <span
name="fun">methods</span> do not. In plain English, a class declaration is the `class` keyword, followed by the class's name, then a list of method declarations surrounded by curly braces. Here's one:

<aside name="fun">

Not that I'm trying to say methods aren't fun or anything.

</aside>

```lox
class Breakfast {
  cook() {
    print "Eggs a-fryin'!";
  }

  serve(who) {
    print "Enjoy your breakfast, " + who + ".";
  }
}
```

Like most dynamically typed languages, fields are not explicitly listed in the
class declaration. Instances are loose bags of data and you can freely add
fields to them as you see fit in the middle of normal imperative code.

The new `classDecl` grammar rule gets its own statement AST node:

^code class-ast (1 before, 1 after)

It stores the name and the methods inside the body. We reuse the existing
Stmt.Function class that we use for function declaration AST nodes to represent
method declarations too since they have all the bits of state that we need --
method name, parameter list, and body.

We can parse a class declaration anywhere a named declaration is allowed,
triggered by the leading `class` keyword:

^code match-class (1 before, 1 after)

That calls out to:

^code parse-class-declaration

It's a little bigger than the parsing methods for most of our productions, but
it roughly corresponds to the grammar. We've already consumed the `class`
keyword, so we look for the expected class name next, followed by the opening
curly brace for the class body. Then we keep parsing method declarations until
we hit the closing brace. Each method declaration is parsed by a call to
`function()`, which we defined back in the [chapter where functions were
introduced][functions].

[functions]: functions.html

Like we do in any open-ended loop in the parser, we also check for hitting the
end of the file. That won't happen in correct code since a class should have a
closing brace at the end, but it ensures the parser doesn't get stuck in an
infinite loop if the user has a syntax error and forgot to correctly end the
class body.

Then we wrap the name and list of methods up in a Stmt.Class node and we're
done. Now that we have that resolving pass, before we get to interpreting the
new node, we need to resolve it first:

^code resolver-visit-class

We aren't going to worry about resolving the methods themselves just yet, so for
now all we need to do is declare the class name itself. It's not common to
declare a class as a local variable, but Lox permits it, so we need to handle it
correctly.

Now we can interpret it:

^code interpreter-visit-class

This looks similar to how we interpret function declarations. We declare the
class's name in the current environment. Then we turn the class *syntax node*
into a LoxClass, the *runtime* representation of a class.

Finally, we circle back and store the class object in the variable we already
declared. As with function declarations, that two-stage variable binding process
allows classes to refer to themselves inside the bodies of methods.

It's going to grow throughout this chapter, but the first draft of our class's
runtime form looks like this:

^code lox-class

Literally a wrapper around the name. We don't even store the methods yet. Not
super useful, but it does have a `toString()` method so we can write a little
script and test that class objects are actually being parsed and executed:

```lox
class DevonshireCream {
  serveOn() {
    return "Scones";
  }
}

print DevonshireCream; // Prints "DevonshireCream".
```

## Creating Instances

We have classes, but they don't *do* anything yet. Lox doesn't have "static"
methods that you can call right on the class itself, so without actual
instances, classes are pretty useless. We'll fix that next.

While some aspects of object-orientation are fairly standard across languages,
the way you create new instances isn't one of them. Some, like C++ and Java have
a `new` keyword dedicated to birthing a new object. Python has you "call" the
class itself like a function. (JavaScript, ever weird, sort of does both of
these.) Ruby, following Smalltalk, places construction as a method on the class
object itself, a <span name="turtles">recursively</span> graceful approach.

<aside name="turtles">

In Smalltalk, even *classes* are created by calling methods on an existing
object, usually the desired superclass. It's sort of a turtles-all-the-way down
thing. It ultimately bottoms out on a few magical classes like Object and
Metaclass that the runtime conjures into being *ex nihilo*.

</aside>

I took a minimal approach with Lox. We already have class objects, and we
already have function calls, so we'll use call expressions on class objects to
create new instances. It's as if a class is a factory function that generates
instances of itself.

This feels elegant to me, and also spares us the need to introduce syntax like
`new`. Thus, we can skip past the front end right into the runtime.

Right now, if you try this:

```lox
class Bagel {}
Bagel();
```

You get a runtime error. `visitCallExpr()` checks to see if the called object
implements `LoxCallable` and reports an error since LoxClass doesn't. At least,
not yet:

^code lox-class-callable (2 before, 1 after)

That interface requires two methods. The interesting one is `call()`:

^code lox-class-call

Now when you "call" a class, it instantiates a new LoxInstance for the called
class and returns it. LoxCallable also requires one other helper method:

^code lox-class-arity

This is how `visitCallExpr()` checks to see if you passed the right number of
arguments to it. For now, we'll say that you can't pass any. When we get to
user-defined constructors, we'll revisit this.

The remaining piece is LoxInstance, our runtime representation of an instance of
a Lox class. Here's a first stab at it:

^code lox-instance

Like LoxClass, it's pretty bare bones, but we're only getting started. If you
want to give it a try, here's a script to run:

```lox
class Bagel {}
var bagel = Bagel();
print bagel; // Prints "Bagel instance".
```

## Properties on Instances

We have instances, so we should make them useful. We're at a fork in the road.
We could add behavior first -- methods -- or we could start with state. We're
going to take the latter because, as we'll see, the two get entangled in an
interesting way and it will be easier to make sense of them if we get
properties working first.

Lox follows JavaScript and Python in how it handles state. Every instance is a
loose bag of named data. Methods on the instance's class can access and modify
those properties, but so can <span name="outside">outside</span> code. As in
those languages, properties are accessed using a `.` syntax:

<aside name="outside">

Allowing code outside of the class to directly modify an object's state goes
against the object-oriented credo that a class *encapsulates* state. Some
languages take a more principled state. In Smalltalk, there is no syntax for
directly modifying the state of an object. Instead, fields are accessed using
simple identifiers, essentially variables that are only in scope inside the
class's methods. Ruby uses `@` followed by a name to access a field in an
object. That syntax is only meaningful inside a method and always accesses state
on the current object.

Lox, for better or worse, isn't quite so pious about its OOP faith.

</aside>

```lox
someObject.someProperty
```

An expression followed by a `.` and an identifier reads the property with that
name from the object the expression evaluates to. That `.` is as high precedence
as the parentheses in a function call expression, so we slot it into the grammar
by replacing the existing `call` rule with:

```lox
call → primary ( "(" arguments? ")" | "." IDENTIFIER )* ;
```

After a primary expression, we allow a series of any mixture of parenthesized
calls and dotted property accesses. "Property access" is a mouthful, so from
here on out, we'll call these "get expressions".

### Get expressions

The syntax tree node is:

^code get-ast (1 before, 1 after)

Following the grammar, the new parsing code goes in our existing `call()`
method:

^code parse-property (3 before, 4 after)

The outer while loop there corresponds to the `*` in the grammar rule. We zip
along the tokens building up a chain of calls and gets as we find them.

**todo: illustrate**

That feeds into the resolver:

^code resolver-visit-get

OK, not much to that. Since properties are themselves looked up dynamically,
they don't get resolved. However, we do need to recurse into the expresion to
the left of the dot. The actual work happens in the interpreter:

^code interpreter-visit-get

First, we evaluate the expression whose property is being accessed. In Lox, only
instances of classes have properties. If the object is a primitive type like a
number, trying to access a property on it is a runtime error.

If the object *is* a LoxInstance, then we ask it for the value of the given
property. LoxInstance needs some way to actually store data. We do that by
giving it a map:

^code lox-instance-fields (1 before, 2 after)

Each key in the map is a property name whose value is the property's value.
Using that, we can implement `get()` to look up a property on an instance:

^code lox-instance-get-property

<aside name="hidden">

Doing a hash table lookup for every field access is fast enough for many
language implementations, but not ideal. High performance VMs for languages like
JavaScript have sophisticated optimizations like "[hidden classes][]" to avoid
that when possible by statically determining the fields an object will end up
having and compiling field access code to take advantage of that.

Interestingly, many of the optimizations invented to make highly dynamic
languages fast rest on the observation that -- even in those languages -- most
code is fairly static in terms of the types of objects it works with and their
shapes.

[hidden classes]: http://richardartoul.github.io/jekyll/update/2015/04/26/hidden-classes.html

</aside>

An interesting edge case we need to handle is what happens if the instance
doesn't *have* a property with the given name. We could silently return some
dummy value, probably `nil`. But my feeling is that that tends to mask bugs more
often than it does something useful. Instead, we'll make it a runtime error.

So the first thing we do is see if the instance actually has a field with the
given name. Only then does it return it. Otherwise, it raises an error.

Note how I switched from talking about "properties" to "fields". There is a
subtle difference between the two. Fields are named bits of state stored
directly in an instance. Properties are the named, uh, things, that you can
access on an instance. Every field is a property, but as we'll see <span
name="foreshadowing">later</span>, not every property is a field.

<aside name="foreshadowing">

Ooh, foreshadowing. Spooky!

</aside>

In theory, we can now read properties on objects. But since there's no way to
actually stuff any state into an instance, there are no fields to access. Before
we can read, we must write.

### Set expressions

Setters use the same syntax as getters, except they appear on the left side of
an assignment:

```lox
someObject.someProperty = value;
```

In order to keep grammar up to date, we extend rule for assignment to allow
dotted identifiers on the left-hand side:

```lox
assignment → ( call "." )? IDENTIFIER "=" assignment
           | logic_or;
```

We don't need to explicitly allow a chain of them. The reference to `call`
allows any high precedence expression before the last `.`, including a series of other `.`, as in:

```lox
a.b.c.d = e;
```

**todo: illustrate mapped to grammar?**

Note here that only the *last* part, the `.d` is the *setter*. The `.b` and `.c`
are both get expressions.

Like we have two separate AST nodes for variable access and variable assignment,
we need a second node for property assignment:

^code set-ast (1 before, 1 after)

In case you don't remember, the way we handle assignment in the parser is a
little funny. We can't easily tell that a series of tokens is an L-value in an
assignment until we hit the `=`. Now that our assignment grammar rule has `call`
on the left side, which can expand to arbitrarily large expressions, that can be
hard to correct anticipate.

Instead, the trick we do is to parse the left hand side as a normal R-value
expression. Then, when we stumble onto the equals sign, we take the expression
we already parsed and transform it into the correct syntax tree node for the
assignment.

Now that we have setters, we'll add another clause to that transformation to
handle turning an Expr.Get expression on the left into the corresponding
Expr.Set:

^code assign-set (1 before, 1 after)

That's our syntax. We push that through into the resolver:

^code resolver-visit-set

Again, like Expr.Get, the property itself is dynamically evaluated, so there's
nothing to resolve there. All we need to do is recurse into the two
subexpressions of Expr.Set, the object whose property is being set, and the
value it's set to.

That leads us to the interpreter:

^code interpreter-visit-set

We evaluate the object that is having the property set on and we check to see if
it's a LoxInstance. If not, that's a runtime error. <span
name="order">Otherwise</span>, we evaluate the value being set and store it on
the instance.

<aside name="order">

This is another semantic edge case. There are three distinct operations:

1. Evaluate the object.
2. Raise a runtime error if it's not an instance of a class.
3. Evaluate the value.

The order that those are performed could be user visible, which means we need to
carefully specify it and ensure our implementations do these in the same order.
Otherwise an implementation that, say, evaluated the value before the object
could break a user's program.

**todo: overlap**

</aside>

That relies on:

^code lox-instance-set-property

No real magic here. We go straight to the Java map in the instance where fields
are stored.

## Methods on Classes

You can create instances of classes and stuff data into them. But the class
itself doesn't really *do* anything. Instances are just maps and all instances
are more or less the same. To make them feel like instances *of classes*, we
need behavior -- methods. Finally, we get to use the method declarations our
parser already handles.

We also don't need to add any new parser support for method *calls*. We already
have `.` (getters) and `()` (function calls). A "method call" is simply the
combination of those two expressions:

```lox
object.method(argument);
```

That raises an interesting question. What happens when those two expressions are
pulled apart? Assuming `method` is a method on the class of `object` and not a
field on the instance, what does this do:

```lox
var m = object.method;
m(argument);
```

This program "looks up" the method and stores the result -- whatever that is --
in a variable and then calls that object later. Is this allowed? Can you treat a
method like it's a function on the instance?

Can you go the other way?

```lox
class Box {}

fun notMethod(argument) {
  print "called function with " + argument;
}

var box = Box();
box.function = notMethod;
box.function("argument");
```

This program creates an instance and then stores a function in a field on it.
Then it calls that function using the same syntax as a method call. Does that
work?

Different languages take different approaches here. One could write a treatise
on it. For Lox, we'll say the answer to both of these is that, yes, it does
work. We have a couple of reasons to justify that.

For the second example, calling a function stored in a field, we want to support
that because first class functions are useful and storing them in fields is a
perfectly normal thing to do.

The first example is more complex. One motivation is because users generally
expect to be able to hoist a subexpression out into a local variable without
changing the meaning of the program. You can take this:

```lox
var average = (3 + 4) / 2;
```

And turn it into this:

```lox
var sum = 3 + 4;
var average = sum / 2;
```

And it does the same thing. Likewise, you can hoist the method *lookup* part of
a method call out into a variable and then use that to call it later. This
requires us to figure out what the *thing* you get when you look up a method is
-- the runtime representation of a grabbed-but-not-yet-invoked method.

In particular, we need to think about stuff like this:

```lox
class Person {
  sayName() {
    print this.name;
  }
}

var jane = Person();
jane.name "jane";

var method = jane.sayName;
method(); // ?
```

If you "pull off" a reference to a method on some instance and call it later,
does it "remember" the instance it was pulled off from? Does `this` inside the
method still refer to that original instance?

Here's a more pathological example to wrap your head around:

```lox
class Person {
  sayName() {
    print this.name;
  }
}

var jane = Person();
jane.name "jane";

var bill = Person();
bill.name = "bill";

bill.sayName = jane.sayName;
bill.sayName(); // ?
```

Does that last line print "bill" because that's the instance that we *called*
the method through, or "jane" because it's the instance where we first grabbed
the method?

Equivalent code in Lua and JavaScript would print "bill". Those languages don't
really have a notion of "methods". Everything is sort of functions in fields, so
it's not clear that `jane` "owns" `sayName` any more than `bill` does.

In Lox, though, we do have real class syntax so we don't know which
callable-things-on-instances are methods from its class versus functions stored
in its fields. Thus, like Python, C#, and others, we will have methods "bind"
`this` to the original instance when the method is first grabbed. Python calls
these "bound methods".

In practice, that's usually what you want. If you take a reference to a method
on some object so you can use it as a callback later, you want that call to
remember the instance it belonged to, even if that callback happens to be stored
in a field in some other random object.

OK, that's a lot of semantics to load into your head. Forget about the edge
cases for a bit. We'll get back to those. For now, let's get basic method calls
working. We're already parsing the method declarations inside the class body, so
the next step is to resolve them:

^code resolve-methods (1 before, 1 after)

<aside name="local">

Storing the function type in a local variable is pointless right now, but we'll
expand this code before too long and it will make more sense.

</aside>

We iterate through the methods in the class body and call the
`resolveFunction()` method we wrote for handling function declarations already.
The only difference is that we pass in a new FunctionType enum value:

^code function-type-method (1 before, 1 after)

That's going to be important when we resolve `this` expressions. For now, don't
worry about it. The interesting stuff now is in the interpreter:

^code interpret-methods (1 before, 1 after)

When we interpreter a class declaration statement, we turn the syntactic
representation of the class -- its AST node -- into its runtime representation.
Now, we need to do that for the methods contained in the class as well. Each
method declaration blossoms into a LoxFunction object.

We take all of those and wrap them up into a map, keyed by the method names.
That gets passed into LoxClass:

^code lox-class-methods (1 before, 3 after)

Where an instance stores state, the class itself stores behavior. LoxInstance
has its map of fields. LoxClass gets a map of methods.

Those methods come into play when a program looks up a property on an instance.
This is where the distinction between "field" and "property" comes into play.
When accessing a property, you might get a field -- a bit of state stored on the
instance -- or you could hit a method defined on the instance's class:

^code lox-instance-get-method (5 before, 2 after)

When looking up a property on an instance, if we don't <span
name="shadow">find</span> a matching field, we look for a method with that name
on the instance's class. If found, we return that. It's looked up using:

<aside name="shadow">

This implies that fields shadow methods, a subtle but important semantic point.

</aside>

^code lox-class-find-method

You can probably guess this method is going to get more interesting over time.
We pass in the instance, but we aren't using it yet. For now, a simple map
lookup on the class's method table is enough to get us started.

Give it a try:

<span name="crunch"></span>

```lox
class Bacon {
  eat() {
    print "Crunch crunch crunch!";
  }
}

Bacon().eat(); // Prints "Crunch crunch crunch!".
```

<aside name="crunch">

Apologies if you prefer chewy bacon over crunchy. Feel free to adjust the script
to your taste.

</aside>

## This

We can both behavior and state on objects, but they aren't tied together yet.
Inside a method, there is no way to access the fields of the "current" object --
the instance that the method was called on, nor can you call other methods on
that same object.

Languages vary in how they handle this. Most statically-typed OOP languages like
C++, Java, and C# allow you to refer to fields and methods on the current object
implicitly using bare identifiers and function call syntax. That's a little more
awkward and error-prone in a dynamic language like Lox -- what if there is a
global variable or function with the same name as a field or method on the
current object?

To sidestep that, we'll take the same approach as JavaScript and Python and
require property access on the current object to be explicitly marked by
referring to it by <span name="i">name</span>. Common names for this special
concept are "self" (Smalltalk, Swift, Ruby), "this" (C++, Java, C#) and "me"
(Visual BASIC). Python by convention uses "self", but you can use whatever name
your like.

<aside name="i">

"I" would have been a great choice, but using "i" for loop variables predates
OOP and goes all the way back to Fortran. We are victims of the incidental
choices of our forebears.

</aside>

For Lox, since we generally hew to Java-ish style, we'll do "this". Inside a
method body, `this` will be an expression that evaluates to the instance that
the method was called on. Or, more specifically, since methods are accessed and
then invoked as two steps, it will refer to the object that the method was
*accessed* from.

The key challenge is how the method grabs a hold of the current instance when
the method is looked up and hangs onto it so that it can be found later when the
method object is actually called. The runtime representation of a method is a
LoxFunction, and that function is shared across all instances of the class.
However, when that function is invoked, each invocation should see a `this` that
refers to some specific, likely different instance.

It's almost like `this` is a special hidden argument that gets passed to the
method when you call it. That analogy breaks down, though, since `this` needs to
be "passed in" and pinned down even before the method is actually *called*.

But that intuition hints at a promising approach. We already have environments
and closures that let a function hang onto some state as long its needs it. If
we could stuff `this` into that surrounding environment, the function would be
able to find it when called later.

**todo: illustrate**

The trick is that we "pass in this" when the method is looked up on the `.`
expression. That returns a closure for the method that binds `this` to the
current receiver. When the closure is later called, any uses of `this` inside
its body will correctly refer to the original bound receiver.

Reusing our existing machinery for managing environments to handle `this` also
takes care of lots of interesting cases where methods and functions nest and
interact, like:

```lox
class Outer {
  method() {
    class Inner {
      method() {
        // Inner this should shadow outer this.
        print this;
      }
    }
  }
}
```

Since class declarations are statements, they can nest (though realistic code
rarely does this). When that happens, we need to make sure the two different
uses of `this` shadow each other correctly.

A more useful tricky example is:

```lox
class Thing {
  getCallback() {
    fun localFunction() {
      print this;
    }

    return localFunction;
  }
}

var callback = Thing().getCallback();
callback();
```

In, say, JavaScript, it's common to want to return a callback from inside a
method. That callback may want to hang onto and retain access to the original
object the method was associated with. Our existing support for closures and
environment chains should do all this correctly. The first step is adding new
syntax for `this`:

^code this-ast (1 before, 1 after)

We could treat `this` as simply an identifier that is handled specially in a
couple of places. In that case, it would parse as a variable access expression.
That works for some uses, but would also allow bad code like:

```lox
var this = "wat"; // Declare a variable with that name.
this = // Assign to it.
fun this() {} // Use it as a function name?!
```

The simplest way to rule all of that out is to make it a reserved word so that
it can't be parsed as an identier. Then we add a new syntax node for an
expression that *accesses* `this`, which is the only thing we want to allow you
to do with it.

Parsing is simple since it's a single token:

^code parse-this (2 before, 2 after)

You can start to see how `this` works like a variable when we get to the
resolver:

^code resolver-visit-this

We resolve it exactly like any other local variable using "this" as the name for
the "variable". Of course, that's not going to work right now, because "this"
*isn't* declared in any scope. Let's fix that over in `visitClassStmt()`:

^code resolver-begin-this-scope (2 before, 1 after)

Before we step in and start resolving the method bodies, we push a new scope and
define "this" in it as if it were a variable. Then, when we're done, we discard
it:

^code resolver-end-this-scope (2 before, 1 after)

Now, whenever a `this` expression is encountered (at least inside a method) it
will resolve to a "local variable" defined in an implicit scope just outside of
the block for the method body.

**todo: illustrate**

The resolver has a new *scope* for `this`, so the interpreter needs to create a
corresponding environment for it at runtime. That happens at the point that the
method is looked up on the instance, so we replace the previous line of code
that simply returned the LoxFunction for the method with this:

^code lox-class-find-method-bind (1 before, 1 after)

Note the new call to `bind()`. This is where we use the instance that we're
already passing to `findMethod()`:

^code bind-instance

There isn't much to this. We create a new environment whose parent is the
function's original closure environment. That inserts the environment between
the environment surrounding the class -- remember that class declarations can be
nested inside other code -- and the inner environment for the method body that
will get created when the method is called. In other words, it gives a new
even-more-inner closure.

Then we declare "this" as a variable in that environment and bind it to the
given instance, the same instance that the method is being accessed from. *Et
voila*, the returned LoxFunction now carries around its own little persistent
world where "this" is bound to the object.

**todo: spelling**

The remaining loose end is interpreting those `this` expressions. Much like they
are resolved, the interpeting code is the same as how we handle variables:

^code interpreter-visit-this

Go ahead and give it a try:

```lox
class Cake {
  taste() {
    print "The " + this.flavor + " cake is delicious!";
  }
}

var cake = Cake();
cake.flavor = "German chocolate";

var taste = cake.taste;
taste(); // Prints "The German chocolate cake is delicious!".
```

Virtual high fives all-around. Our interpreter handles `this` inside methods
even in all of the weird ways it can interact with nested classes, functions
inside methods, handles to methods, etc.

But what happens if you try to use `this` *outside* of a method? What about:

```lox
print this;
```

Or:

```lox
fun notAMethod() {
  print this;
}
```

### This outside of methods

There is no instance if you're not in a method, so `this` isn't defined. We
could return some default value like `nil` or make it a runtime error, but the
user has clearly made a mistake. The sooner they find and fix that mistake, the
happier they'll be.

Our resolver pass is a great place to detect this error statically. It already
detects return statements outside of functions. We'll do something similar for
`this`. In the vein of our existing FunctionType enum, we define a new ClassType
one:

^code class-type (1 before, 1 after)

Yeah, it could be a simple Boolean. When we get to inheritance, it will get a
third enum value, hence the enum right now. As the name so helpfully implies,
`currentClass` keeps track of whether the current syntax node is within a class
declaration while the resolver is traversing the tree.

We set it before resolving the method bodies in a class declaration:

^code set-current-class (1 before, 2 after)

As with `currentFunction`, we store the previous value of the field in a local
variable. This lets us piggyback onto the JVM to keep a stack of `currentClass`
values. That way we don't lose track if one class is nested inside another.

Once the methods have been resolved, we "pop" that stack by restoring the
previous value:

^code restore-current-class (3 before, 1 after)

When we resolve a `this` expression, that gives us the bit of data we need to
report an error if the expression doesn't occur nestled inside a method body:

^code this-outside-of-class (1 before, 1 after)

Cool, right? We are almost done!

## Constructors and Initializers

We can do almost everything with classes now, and all that remains as we near
the end of the chapter is the beginning. Methods and fields let us encapsulate
state and behavior together so that an object always stays in a valid
configuration. But how do we ensure a brand object is created in a meaningful
state?

For that, we need constructors. I find them one of the trickiest parts of a
language to design and if you peer closely at other most languages, you'll see
<span name="cracks">cracks</span> around object construction where the seams of
the design don't quite hang together perfectly. Maybe there's something messy
about the moment of birth that defies elegance.

<aside name="cracks">

A few examples: In C++, constructors, field initializers and exceptions interact
in ways only a few experts truly understand, and they have the scars to prove
it. In Java and C#, final fields can be observed in their primordial
uninitialized state if you try hard enough.

**todo: more**

</aside>

For Lox, we'll do our best and try to keep it simple. "Constructing" an object
is actually a pair of operations:

*   The runtime <span name="allocate">*allocates*</span> the memory required for
    a fresh instance. In most languages, this operation is at a fundamental
    level beneath what user code is able to access.

    <aside name="allocate">

    C++ and "placement new" is a rare example where the bowels of allocation are
    laid bare for the programmer to poke and prod.

    **todo: link**

    </aside>

*   Then a user-provided chunk of code is called which *initializes* the
    unformed object.

The latter is what we tend to think of when we think of "constructors", but the
language itself has usually done some ground work for us first before we get to
that point. In fact, our Lox interpreter already has that covered. That's the
point where it creates a new LoxInstance object.

We'll do the remaining part -- user-defined initialization -- now. Languages
have a variety of notations for the chunk of code that sets up a new object for
a class. C++, Java, and C# use a method whose name matches the class name. Ruby
and Python call it `init()`. That's nice and short, so we'll do that.

The basic idea is that when you call a class, the interpreter will allocate a
new instance. Then, if the class has defined a method named "init", it calls
that. Any arguments passed when calling the class are forwarded on to this
initializer.

Unlike, "this", we are not going to make "init" a reserved word. It feels a
little too useful to users to take out of their hands, and allowing them to use
it doesn't open up the same pitfalls that treating "this" like an identifier
does.

I'm rambling. Let's code:

^code lox-class-call-initializer (1 before, 1 after)

When a class is called, after the LoxInstance is created, we look for an "init"
method. If we find one, we immediately bind and invoke it just like a normal
method call. The argument list is passed along.

That argument list means we also need to tweak how a class declares its arity:

^code lox-initializer-arity (1 before, 1 after)

If there is an initializer, that method's arity determines the class's. Note
that we still support classes that do *not* define an initializer. Like other
languages, it's a nice convenience to not require users to define one if the
class doesn't need any special behavior when an instance is birthed.

As ever, exploring this new semantic territory rustles up a few weird creatures.
Consider:

```lox
class Foo {
  init() {
    print this;
  }
}

var foo = Foo();
print foo.init();
```

Can you re-initialize an object by directly calling its `init()` method? If you
do, what does it return? A <span name="compromise">reasonable</span> answer
would be to have it return `nil` when called directly, because that's what it
appears the body does. However -- and I usually dislike compromising to satisfy
the implementation -- it will make clox's implementation of constructors much
easier if we say that `init()` methods implicitly return `this`, even when
directly called.

<aside name="compromise">

Maybe "dislike" is too strong a claim. It's reasonable to have the constraints
and resources of your implementation affect the design of the language. There
are only so many hours in the day and if a cut corner here or there lets you get
more features to users in less time, it may very well be a net win for their
happiness and productivity. The trick is figuring out *which* corners to cut
that won't cause your future self to curse your short-sightedness.

</aside>

In order to keep jlox compatible with that, we'll add a little special case
code in LoxFunction:

^code return-this (2 before, 1 after)

If the function is an initializer, we override the actual return value and
forcibly return "this". That relies on a new `isInitializer` field:

^code is-initializer-field (1 before, 2 after)

We can't simply see if the name of the LoxFunction is "init" because the user
could have defined a *function* with that name. In that case, there *is* no
`this` to return.

Instead, we'll track whether the LoxFunction represents an initializer method.
That means we need to go back and fix up every place where we create a
LoxFunction:

^code construct-function (1 before, 1 after)

For actual functions, `isInitializer` is always false. For methods, we check the
name:

^code interpreter-method-initializer (1 before, 1 after)

And then in `bind()` where we create the closure that binds `this` to a method,
we pass along the original method's value:

^code lox-function-bind-with-initializer (1 before, 1 after)

We aren't out of the woods yet. If an `init()` method in a class always returns
`this`, what happens if the user tries to return something else? What about:

```lox
class Foo {
  init() {
    return "something else";
  }
}
```

It's definitely not going to do what they want, so we may as well make it a
static error. Back in the resolver, we add another case to FunctionType:

^code function-type-initializer (1 before, 1 after)

When resolving a method, we look at the name to determine if we're resolving an
initializer or some other normal method:

^code resolver-initializer-type (1 before, 1 after)

And finally we can check that field to make it an error to have a return
statement with a value inside an `init()` method:

^code return-in-initializer (1 before, 1 after)

Phew! That was a real task but our reward is that our little interpreter has
grown an entire programming paradigm. Classes, methods, fields, `this`, and
constructors. If I didn't know better, I'd suspect we'd made some giant
enterprise language.

<div class="challenges">

## Challenges

1.  Methods are available on instances, but there is no way to define "static"
    methods that can be called directly on the class object itself. Add support
    for "class methods" defined inside a class body using a preceding `class`
    keyword, like so:

        :::lox
        class Math {
          class square(n) {
            return n * n;
          }
        }

        print Math.square(3); // Prints "9".

    You can solve this however you like, but the "metaclasses" used by Smalltalk
    and Ruby will give you a simple, elegant solution. *Hint: Make LoxClass
    extend LoxInstance and go from there.*

2.  Most modern languages let you define "getters" and "setters" -- members on
    a class that appear to be field reads and writes but that actually perform
    user-defined computation. Extend Lox to support getter methods. These are
    declared without a parameter list. The body of the getter is executed when
    a property with that name is accessed:

        :::lox
        class Circle {
          init(radius) {
            this.radius = radius;
          }

          area {
            return 3.141592653 * this.radius * this.radius;
          }
        }

        var circle = Circle(4);
        print circle.area; // Prints roughly "50.2655".

3.  Python and JavaScript allow you to freely access the fields on an object
    from outside of the methods on that object. Ruby and Smalltalk encapsulate
    instance state. Only methods on the class can access the raw fields, and it
    is up to the class to decide which state is exposed using getters and
    setters. Most statically typed languages offer access control modifiers
    like `private` and `public` to explicitly control on a per-member basis
    which parts of a class are externally accesible.

    What are the trade-offs between these approaches and why might a language
    might prefer one or the other?

</div>

<div class="design-note">

## Design Note: Prototypes and Power

In this chapter, we introduced two new runtime entities, LoxClass and
LoxInstance. The former is where behavior for objects lives and the latter is
for state. What if LoxInstance allowed defining methods directly on a single
object? In that case, we wouldn't need LoxClass at all. LoxInstance would be a
complete package for defining the behavior and state of an object.

We'd still want some way to reuse behavior across multiple instances. If one
LoxInstance could "inherit from" another one (let's call it "delegate to"), that
would cover that too. The end result would be a simpler runtime with only a
single internal construct, LoxInstance.

At the user level, they would model their program as a sea of objects, some of
which delegate to each other to reflect commonality. There are no longer
classes. Instead, individual objects that are delegated to represent "canonical"
or "prototypical" objects that many others are similar to.

That's where the name "prototypes" comes from for this paradigm. It was invented
by David Ungar and Randall Smith in a language called [Self][]. They followed
the above mental exercise of seeing just how simple they could make the way
objects are modeled.

Prototypes were an academic curiosity for a long time, a fascinating one that
generated a lot of interesting research but didn't make much of a dent in the
larger world of programming. That is until this guy working on a weird little
domain-specific scripting language to be embedded in a larger application took
inspiration from it. That person was Brendan Eich, and the little app scripting
language was JavaScript. Many (many) <span name="words">words</span> have been
written about prototypes in JavaScript. Whether that shows that prototypes are
brilliant or merely confusing -- or both! -- is an open question.

<aside name="words">

Including [more than a handful][prototypes] by yours truly.

</aside>

[prototypes]: http://gameprogrammingpatterns.com/prototype.html
[self]: http://www.selflanguage.org/

I won't get into whether or not I think prototypes are a good idea for a
language. I've made languages that are prototypal and class-based, and my
opinions of both are complex. What I'm interested in discussing here is the role
of *simplicity* in a language.

Prototypes are simpler than classes -- less code for the language implementer to
write, and fewer concepts for the user to learn and understand. Does that make
them better? We language nerds have a tendency to fetishize simplicity. The idea
is that a simpler system is easier for users since they have to invest less in
order to reap the rewards of the language.

Personally, I think simplicity is only part of the equation. What we really to
give the user is *power*, which I define as:

    power = breadth × ease ÷ complexity

None of these are precise numeric measures. I'm using math as analogy here, not
actual quantification. "Breadth" is the range of different things the language
lets you express. C has a lot of breadth -- it's been used for everything from
operating systems to user applications to games. Domain-specific languages like
AppleScript and Matlab have less breath.

"Ease" is how much effort it takes to make the language do what you want.
"Usability" might be another good term for this, though that term carries more
baggage than I want to bring in right now. Unless what you're trying to express
is the low-level memory shenanigans C excells in, C generally feels like a
low-ease language -- even basic things like string manipulation are a chore.
Programming in C can feel like building the Eiffel Tower out of matchsticks.
Modern "high-level" languages like C# and Swift let you get more done in a day's
work and have greater ease.

"Complexity" is how big and complex the language is (and its attendant runtime,
core libraries, tools, and ecosystem, etc.). You can think of it as how long is
the language spec document is. It's how much the user has to load into their
wetware before they can be productive in the system. It is the antonym to
simplicity.

Reducing complexity *does* increase power. The smaller the denominator, the
larger the resulting value, so our intuition that simplicity is good is valid.
However, when reducing complexity, we must take care not to sacrifice breadth or
ease in the process, or the total power may end up going down. Java would be a
strictly simpler language if it removed strings, but it probably wouldn't handle
text manipulation tasks well, nor would it be as easy to get things done.

The art, then is finding *accidental* complexity that can be removed. Language
features and interactions that don't "carry their weight" by increasing the
breadth or ease of using the language. Candidates for the cutting room floor are
features that aren't used by many users or interact poorly with other features.

When it comes to classes, my impression is that they end up a net positive when
it comes to power. I've seen (and written) a lot of code in prototypal
languages. One of the first things most users do is reinvent classes at the user
level. There seems to be a strong tendency to express programs in terms of
categories of objects.

If that's what the user wants to express, then baking classes into the language
increases the ease of doing that. If they want that frequently enough, the
overall power of the language may go up, even after spending the complexity cost
required to add them.

</div>
