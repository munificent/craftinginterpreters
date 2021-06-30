> One has no right to love or hate anything if one has not acquired a thorough
> knowledge of its nature. Great love springs from great knowledge of the
> beloved object, and if you know it but little you will be able to love it only
> a little or not at all.
>
> <cite>Leonardo da Vinci</cite>

We're eleven chapters in, and the interpreter sitting on your machine is nearly
a complete scripting language. It could use a couple of built-in data structures
like lists and maps, and it certainly needs a core library for file I/O, user
input, etc. But the language itself is sufficient. We've got a little procedural
language in the same vein as BASIC, Tcl, Scheme (minus macros), and early
versions of Python and Lua.

If this were the '80s, we'd stop here. But today, many popular languages support
"object-oriented programming". Adding that to Lox will give users a familiar set
of tools for writing larger programs. Even if you personally don't <span
name="hate">like</span> OOP, this chapter and [the next][inheritance] will help
you understand how others design and build object systems.

[inheritance]: inheritance.html

<aside name="hate">

If you *really* hate classes, though, you can skip these two chapters. They are
fairly isolated from the rest of the book. Personally, I find it's good to learn
more about the things I dislike. Things look simple at a distance, but as I get
closer, details emerge and I gain a more nuanced perspective.

</aside>

## OOP and Classes

There are three broad paths to object-oriented programming: classes,
[prototypes][], and <span name="multimethods">[multimethods][]</span>. Classes
came first and are the most popular style. With the rise of JavaScript (and to a
lesser extent [Lua][]), prototypes are more widely known than they used to be.
I'll talk more about those [later][]. For Lox, we're taking the, ahem, classic
approach.

[prototypes]: http://gameprogrammingpatterns.com/prototype.html
[multimethods]: https://en.wikipedia.org/wiki/Multiple_dispatch
[lua]: https://www.lua.org/pil/13.4.1.html
[later]: #design-note

<aside name="multimethods">

Multimethods are the approach you're least likely to be familiar with. I'd love
to talk more about them -- I designed [a hobby language][magpie] around them
once and they are *super rad* -- but there are only so many pages I can fit in.
If you'd like to learn more, take a look at [CLOS][] (the object system in
Common Lisp), [Dylan][], [Julia][], or [Raku][].

[clos]: https://en.wikipedia.org/wiki/Common_Lisp_Object_System
[magpie]: http://magpie-lang.org/
[dylan]: https://opendylan.org/
[julia]: https://julialang.org/
[raku]: https://docs.raku.org/language/functions#Multi-dispatch

</aside>

Since you've written about a thousand lines of Java code with me already, I'm
assuming you don't need a detailed introduction to object orientation. The main
goal is to bundle data with the code that acts on it. Users do that by declaring
a *class* that:

<span name="circle"></span>

1. Exposes a *constructor* to create and initialize new *instances* of the
   class

1. Provides a way to store and access *fields* on instances

1. Defines a set of *methods* shared by all instances of the class that
   operate on each instances' state.

That's about as minimal as it gets. Most object-oriented languages, all the way
back to Simula, also do inheritance to reuse behavior across classes. We'll add
that in the [next chapter][inheritance]. Even kicking that out, we still have a
lot to get through. This is a big chapter and everything doesn't quite come
together until we have all of the above pieces, so gather your stamina.

<aside name="circle">

<img src="image/classes/circle.png" alt="The relationships between classes, methods, instances, constructors, and fields." />

It's like the circle of life, *sans* Sir Elton John.

</aside>

[inheritance]: inheritance.html

## Class Declarations

Like we do, we're gonna start with syntax. A `class` statement introduces a new
name, so it lives in the `declaration` grammar rule.

```ebnf
declaration    → classDecl
               | funDecl
               | varDecl
               | statement ;

classDecl      → "class" IDENTIFIER "{" function* "}" ;
```

The new `classDecl` rule relies on the `function` rule we defined
[earlier][function rule]. To refresh your memory:

[function rule]: functions.html#function-declarations

```ebnf
function       → IDENTIFIER "(" parameters? ")" block ;
parameters     → IDENTIFIER ( "," IDENTIFIER )* ;
```

In plain English, a class declaration is the `class` keyword, followed by the
class's name, then a curly-braced body. Inside that body is a list of method
declarations. Unlike function declarations, methods don't have a leading <span
name="fun">`fun`</span> keyword. Each method is a name, parameter list, and
body. Here's an example:

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
fields to them as you see fit using normal imperative code.

Over in our AST generator, the `classDecl` grammar rule gets its own statement
<span name="class-ast">node</span>.

^code class-ast (1 before, 1 after)

<aside name="class-ast">

The generated code for the new node is in [Appendix II][appendix-class].

[appendix-class]: appendix-ii.html#class-statement

</aside>

It stores the class's name and the methods inside its body. Methods are
represented by the existing Stmt.Function class that we use for function
declaration AST nodes. That gives us all the bits of state that we need for a
method: name, parameter list, and body.

