^title Inheritance
^part A Tree-Walk Interpreter

**todo: more short asides.**

Can you believe it? We're at the last chapter of Part II. We're almost done with
our first Lox interpreter. The [previous chapter][] was a big ball of stuff
because many class features are intertwined together and hard to separate out.
However one -- well sort of two -- parts do separate pretty cleanly. In this
chapter, we'll finish off Lox's object-orientation support by adding
inheritance.

[previous chapter]: classes.html

Inheritance has been a feature of object-oriented languages all the way back to
the first one, [Simula][]. Early on, Kristen Nygaard and Ole-Johan Dahl noticed
that in many of the simulation programs they wrote, several different classes
had similarities. Inheritance gave them a way to reuse those common parts across
classes.

[simula]: https://en.wikipedia.org/wiki/Simula

When OOP first got big, inheritance was considered one of its most important
features. "Good OOP design" meant lots of big deep class hierarchies. If your
class diagrams didn't look like the Hapsburg family tree, you were an amateur.
We've thankfully dialed it back since then. We now realize than inheriting from
another class is one of the deepest couplings you can introduce in your code.
It's a powerful tool, and the best way to use it safely is to understand it well. Implementing it ourselves will help.

## Superclasses and Subclasses

Given that the concept is *inheritance*, you would think they would pick a
consistent metaphor and call them "parent" and "child" classes, but that would
be too easy. Way back when, C.A.R. Hoare coined the term "<span
name="subclass">subclass</span>" to refer to a record type that refines another
type. Simula borrowed that term to refer to *classes* that inherit from another.
I don't think it was until Smalltalk came along that someone flipped the Latin
prefix to get "superclass" to refer to the other side of the relationship. From
C++, we also get "base" and "derived" classes, so you hear those terms as well.
I'll mostly stick with "superclass" and "subclass".

<aside name="subclass">

"Super-" and "sub-" mean "above" and "below" in Latin, respectively. If you imagine an inheritance tree with the root at the top like a family tree, than the spatial metaphor holds up -- subclasses are below their superclasses on the diagram. More generally, "sub-" is used to refer to things that refine or are contained by some more general concept. In zoology, a subclass is a finer categorization of a larger class of living things.

In set theory, a subset is contained by a larger superset which contains all of the elements of the subset and possibly more. Set theory and programming languages directly collide in type theory. There, you have "supertypes" and "subtypes". Think of a type as a (possibly infinite) set of values. The type "int" is the set `{1, 2, 3, ...}`. In that case, a sub-*type* is a sub-*set* of those values.

In statically-typed object-oriented languages, a subclass is also often a subtype of its superclass. Let's say we have a Bread superclass and a Rye subclass. Think of Bread as the set of all instances that are of that class and likewise for Rye. Because Rye inherits from Bread, every Rye object is *also* a Bread object. That means the set of Bread objects includes both `a` and `b`. The set of Rye objects is just `b`, which is a subset of the set `{a, b}`. Subclass, subtype, and subset all line up.

**todo illustrate**

</aside>

Our first step in supporting inheritance in Lox is a way to create a class that subclasses another one. There's a lot of variety in syntax for this. C++ and C# follow the subclass's name with a colon then the superclass. Java uses `extends` instead of `:`. Python puts the superclasses in parentheses after the class name. Simula puts the superclass's name *before* the subclass.

This late in the game, I'd rather not add a new reserved word or token to the lexer. We don't have `extends` or even `:`, so we'll do what Ruby does and use `<`, as in:

```lox
class Bread {
  // General bread stuff...
}

class Rye < Bread {
  // Rye-specific stuff...
}
```

In the grammar, we tweak our existing `classDecl` rule like so:

```lox
classDecl → "class" IDENTIFIER ( "<" IDENTIFIER )?
            "{" function* "}" ;
```

So, after the class name, you can have a `<` followed by the superclass's name.
The superclass clause is optional because you don't *have* to have a superclass.
Unlike some other object-oriented languages like Java, Lox has no root "Object"
class that everything inherits from, so when you omit the superclass clause, the
class has *no* superclass, not even an implicit one.

If present, we store the superclass in the class declaration's syntax tree:

^code superclass-ast (1 before, 1 after)

We store it as an Expr. Even though syntactically the superclass must be a
single identifier, semantically it is a variable expression that we evaluate at
runtime to find the superclass. Storing it as an expression instead of a token
gives us an Expr that we can use in the resolver and interpreter to process it.