A class can appear anywhere a named declaration is allowed, triggered by the
leading `class` keyword.

^code match-class (1 before, 1 after)

That calls out to:

^code parse-class-declaration

There's more meat to this than most of the other parsing methods, but it roughly
follows the grammar. We've already consumed the `class` keyword, so we look for
the expected class name next, followed by the opening curly brace. Once inside
the body, we keep parsing method declarations until we hit the closing brace.
Each method declaration is parsed by a call to `function()`, which we defined
back in the [chapter where functions were introduced][functions].

[functions]: functions.html

Like we do in any open-ended loop in the parser, we also check for hitting the
end of the file. That won't happen in correct code since a class should have a
closing brace at the end, but it ensures the parser doesn't get stuck in an
infinite loop if the user has a syntax error and forgets to correctly end the
class body.

We wrap the name and list of methods into a Stmt.Class node and we're done.
Previously, we would jump straight into the interpreter, but now we need to
plumb the node through the resolver first.

^code resolver-visit-class

We aren't going to worry about resolving the methods themselves yet, so for now
all we need to do is declare the class using its name. It's not common to
declare a class as a local variable, but Lox permits it, so we need to handle it
correctly.

Now we interpret the class declaration.

^code interpreter-visit-class

This looks similar to how we execute function declarations. We declare the
class's name in the current environment. Then we turn the class *syntax node*
into a LoxClass, the *runtime* representation of a class. We circle back and
store the class object in the variable we previously declared. That two-stage
variable binding process allows references to the class inside its own methods.

We will refine it throughout the chapter, but the first draft of LoxClass looks
like this:

^code lox-class

Literally a wrapper around a name. We don't even store the methods yet. Not
super useful, but it does have a `toString()` method so we can write a trivial
script and test that class objects are actually being parsed and executed.

```lox
class DevonshireCream {
  serveOn() {
    return "Scones";
  }
}

print DevonshireCream; // Prints "DevonshireCream".
```

## Creating Instances

We have classes, but they don't do anything yet. Lox doesn't have "static"
methods that you can call right on the class itself, so without actual
instances, classes are useless. Thus instances are the next step.

While some syntax and semantics are fairly standard across OOP languages, the
way you create new instances isn't. Ruby, following Smalltalk, creates instances
by calling a method on the class object itself, a <span
name="turtles">recursively</span> graceful approach. Some, like C++ and Java,
have a `new` keyword dedicated to birthing a new object. Python has you "call"
the class itself like a function. (JavaScript, ever weird, sort of does both.)

<aside name="turtles">

In Smalltalk, even *classes* are created by calling methods on an existing
object, usually the desired superclass. It's sort of a turtles-all-the-way-down
thing. It ultimately bottoms out on a few magical classes like Object and
Metaclass that the runtime conjures into being *ex nihilo*.

</aside>

I took a minimal approach with Lox. We already have class objects, and we
already have function calls, so we'll use call expressions on class objects to
create new instances. It's as if a class is a factory function that generates
instances of itself. This feels elegant to me, and also spares us the need to
introduce syntax like `new`. Therefore, we can skip past the front end straight
into the runtime.

Right now, if you try this:

```lox
class Bagel {}
Bagel();
```

You get a runtime error. `visitCallExpr()` checks to see if the called object
implements `LoxCallable` and reports an error since LoxClass doesn't. Not *yet*,
that is.

^code lox-class-callable (2 before, 1 after)

Implementing that interface requires two methods.

^code lox-class-call-arity

The interesting one is `call()`. When you "call" a class, it instantiates a new
LoxInstance for the called class and returns it. The `arity()` method is how the
interpreter validates that you passed the right number of arguments to a
callable. For now, we'll say you can't pass any. When we get to user-defined
constructors, we'll revisit this.

That leads us to LoxInstance, the runtime representation of an instance of a Lox
class. Again, our first implementation starts small.

^code lox-instance

Like LoxClass, it's pretty bare bones, but we're only getting started. If you
want to give it a try, here's a script to run:

```lox
class Bagel {}
var bagel = Bagel();
print bagel; // Prints "Bagel instance".
```

This program doesn't do much, but it's starting to do *something*.

## Properties on Instances

We have instances, so we should make them useful. We're at a fork in the road.
We could add behavior first -- methods -- or we could start with state --
properties. We're going to take the latter because, as we'll see, the two get
entangled in an interesting way and it will be easier to make sense of them if
we get properties working first.

Lox follows JavaScript and Python in how it handles state. Every instance is an
open collection of named values. Methods on the instance's class can access and
modify properties, but so can <span name="outside">outside</span> code.
Properties are accessed using a `.` syntax.

<aside name="outside">

Allowing code outside of the class to directly modify an object's fields goes
against the object-oriented credo that a class *encapsulates* state. Some
languages take a more principled stance. In Smalltalk, fields are accessed using
simple identifiers -- essentially, variables that are only in scope inside a
class's methods. Ruby uses `@` followed by a name to access a field in an
object. That syntax is only meaningful inside a method and always accesses state
on the current object.