The tiny parser change follows the grammar exactly:

^code parse-superclass (1 before, 1 after)

Then at the end of `classDeclaration()` we pass in the superclass to the AST:

^code construct-class-ast (2 before, 1 after)

If we didn't parse a superclass clause, the superclass expression will be
`null`. We'll have to make sure the later passes check for that. Moving along to
the next phase, the resolver:

^code resolve-superclass (1 before, 2 after)

The class declaration AST node has a new subexpression, so we traverse into and
resolve that. When present, the superclass is usually a global variable since
classes are typically declared at the top level of a program, so this doesn't
generally do anything useful. However, Lox allows class declarations even inside
blocks, so it's possible for the superclass name to refer to a local variable.
In that case, we need to make sure it's resolved.

Once that's done, we move to the interpreter:

^code interpret-superclass (1 before, 1 after)

If there's a superclass expression, we evaluate it. Since that could potentially
evaluate to any random object, we need to check at runtime that the thing you
want to be your superclass is actually a class. Who knows what would happen if
we tried to run code like:

```lox
var NotAClass = "I am totally not a class";

class Subclass < NotAClass {} // ?!
```

Assuming that check passes, we continue on. Executing a class declaration turns
the syntactic representation of a class -- its AST node -- into it's runtime
representation, a LoxClass object. We need to plumb the superclass through to
that too. We pass it through the constructor:

^code interpreter-construct-class (3 before, 1 after)

...which initializes it:

^code lox-class-constructor (1 after)

...and stores it in a new field:

^code lox-class-superclass-field (1 before, 1 after)

That's our foundation. You can now define classes that are subclasses of other classes. This runs:

```lox
class Bread {
  // General bread stuff...
}

class Rye < Bread {
  // Rye-specific stuff...
}
```

Now, what does having a superclass actually *do?*

## Inheriting Methods

At a vague high level, inheriting from another class means that everything
that's <span name="liskov">true</span> of the superclass should be true, more or
less, of the subclass. In more rigid languages, that carries a lot of
implications. If the language has static types, that usually means a sub-*class*
also needs to be a sub-*type*. Otherwise it would be a type error to pass a
subclass where its superclass is expected. It often places restrictions on the
memory layout of subclass instances relative to superclass ones so that
accessing a field declared on the superclass still works given an instance of
the subclass.

<aside name="liskov">

A fancier name for this hand-wavey guideline is the [**Liskov substitution
principle**][liskov]. Barbara Liskov introduced it in a seminal keynote speech
during the formative period of object-oriented programming. She states it as:

[liskov]: https://en.wikipedia.org/wiki/Liskov_substitution_principle

> If for each object *o1* of type S there is an object *o2* of type T such that
> for all programs P defined in terms of T, the behavior of P is unchanged when
> *o1* is substituted for *o2*, then S is a subtype of T.

That's a little dense. She's defining what it means for one type to be a subtype
of another. The idea is that if you could swap out every object of one type with
objects of another type and the program doesn't blow up, then that latter type
is a subtype.

For us language designers, the rule is more useful going the other direction. If
the user has declared that B is a subtype of A, then the language should, as
much as possible, make sure sure that B can do everything A can do, and can be
used anywhere an A could be used.

**todo: overlap**

</aside>

Lox is a little dynamically typed language, so our requirements are much
simpler. Basically, it means that if you can call some method on an instance of
the superclass, you should be able to call the "same" method when given an
instance of the superclass. In other words, methods are inherited from the
superclass.

This lines up with one of the goals of inheritance -- to give users a way to
reuse code across classes. Implementing this is astonishingly easy:

^code find-method-recurse-superclass (3 before, 1 after)

That's literally all there is to it. When we are looking up a method on an
instance, if we don't find it on the instance's class, check its superclass if
there is one. That calls the same `findMethod()`, uh, method, which means it
implicitly recursively walks the entire inheritance chain looking for the
method.

Give it a try:

```lox
class Doughnut {
  cook() {
    print "Fry until golden brown.";
  }
}

class BostonCream < Doughnut {}

BostonCream().cook();
```

There we go, half of our inheritance features are done.

## Calling Superclass Methods

When we call a method on an instance, we walk the inheritance chain looking for
a match. We start at the bottom of the inheritance tree, at the class of the
object we called the method on and walk up the chain of superclasses. As soon as
we find a method, we stop. This is what gives us the familiar behavior that a
subclass can **override** a method inherited from its superclass. Sort of like
shadowing with variables, a subclass method will hide a superclass method with
the same name.