Lox, for better or worse, isn't quite so pious about its OOP faith.

</aside>

```lox
someObject.someProperty
```

An expression followed by `.` and an identifier reads the property with that
name from the object the expression evaluates to. That dot has the same
precedence as the parentheses in a function call expression, so we slot it into
the grammar by replacing the existing `call` rule with:

```ebnf
call           → primary ( "(" arguments? ")" | "." IDENTIFIER )* ;
```

After a primary expression, we allow a series of any mixture of parenthesized
calls and dotted property accesses. "Property access" is a mouthful, so from
here on out, we'll call these "get expressions".

### Get expressions

The <span name="get-ast">syntax tree node</span> is:

^code get-ast (1 before, 1 after)

<aside name="get-ast">

The generated code for the new node is in [Appendix II][appendix-get].

[appendix-get]: appendix-ii.html#get-expression

</aside>

Following the grammar, the new parsing code goes in our existing `call()`
method.

^code parse-property (3 before, 4 after)

The outer `while` loop there corresponds to the `*` in the grammar rule. We zip
along the tokens building up a chain of calls and gets as we find parentheses
and dots, like so:

<img src="image/classes/zip.png" alt="Parsing a series of '.' and '()' expressions to an AST." />

Instances of the new Expr.Get node feed into the resolver.

^code resolver-visit-get

OK, not much to that. Since properties are looked up <span
name="dispatch">dynamically</span>, they don't get resolved. During resolution,
we recurse only into the expression to the left of the dot. The actual property
access happens in the interpreter.

<aside name="dispatch">

You can literally see that property dispatch in Lox is dynamic since we don't
process the property name during the static resolution pass.

</aside>

^code interpreter-visit-get

First, we evaluate the expression whose property is being accessed. In Lox, only
instances of classes have properties. If the object is some other type like a
number, invoking a getter on it is a runtime error.

If the object is a LoxInstance, then we ask it to look up the property. It must
be time to give LoxInstance some actual state. A map will do fine.

^code lox-instance-fields (1 before, 2 after)

Each key in the map is a property name and the corresponding value is the
property's value. To look up a property on an instance:

^code lox-instance-get-property

<aside name="hidden">

Doing a hash table lookup for every field access is fast enough for many
language implementations, but not ideal. High performance VMs for languages like
JavaScript use sophisticated optimizations like "[hidden classes][]" to avoid
that overhead.

Paradoxically, many of the optimizations invented to make dynamic languages fast
rest on the observation that -- even in those languages -- most code is fairly
static in terms of the types of objects it works with and their fields.

[hidden classes]: http://richardartoul.github.io/jekyll/update/2015/04/26/hidden-classes.html

</aside>

An interesting edge case we need to handle is what happens if the instance
doesn't *have* a property with the given name. We could silently return some
dummy value like `nil`, but my experience with languages like JavaScript is that
this behavior masks bugs more often than it does anything useful. Instead, we'll
make it a runtime error.

So the first thing we do is see if the instance actually has a field with the
given name. Only then do we return it. Otherwise, we raise an error.

Note how I switched from talking about "properties" to "fields". There is a
subtle difference between the two. Fields are named bits of state stored
directly in an instance. Properties are the named, uh, *things*, that a get
expression may return. Every field is a property, but as we'll see <span
name="foreshadowing">later</span>, not every property is a field.

<aside name="foreshadowing">

Ooh, foreshadowing. Spooky!

</aside>

In theory, we can now read properties on objects. But since there's no way to
actually stuff any state into an instance, there are no fields to access. Before
we can test out reading, we must support writing.

### Set expressions

Setters use the same syntax as getters, except they appear on the left side of
an assignment.

```lox
someObject.someProperty = value;
```

In grammar land, we extend the rule for assignment to allow dotted identifiers
on the left-hand side.

```ebnf
assignment     → ( call "." )? IDENTIFIER "=" assignment
               | logic_or ;
```

Unlike getters, setters don't chain. However, the reference to `call` allows any
high-precedence expression before the last dot, including any number of
*getters*, as in:

<img src="image/classes/setter.png" alt="breakfast.omelette.filling.meat = ham" />

Note here that only the *last* part, the `.meat` is the *setter*. The
`.omelette` and `.filling` parts are both *get* expressions.

Just as we have two separate AST nodes for variable access and variable
assignment, we need a <span name="set-ast">second setter node</span> to
complement our getter node.

^code set-ast (1 before, 1 after)

<aside name="set-ast">

The generated code for the new node is in [Appendix II][appendix-set].

[appendix-set]: appendix-ii.html#set-expression

</aside>

In case you don't remember, the way we handle assignment in the parser is a
little funny. We can't easily tell that a series of tokens is the left-hand side
of an assignment until we reach the `=`. Now that our assignment grammar rule
has `call` on the left side, which can expand to arbitrarily large expressions,
that final `=` may be many tokens away from the point where we need to know
we're parsing an assignment.