That's great if the subclass wants to *replace* some superclass behavior
completely. But, in practice, subclasses often want to *refine* the superclass's
behavior. They want to do a little bit that's specific to the subclass, but also
invoke the original superclass behavior too.

However, since the subclass has overridden the method, there's no way to refer
to the original one. If the subclass method tries to call it by name, it will
just recursively hit its own override. We need a way to say "Call this method,
but look for it directly on my superclass and ignore my override". Java uses
`super` for this, and we'll use the same syntax in Lox:

```lox
class Doughnut {
  cook() {
    print "Fry until golden brown.";
  }
}

class BostonCream < Doughnut {
  cook() {
    super.cook();
    print "Pipe full of custard and coat with chocolate.";
  }
}

BostonCream().cook();
```

The `super` keyword, followed by a dot and an identifier looks for a method with
that name. Unlike calls on `this`, the search starts at the superclass.

### Syntax

The syntax seems similar to `this` where the keyword works sort of like a magic variable. But with `super`, it's not a standalone expression. Instead, the property access is part of the super expression itself:

```lox
primary → "true" | "false" | "null" | "this"
        | NUMBER | STRING | IDENTIFIER | "(" expression ")"
        | "super" "." IDENTIFIER ;
```

It's still a highest-precedence primary expression, but the `.` and the
following identifer are part of the expression. Typically a super expression is
used for a method call, but, as with regular methods, the argument list is *not*
part of the expression. Instead, a super *call* is a super *access* followed by
a function call. Like other method calls, you can get a handle to a superclass
method and invoke it separately:

```lox
var method = super.cook;
method();
```

So the super expression itself contains only the token for the `super` keyword
(for error reporting) and the name of the method being looked up. The
corresponding syntax tree node is thus:

^code super-expr (1 before, 1 after)

The new production lives in the `primary` rule in the grammar, so in the parser,
we insert the new parsing code into the same-named method:

^code parse-super (2 before, 2 after)

A leading `super` keyword tells us we've hit a super expression. After that we
consume the expected `.` and method name.

### Semantics

Earlier, I said a super expression starts the method lookup from the superclass,
but *whose* superclass? We need to be more precise. The naïve answer is the
superclass of `this` the object the surrounding method was called on. That does
produce the intended behavior in a lot of cases, but that's not actually the
right rule. Here's an example to show you:

```lox
class A {
  method() {
    print "A method";
  }
}

class B < A {
  method() {
    print "B method";
  }

  test() {
    super.method();
  }
}

class C < B {}

C().test();
```

**todo: illustrate**

If we do things right, this program should print "A method". Tracing through
that goes something like:

* We call `C()` which creates an instance of C.
* We call `test()` on that. Inside that method, `this` is an instance of C.
* Inside that, we look up `method` on "the superclass".

That last step should *not* resolve to the superclass of `this`. That would give
us B, which is the same class that `test()` itself is defined in. Instead, the
rule we really want is the superclass of *the class containing the super
expression*. In this case, since `test()` is defined inside B, the super
expression inside it should start the lookup on *B*&rsquo;s superclass -- A.

So, in order to evaluate a super expression, we need access to the superclass of
the class definition surrounding the call. Alack and alas, at the point in the
interpreter where we are evaluating a super expression, we don't have that
easily available.

We could add a field to LoxFunction to store a reference to the LoxClass
associated with that method. Then, when calling a function, the interpreter
would store a reference to LoxFunction so that we could look it up later when we
hit a super expression. From there, we'd get the LoxClass of the method, then
its superclass.

That's a lot of plumbing, though. Back in the [last chapter][], we had a similar
problem when we needed to add support for `this`. In that case, we used our
existing environment and closure mechanism to store a reference to the current
object. Could we do something similar for storing the superclass? Well, I
probably wouldn't be talking about it if the answer was "no", so... yes.

[last chapter]: classes.html

One important difference is that with `this`, we create the environment for it
right when the property access for the method is executed. That's because the
same method can be called on multiple instances and each needs its own `this`.
With super expressions, the superclass is a fixed property of the *class
declaration itself*. Every time you evaluate some super expression, the
superclass is always the same class.

So for super expressions, we want to create the environment once, when the class
definition is first executed. The environment chain looks like this:

**todo: illustrate**

We need a name to store the reference in the environment. As with `this`, we use
a reserved word, now `super`, to ensure the name isn't inadvertently shadowed by
some user-defined name.