Instead, the trick we do is parse the left-hand side as a normal expression.
Then, when we stumble onto the equal sign after it, we take the expression we
already parsed and transform it into the correct syntax tree node for the
assignment.

We add another clause to that transformation to handle turning an Expr.Get
expression on the left into the corresponding Expr.Set.

^code assign-set (1 before, 1 after)

That's parsing our syntax. We push that node through into the resolver.

^code resolver-visit-set

Again, like Expr.Get, the property itself is dynamically evaluated, so there's
nothing to resolve there. All we need to do is recurse into the two
subexpressions of Expr.Set, the object whose property is being set, and the
value it's being set to.

That leads us to the interpreter.

^code interpreter-visit-set

We evaluate the object whose property is being set and check to see if it's a
LoxInstance. If not, that's a runtime error. Otherwise, we evaluate the value
being set and store it on the instance. That relies on a new method in
LoxInstance.

<aside name="order">

This is another semantic edge case. There are three distinct operations:

1. Evaluate the object.

2. Raise a runtime error if it's not an instance of a class.

3. Evaluate the value.

The order that those are performed in could be user visible, which means we need
to carefully specify it and ensure our implementations do these in the same
order.

</aside>

^code lox-instance-set-property

No real magic here. We stuff the values straight into the Java map where fields
live. Since Lox allows freely creating new fields on instances, there's no need
to see if the key is already present.

## Methods on Classes

You can create instances of classes and stuff data into them, but the class
itself doesn't really *do* anything. Instances are just maps and all instances
are more or less the same. To make them feel like instances *of classes*, we
need behavior -- methods.

Our helpful parser already parses method declarations, so we're good there. We
also don't need to add any new parser support for method *calls*. We already
have `.` (getters) and `()` (function calls). A "method call" simply chains
those together.

<img src="image/classes/method.png" alt="The syntax tree for 'object.method(argument)" />

That raises an interesting question. What happens when those two expressions are
pulled apart? Assuming that `method` in this example is a method on the class of
`object` and not a field on the instance, what should the following piece of
code do?

```lox
var m = object.method;
m(argument);
```

This program "looks up" the method and stores the result -- whatever that is --
in a variable and then calls that object later. Is this allowed? Can you treat a
method like it's a function on the instance?

What about the other direction?

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

Different languages have different answers to these questions. One could write a
treatise on it. For Lox, we'll say the answer to both of these is yes, it does
work. We have a couple of reasons to justify that. For the second example --
calling a function stored in a field -- we want to support that because
first-class functions are useful and storing them in fields is a perfectly
normal thing to do.

The first example is more obscure. One motivation is that users generally expect
to be able to hoist a subexpression out into a local variable without changing
the meaning of the program. You can take this:

```lox
breakfast(omelette.filledWith(cheese), sausage);
```

And turn it into this:

```lox
var eggs = omelette.filledWith(cheese);
breakfast(eggs, sausage);
```

And it does the same thing. Likewise, since the `.` and the `()` in a method
call *are* two separate expressions, it seems you should be able to hoist the
*lookup* part into a variable and then call it <span
name="callback">later</span>. We need to think carefully about what the *thing*
you get when you look up a method is, and how it behaves, even in weird cases
like:

<aside name="callback">

A motivating use for this is callbacks. Often, you want to pass a callback whose
body simply invokes a method on some object. Being able to look up the method and
pass it directly saves you the chore of manually declaring a function to wrap
it. Compare this:

```lox
fun callback(a, b, c) {
  object.method(a, b, c);
}

takeCallback(callback);
```

With this:

```lox
takeCallback(object.method);
```

</aside>

```lox
class Person {
  sayName() {
    print this.name;
  }
}

var jane = Person();
jane.name = "Jane";

var method = jane.sayName;
method(); // ?
```

If you grab a handle to a method on some instance and call it later, does it
"remember" the instance it was pulled off from? Does `this` inside the method
still refer to that original object?

Here's a more pathological example to bend your brain:

```lox
class Person {
  sayName() {
    print this.name;
  }
}

var jane = Person();
jane.name = "Jane";

var bill = Person();
bill.name = "Bill";

bill.sayName = jane.sayName;
bill.sayName(); // ?
```

Does that last line print "Bill" because that's the instance that we *called*
the method through, or "Jane" because it's the instance where we first grabbed
the method?

Equivalent code in Lua and JavaScript would print "Bill". Those languages don't
really have a notion of "methods". Everything is sort of functions-in-fields, so
it's not clear that `jane` "owns" `sayName` any more than `bill` does.

Lox, though, has real class syntax so we do know which callable things are
methods and which are functions. Thus, like Python, C#, and others, we will have
methods "bind" `this` to the original instance when the method is first grabbed.
Python calls <span name="bound">these</span> **bound methods**.

<aside name="bound">

I know, imaginative name, right?

</aside>

In practice, that's usually what you want. If you take a reference to a method
on some object so you can use it as a callback later, you want to remember the
instance it belonged to, even if that callback happens to be stored in a field
on some other object.

OK, that's a lot of semantics to load into your head. Forget about the edge
cases for a bit. We'll get back to those. For now, let's get basic method calls
working. We're already parsing the method declarations inside the class body, so
the next step is to resolve them.

^code resolve-methods (1 before, 1 after)

<aside name="local">

Storing the function type in a local variable is pointless right now, but we'll
expand this code before too long and it will make more sense.

</aside>

We iterate through the methods in the class body and call the
`resolveFunction()` method we wrote for handling function declarations already.
The only difference is that we pass in a new FunctionType enum value.

^code function-type-method (1 before, 1 after)

That's going to be important when we resolve `this` expressions. For now, don't
worry about it. The interesting stuff is in the interpreter.

^code interpret-methods (1 before, 1 after)

When we interpret a class declaration statement, we turn the syntactic
representation of the class -- its AST node -- into its runtime representation.
Now, we need to do that for the methods contained in the class as well. Each
method declaration blossoms into a LoxFunction object.

We take all of those and wrap them up into a map, keyed by the method names.
That gets stored in LoxClass.

^code lox-class-methods (1 before, 3 after)

Where an instance stores state, the class stores behavior. LoxInstance has its
map of fields, and LoxClass gets a map of methods. Even though methods are
owned by the class, they are still accessed through instances of that class.

^code lox-instance-get-method (5 before, 2 after)

When looking up a property on an instance, if we don't <span
name="shadow">find</span> a matching field, we look for a method with that name
on the instance's class. If found, we return that. This is where the distinction
between "field" and "property" becomes meaningful. When accessing a property,
you might get a field -- a bit of state stored on the instance -- or you could
hit a method defined on the instance's class.

The method is looked up using this:

<aside name="shadow">

Looking for a field first implies that fields shadow methods, a subtle but
important semantic point.

</aside>

^code lox-class-find-method

You can probably guess this method is going to get more interesting later. For
now, a simple map lookup on the class's method table is enough to get us
started. Give it a try:

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

We can define both behavior and state on objects, but they aren't tied together
yet. Inside a method, we have no way to access the fields of the "current"
object -- the instance that the method was called on -- nor can we call other
methods on that same object.

To get at that instance, it needs a <span name="i">name</span>. Smalltalk,
Ruby, and Swift use "self". Simula, C++, Java, and others use "this". Python
uses "self" by convention, but you can technically call it whatever you like.

<aside name="i">

"I" would have been a great choice, but using "i" for loop variables predates
OOP and goes all the way back to Fortran. We are victims of the incidental
choices of our forebears.

</aside>

For Lox, since we generally hew to Java-ish style, we'll go with "this". Inside
a method body, a `this` expression evaluates to the instance that the method was
called on. Or, more specifically, since methods are accessed and then invoked as
two steps, it will refer to the object that the method was *accessed* from.

That makes our job harder. Peep at:

```lox
class Egotist {
  speak() {
    print this;
  }
}

var method = Egotist().speak;
method();
```

On the second-to-last line, we grab a reference to the `speak()` method off an
instance of the class. That returns a function, and that function needs to
remember the instance it was pulled off of so that *later*, on the last line, it
can still find it when the function is called.

We need to take `this` at the point that the method is accessed and attach it to
the function somehow so that it stays around as long as we need it to. Hmm... a
way to store some extra data that hangs around a function, eh? That sounds an
awful lot like a *closure*, doesn't it?

If we defined `this` as a sort of hidden variable in an environment that
surrounds the function returned when looking up a method, then uses of `this` in
the body would be able to find it later. LoxFunction already has the ability to
hold on to a surrounding environment, so we have the machinery we need.

Let's walk through an example to see how it works:

```lox
class Cake {
  taste() {
    var adjective = "delicious";
    print "The " + this.flavor + " cake is " + adjective + "!";
  }
}

var cake = Cake();
cake.flavor = "German chocolate";
cake.taste(); // Prints "The German chocolate cake is delicious!".
```

When we first evaluate the class definition, we create a LoxFunction for
`taste()`. Its closure is the environment surrounding the class, in this case
the global one. So the LoxFunction we store in the class's method map looks
like so:

<img src="image/classes/closure.png" alt="The initial closure for the method." />

When we evaluate the `cake.taste` get expression, we create a new environment
that binds `this` to the object the method is accessed from (here, `cake`). Then
we make a *new* LoxFunction with the same code as the original one but using
that new environment as its closure.

<img src="image/classes/bound-method.png" alt="The new closure that binds 'this'." />

This is the LoxFunction that gets returned when evaluating the get expression
for the method name. When that function is later called by a `()` expression,
we create an environment for the method body as usual.

<img src="image/classes/call.png" alt="Calling the bound method and creating a new environment for the method body." />

The parent of the body environment is the environment we created earlier to bind
`this` to the current object. Thus any use of `this` inside the body
successfully resolves to that instance.

Reusing our environment code for implementing `this` also takes care of
interesting cases where methods and functions interact, like:

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

In, say, JavaScript, it's common to return a callback from inside a method. That
callback may want to hang on to and retain access to the original object -- the
`this` value -- that the method was associated with. Our existing support for
closures and environment chains should do all this correctly.