Before we can get to creating the environment at runtime, we need to handle the
corresponding scope chain in the resolver:

^code begin-super-scope (2 before, 1 after)

If the class declaration has a superclass, then we create a new scope
surrounding all of its methods. In that scope, we define the name "super". Once
we're done resolving the class's methods, we discard that scope:

^code end-super-scope (2 before, 1 after)

It's a minor optimization, but we only create the extra environment if the class
actually has a superclass. Otherwise, it would be pointless since you can only
have super expressions inside a subclass.

With "super" defined in a scope chain, we are able to resolve the super
expression itself:

^code resolve-super-expr

We resolve the `super` token exactly as if it were a variable. That stores the
number of hops along the environment chain the interpreter needs to walk to find
the environment where the superclass is stored.

This code is mirrored in the interpreter. When we evaluate a class definition,
we create a new environment if the class is a subclass:

^code begin-superclass-environment (6 before, 1 after)

Inside that environment, we store a reference to the superclass -- the actual
LoxClass object for the superclass which we have now that we are in the runtime.
Then we create the LoxFunctions for each method. Those will capture the current
environment as their closure, holding onto the superclass like we need. Once
that's done, we discard the environment:

^code end-superclass-environment (2 before, 2 after)

We're ready to interpret super expressions themselves. There's a few moving
parts, so we'll build this method up in pieces:

^code interpreter-visit-super

First, the work we've been leading up to. We look up the surrounding class's
superclass by looking up "super" in the proper environment.

When we access a method, we also need to bind `this` to the object the method is
accessed from. In an expression like `doughnut.cook`, the object is whatever we
get from evaluating `doughnut`. In a super expression like `super.cook`, the
current object is implicitly the *same* current object that we're using. In
other words, `this`. Even though we are looking up the *method* on the
superclass, the *instance* is still `this`.

Unfortunately, inside the super expression, we don't have easy access to `this`.
Fortunately, we do control the layout of the environment chains. The environment
where "this" is bound is always just inside the environment where we stored
"super":

^code super-find-this (2 before, 1 after)

Offsetting the distance by one looks up "this" in that inner environment. I
admit this isn't the most <span name="elegant">elegant</span> code in the book,
but it works.

<aside name="elegant">

One of the challenges of writing a book that includes every single line of code
for a program is that I can't hide the hacks and cut corners by leaving them as
an "exercise for the reader".

</aside>

Now we're ready to look up and bind the method, starting at the superclass:

^code super-find-method (2 before, 1 after)

This is almost exactly like the code for looking up a method for a get
expression, except that we call `findMethod()` on the superclass instead of on
the class of the current object.

That's basically it. Except, of course, that we might *fail* to find the method.
So we check for that too:

^code super-no-method (2 before, 2 after)

There you have it! Take that BostonCream example earlier and give it a try.
Assuming you and I did everything right, it should fry it first, then stuff it
with cream.

### Invalid uses of super

Like we have with previous language features, we've got an implementation that
works correctly when the user writes correct code, but we haven't bulletproofed
the intepreter against bad code. In particular, consider:

```lox
class Eclair {
  cook() {
    super.cook();
    print "Pipe full of crème pâtissière.";
  }
}
```

This class has a super expression, but no superclass. At runtime, the interpret
method for super expressions assumes that "super" was successfully resolved and
can be found in an environment. That's going to blow up here because there is no
surrounding environment for the superclass since there is no superclass. The JVM
is going to throw an exception and bring our interpreter to its knees.

Heck, there are even simpler bad uses of super:

```lox
super.notEvenInAClass();
```

We could catch these error at runtime by checking to see if the look-up of
"super" succeeded. But we can tell statically -- just by looking at the source
code -- that Eclair has no superclass and thus no super expression will work
inside it. Likewise, in the second example, we know that the super expression is
not even inside a method body.

Even though Lox is dynamically typed, that doesn't mean we want to defer
*everything* to runtime. If the user made a mistake, we'd like to help them find
it sooner rather than later. So we'll report these errors statically, in the
resolver.

First, we add a new case to the enum we use to keep track of what kind of class
is surrounding the current code being visited:

^code class-type-subclass (1 before, 1 after)

We'll use that to distinguish when we're inside a class that has a superclass
clause versus one that doesn't. When we resolve a superclass, we set that if we
found a superclass:

^code set-current-subclass (1 before, 1 after)

Then, when we resolve a super expression, we check to see that we are currently
inside a class that has a superclass:

^code invalid-super (1 before, 1 after)

If not -- oopsie! -- the user made a mistake.

## Conclusion

We made it! That final bit of error-handling are the last lines of code needed
to complete our Java implementation of Lox. This is a real accomplishment and
one you should be proud of. In the past dozen chapters and a thousand or so
lines of code, we have learned and implemented [tokens and lexing][4], [abstract
syntax trees][5], [recursive descent parsing][6], prefix and infix expressions,
runtime representation of objects, [interpreting code using the Visitor
pattern][7], [lexical scope][8], environment chains for storing variables,
[control flow][9], [functions with parameters][10], closures, [static variable
resolution and error detection][11], [classes][12], constructors, fields,
methods, and finally inheritance.

[4]: scanning.html
[5]: representing-code.html
[6]: parsing-expressions.html
[7]: evaluating-expressions.html
[8]: statements-and-state.html
[9]: control-flow.html
[10]: functions.html
[11]: resolving-and-binding.html
[12]: classes.html

We did all of that ourselves, with no external dependencies or magic tools. Just
you and I, our respective text editors, a couple of collection classes in the
Java standard library, and the JVM runtime.

This marks the end of Part II, but not the end of the book. Take a break. Maybe
write a few fun Lox programs and run them in your interpreter. (You may want to
add a couple more native methods for things like reading user input.) When
you're ready to come back, we'll embark on our next adventure.

Our Java interpreter covers a lot of territory, but it's not perfect. First, if
you run any interesting Lox programs on it, you'll discover it's
heart-breakingly slow. The style of interpretation it uses -- walking the AST
directly -- is good enough for some real-world uses, but leaves a lot to be
desired for a general-purpose scripting language. Also, we implicitly rely on
some fundamental stuff from the JVM itself. We take for granted that things like
`instanceof` in Java work *somehow*. And we never for a second worry about
memory management because the JVM's garbage collector gives us that for free.

Those were useful training wheels while we were getting using to the basic
concepts in interpreters. But now that we have those concepts down, it's time
for us to take those wheels off and build our own virtual machine from scratch
using nothing more than the C standard library...

<div class="challenges">

## Challenges

1.  Lox only supports *single inheritance* -- a class may have a single
    superclass and that's the only way to reuse methods across classes. Other
    languages have explored a variety of ways to more freely reuse and share
    capabilities between classes: mixins, traits, multiple inheritance, virtual
    inheritance, extension methods, etc.

    If you were to add one to Lox, which would you pick and why? If you're
    feeling courageous (and you should be at this point), go ahead and add it.

1.  In Lox, as in most other object-oriented languages when looking up a method,
    we start at the bottom of the class hierarchy and work our way up -- a
    subclass's method is preferred over a superclass method of the same name.
    This means subclasses override superclass methods. In order to get to the
    superclass method from within an overriding method, you use `super`.

    The language BETA takes an opposite approach. When you call a method, it
    starts at the top of the class hierarchy and works down. A superclass method
    wins over a subclass method. In order to get to the subclass method, the
    superclass method can call `inner`, which is sort of like the inverse of
    `super`. It chains to the next method down the hierarchy. If the superclass
    method doesn't call `inner`, then the subclass has no way of overriding or
    modifying the superclass's behavior.

    Take out Lox's current overriding and `super` behavior and replace it with
    BETA's semantics. In short:

    * When calling a method on a class, prefer the method *highest* on the
      class's inheritance chain.

    * Inside the body of a method, a call to `inner` looks for a method with the
      same name in the nearest subclass along the inheritance chain between the
      class containing the `inner` and the class of `this`. If there is no
      matching method, the `inner` call does nothing.

    For example:

        :::lox
        class Doughnut {
          cook() {
            print "Fry until golden brown.";
            inner();
            print "Place in a nice box."
          }
        }

        class BostonCream < Doughnut {
          cook() {
            print "Pipe full of custard and coat with chocolate.";
          }
        }

        BostonCream().cook();

    This should print:

        :::text
        Fry until golden brown.
        Pipe full of custard and coat with chocolate.
        Place in a nice box.

    Note how `inner()` lets the subclass refine the superclass's method while
    giving the superclass control over *when* the refinement happens.

1.  In the chapter where I introduced Lox, [I challenged you][challenge] to
    come up with a couple of features you think the language is missing. Now
    that you know how to build an interpreter, implement one.

[challenge]: the-lox-language.html#challenges

</div>