Let's code it up. The first step is adding <span name="this-ast">new
syntax</span> for `this`.

^code this-ast (1 before, 1 after)

<aside name="this-ast">

The generated code for the new node is in [Appendix II][appendix-this].

[appendix-this]: appendix-ii.html#this-expression

</aside>

Parsing is simple since it's a single token which our lexer already
recognizes as a reserved word.

^code parse-this (2 before, 2 after)

You can start to see how `this` works like a variable when we get to the
resolver.

^code resolver-visit-this

We resolve it exactly like any other local variable using "this" as the name for
the "variable". Of course, that's not going to work right now, because "this"
*isn't* declared in any scope. Let's fix that over in `visitClassStmt()`.

^code resolver-begin-this-scope (2 before, 1 after)

Before we step in and start resolving the method bodies, we push a new scope and
define "this" in it as if it were a variable. Then, when we're done, we discard
that surrounding scope.

^code resolver-end-this-scope (2 before, 1 after)

Now, whenever a `this` expression is encountered (at least inside a method) it
will resolve to a "local variable" defined in an implicit scope just outside of
the block for the method body.

The resolver has a new *scope* for `this`, so the interpreter needs to create a
corresponding *environment* for it. Remember, we always have to keep the
resolver's scope chains and the interpreter's linked environments in sync with
each other. At runtime, we create the environment after we find the method on
the instance. We replace the previous line of code that simply returned the
method's LoxFunction with this:

^code lox-instance-bind-method (1 before, 3 after)

Note the new call to `bind()`. That looks like so:

^code bind-instance

There isn't much to it. We create a new environment nestled inside the method's
original closure. Sort of a closure-within-a-closure. When the method is called,
that will become the parent of the method body's environment.

We declare "this" as a variable in that environment and bind it to the given
instance, the instance that the method is being accessed from. *Et voilà*, the
returned LoxFunction now carries around its own little persistent world where
"this" is bound to the object.

The remaining task is interpreting those `this` expressions. Similar to the
resolver, it is the same as interpreting a variable expression.

^code interpreter-visit-this

Go ahead and give it a try using that cake example from earlier. With less than
twenty lines of code, our interpreter handles `this` inside methods even in all
of the weird ways it can interact with nested classes, functions inside methods,
handles to methods, etc.

### Invalid uses of this

Wait a minute. What happens if you try to use `this` *outside* of a method? What
about:

```lox
print this;
```

Or:

```lox
fun notAMethod() {
  print this;
}
```

There is no instance for `this` to point to if you're not in a method. We could
give it some default value like `nil` or make it a runtime error, but the user
has clearly made a mistake. The sooner they find and fix that mistake, the
happier they'll be.

Our resolution pass is a fine place to detect this error statically. It already
detects `return` statements outside of functions. We'll do something similar for
`this`. In the vein of our existing FunctionType enum, we define a new ClassType
one.

^code class-type (1 before, 1 after)

Yes, it could be a Boolean. When we get to inheritance, it will get a third
value, hence the enum right now. We also add a corresponding field,
`currentClass`. Its value tells us if we are currently inside a class
declaration while traversing the syntax tree. It starts out `NONE` which means
we aren't in one.

When we begin to resolve a class declaration, we change that.

^code set-current-class (1 before, 1 after)

As with `currentFunction`, we store the previous value of the field in a local
variable. This lets us piggyback onto the JVM to keep a stack of `currentClass`
values. That way we don't lose track of the previous value if one class nests
inside another.

Once the methods have been resolved, we "pop" that stack by restoring the old
value.

^code restore-current-class (2 before, 1 after)

When we resolve a `this` expression, the `currentClass` field gives us the bit
of data we need to report an error if the expression doesn't occur nestled
inside a method body.

^code this-outside-of-class (1 before, 1 after)

That should help users use `this` correctly, and it saves us from having to
handle misuse at runtime in the interpreter.

## Constructors and Initializers

We can do almost everything with classes now, and as we near the end of the
chapter we find ourselves strangely focused on a beginning. Methods and fields
let us encapsulate state and behavior together so that an object always *stays*
in a valid configuration. But how do we ensure a brand new object *starts* in a
good state?

For that, we need constructors. I find them one of the trickiest parts of a
language to design, and if you peer closely at most other languages, you'll see
<span name="cracks">cracks</span> around object construction where the seams of
the design don't quite fit together perfectly. Maybe there's something
intrinsically messy about the moment of birth.

<aside name="cracks">

A few examples: In Java, even though final fields must be initialized, it is
still possible to read one *before* it has been. Exceptions -- a huge, complex
feature -- were added to C++ mainly as a way to emit errors from constructors.

</aside>

"Constructing" an object is actually a pair of operations:

1.  The runtime <span name="allocate">*allocates*</span> the memory required for
    a fresh instance. In most languages, this operation is at a fundamental
    level beneath what user code is able to access.

    <aside name="allocate">

    C++'s "[placement new][]" is a rare example where the bowels of allocation
    are laid bare for the programmer to prod.

    </aside>

2.  Then, a user-provided chunk of code is called which *initializes* the
    unformed object.

[placement new]: https://en.wikipedia.org/wiki/Placement_syntax

The latter is what we tend to think of when we hear "constructor", but the
language itself has usually done some groundwork for us before we get to that
point. In fact, our Lox interpreter already has that covered when it creates a
new LoxInstance object.

We'll do the remaining part -- user-defined initialization -- now. Languages
have a variety of notations for the chunk of code that sets up a new object for
a class. C++, Java, and C# use a method whose name matches the class name. Ruby
and Python call it `init()`. The latter is nice and short, so we'll do that.

In LoxClass's implementation of LoxCallable, we add a few more lines.

^code lox-class-call-initializer (2 before, 1 after)

When a class is called, after the LoxInstance is created, we look for an "init"
method. If we find one, we immediately bind and invoke it just like a normal
method call. The argument list is forwarded along.

That argument list means we also need to tweak how a class declares its arity.

^code lox-initializer-arity (1 before, 1 after)

If there is an initializer, that method's arity determines how many arguments
you must pass when you call the class itself. We don't *require* a class to
define an initializer, though, as a convenience. If you don't have an
initializer, the arity is still zero.

That's basically it. Since we bind the `init()` method before we call it, it has
access to `this` inside its body. That, along with the arguments passed to the
class, are all you need to be able to set up the new instance however you
desire.

### Invoking init() directly

As usual, exploring this new semantic territory rustles up a few weird
creatures. Consider:

```lox
class Foo {
  init() {
    print this;
  }
}

var foo = Foo();
print foo.init();
```

Can you "re-initialize" an object by directly calling its `init()` method? If
you do, what does it return? A <span name="compromise">reasonable</span> answer
would be `nil` since that's what it appears the body returns.

However -- and I generally dislike compromising to satisfy the
implementation -- it will make clox's implementation of constructors much
easier if we say that `init()` methods always return `this`, even when
directly called. In order to keep jlox compatible with that, we add a little
special case code in LoxFunction.

<aside name="compromise">

Maybe "dislike" is too strong a claim. It's reasonable to have the constraints
and resources of your implementation affect the design of the language. There
are only so many hours in the day, and if a cut corner here or there lets you get
more features to users in less time, it may very well be a net win for their
happiness and productivity. The trick is figuring out *which* corners to cut
that won't cause your users and future self to curse your shortsightedness.

</aside>

^code return-this (2 before, 1 after)

If the function is an initializer, we override the actual return value and
forcibly return `this`. That relies on a new `isInitializer` field.

^code is-initializer-field (2 before, 2 after)

We can't simply see if the name of the LoxFunction is "init" because the user
could have defined a *function* with that name. In that case, there *is* no
`this` to return. To avoid *that* weird edge case, we'll directly store whether
the LoxFunction represents an initializer method. That means we need to go back
and fix the few places where we create LoxFunctions.

^code construct-function (1 before, 1 after)

For actual function declarations, `isInitializer` is always false. For methods,
we check the name.

^code interpreter-method-initializer (1 before, 1 after)

And then in `bind()` where we create the closure that binds `this` to a method,
we pass along the original method's value.

^code lox-function-bind-with-initializer (1 before, 1 after)

### Returning from init()

We aren't out of the woods yet. We've been assuming that a user-written
initializer doesn't explicitly return a value because most constructors don't.
What should happen if a user tries:

```lox
class Foo {
  init() {
    return "something else";
  }
}
```

It's definitely not going to do what they want, so we may as well make it a
static error. Back in the resolver, we add another case to FunctionType.

^code function-type-initializer (1 before, 1 after)

We use the visited method's name to determine if we're resolving an initializer
or not.

^code resolver-initializer-type (1 before, 1 after)

When we later traverse into a `return` statement, we check that field and make
it an error to return a value from inside an `init()` method.

^code return-in-initializer (1 before, 1 after)

We're *still* not done. We statically disallow returning a *value* from an
initializer, but you can still use an empty early `return`.

```lox
class Foo {
  init() {
    return;
  }
}
```

That is actually kind of useful sometimes, so we don't want to disallow it
entirely. Instead, it should return `this` instead of `nil`. That's an easy fix
over in LoxFunction.

^code early-return-this (1 before, 1 after)

If we're in an initializer and execute a `return` statement, instead of
returning the value (which will always be `nil`), we again return `this`.

Phew! That was a whole list of tasks but our reward is that our little
interpreter has grown an entire programming paradigm. Classes, methods, fields,
`this`, and constructors. Our baby language is looking awfully grown-up.

<div class="challenges">

## Challenges

1.  We have methods on instances, but there is no way to define "static" methods
    that can be called directly on the class object itself. Add support for
    them. Use a `class` keyword preceding the method to indicate a static method
    that hangs off the class object.

    ```lox
    class Math {
      class square(n) {
        return n * n;
      }
    }

    print Math.square(3); // Prints "9".
    ```

    You can solve this however you like, but the "[metaclasses][]" used by
    Smalltalk and Ruby are a particularly elegant approach. *Hint: Make LoxClass
    extend LoxInstance and go from there.*

2.  Most modern languages support "getters" and "setters" -- members on a class
    that look like field reads and writes but that actually execute user-defined
    code. Extend Lox to support getter methods. These are declared without a
    parameter list. The body of the getter is executed when a property with that
    name is accessed.

    ```lox
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
    ```

3.  Python and JavaScript allow you to freely access an object's fields from
    outside of its own methods. Ruby and Smalltalk encapsulate instance state.
    Only methods on the class can access the raw fields, and it is up to the
    class to decide which state is exposed. Most statically typed languages
    offer modifiers like `private` and `public` to control which parts of a
    class are externally accessible on a per-member basis.

    What are the trade-offs between these approaches and why might a language
    prefer one or the other?

[metaclasses]: https://en.wikipedia.org/wiki/Metaclass

</div>

<div class="design-note">

## Design Note: Prototypes and Power

In this chapter, we introduced two new runtime entities, LoxClass and
LoxInstance. The former is where behavior for objects lives, and the latter is
for state. What if you could define methods right on a single object, inside
LoxInstance? In that case, we wouldn't need LoxClass at all. LoxInstance would
be a complete package for defining the behavior and state of an object.

We'd still want some way, without classes, to reuse behavior across multiple
instances. We could let a LoxInstance [*delegate*][delegate] directly to another
LoxInstance to reuse its fields and methods, sort of like inheritance.

Users would model their program as a constellation of objects, some of which
delegate to each other to reflect commonality. Objects used as delegates
represent "canonical" or "prototypical" objects that others refine. The result
is a simpler runtime with only a single internal construct, LoxInstance.

That's where the name **[prototypes][proto]** comes from for this paradigm. It
was invented by David Ungar and Randall Smith in a language called [Self][].
They came up with it by starting with Smalltalk and following the above mental
exercise to see how much they could pare it down.

Prototypes were an academic curiosity for a long time, a fascinating one that
generated interesting research but didn't make a dent in the larger world of
programming. That is, until Brendan Eich crammed prototypes into JavaScript,
which then promptly took over the world. Many (many) <span
name="words">words</span> have been written about prototypes in JavaScript.
Whether that shows that prototypes are brilliant or confusing -- or both! -- is
an open question.

<aside name="words">

Including [more than a handful][prototypes] by yours truly.

</aside>

I won't get into whether or not I think prototypes are a good idea for a
language. I've made languages that are [prototypal][finch] and
[class-based][wren], and my opinions of both are complex. What I want to discuss
is the role of *simplicity* in a language.

Prototypes are simpler than classes -- less code for the language implementer to
write, and fewer concepts for the user to learn and understand. Does that make
them better? We language nerds have a tendency to fetishize minimalism.
Personally, I think simplicity is only part of the equation. What we really want
to give the user is *power*, which I define as:

```text
power = breadth × ease ÷ complexity
```

None of these are precise numeric measures. I'm using math as analogy here, not
actual quantification.

*   **Breadth** is the range of different things the language lets you express.
    C has a lot of breadth -- it's been used for everything from operating
    systems to user applications to games. Domain-specific languages like
    AppleScript and Matlab have less breadth.

*   **Ease** is how little effort it takes to make the language do what you
    want. "Usability" might be another term, though it carries more baggage than
    I want to bring in. "Higher-level" languages tend to have more ease than
    "lower-level" ones. Most languages have a "grain" to them where some things
    feel easier to express than others.

*   **Complexity** is how big the language (including its runtime, core libraries,
    tools, ecosystem, etc.) is. People talk about how many pages are in a
    language's spec, or how many keywords it has. It's how much the user has to
    load into their wetware before they can be productive in the system. It is
    the antonym of simplicity.

[proto]: https://en.wikipedia.org/wiki/Prototype-based_programming

Reducing complexity *does* increase power. The smaller the denominator, the
larger the resulting value, so our intuition that simplicity is good is valid.
However, when reducing complexity, we must take care not to sacrifice breadth or
ease in the process, or the total power may go down. Java would be a strictly
*simpler* language if it removed strings, but it probably wouldn't handle text
manipulation tasks well, nor would it be as easy to get things done.

The art, then, is finding *accidental* complexity that can be omitted --
language features and interactions that don't carry their weight by increasing
the breadth or ease of using the language.

If users want to express their program in terms of categories of objects, then
baking classes into the language increases the ease of doing that, hopefully by
a large enough margin to pay for the added complexity. But if that isn't how
users are using your language, then by all means leave classes out.

</div>

[delegate]: https://en.wikipedia.org/wiki/Prototype-based_programming#Delegation
[prototypes]: http://gameprogrammingpatterns.com/prototype.html
[self]: http://www.selflanguage.org/
[finch]: http://finch.stuffwithstuff.com/
[wren]: http://wren.io/
