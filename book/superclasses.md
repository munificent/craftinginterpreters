^title Superclasses
^part A Bytecode Virtual Machine

> You can choose your friends but you sho' can't choose your family, an' they're
> still kin to you no matter whether you acknowledge 'em or not, and it makes
> you look right silly when you don't.
>
> <cite>Harper Lee, <em>To Kill a Mockingbird</em></cite>

This is the very last chapter where we add new functionality to our
rapidly-maturing bytecode VM. We've packed almost the entire Lox language in
there already. All that remains is:

*   Inheriting methods from superclasses.
*   Calling superclass methods.

We have another chapter after this one, but it introduces no new behavior. It
just makes existing stuff faster. Make it to the end of this one, and you'll
have a complete VM.

The approach we take here for inheritance and super calls should be about half
familiar from our earlier Java interpreter. The way we resolve super calls is
pretty much the same, though viewed through clox's more complex mechanism for
storing state on the stack. We have an entirely different, much faster, way of
handling inherited method calls.

## Inheriting Methods

We'll kick things of with method inheritance since it's the simpler piece. To
refresh your memory, in Lox users specify a superclass by placing a less-than
sign after the class name followed by the name of the superclass:

```lox
class Doughnut {
  cook() {
    print "Dunk in the fryer.";
  }
}

class JellyDoughnut < Doughnut {
  filling() {
    print "Raspberry jelly";
  }
}
```

Here, the JellyDoughnut class inherits from Doughnut and thus instances of
JellyDoughnut inherit the `cook()` method. I don't know why I'm belaboring this.
You know how inheritance works. Let's start compiling the new syntax:

^code compile-superclass (2 before, 1 after)

After we compile the class name, if the next token is a `<` then we know we have
a superclass clause. We consume the superclass's identifier token, then call
`variable()`. That function takes the previously consumed token and treats it as
a variable reference. It emits code to read a previously-declared variable with
that name. In other words, it looks up the superclass by name and pushes the
result onto the stack. In most cases, the superclass will be in a global
variable, so this will emit an `OP_GET_GLOBAL` instruction.

After that, we call `namedVariable()` to load the newly-declared subclass onto
to the stack, followed by an `OP_INHERIT` instruction. The approach we're taking
here is similar to how we handled method declarations. The compiler emits an
`OP_CLASS` instruction to create a new bare class. After that, it emits a series
of instructions to incrementally build up all of the capabilities of the class.
In the last chapter, that was `OP_METHOD` instructions. Now, we insert a new
`OP_INHERIT` instruction after the `OP_CLASS` and before the methods. That
instruction tells the interpreter to take the new class and wire it up to
inherit from the superclass we loaded onto the stack.

Before we get to that runtime support, we have an edge case to detect:

^code inherit-self (1 before, 1 after)

<span name="cycle">A</span> class cannot be its own superclass. Unless you have
access to a wingnut nuclear physicist and a very heavily modified DeLorean, you
cannot inherit from yourself.

<aside name="cycle">

Interestingly, with the way we implement method inheritance, I don't think
allowing cycles would actually cause any problems in clox. It wouldn't do
anything *useful*, but I don't think it would cause any crashes or infinite
loops.

</aside>

### Executing inheritance

Let's get out new instruction working:

^code inherit-op (1 before, 1 after)

It has no operands. The two values it works with are the superclass and subclass
and it gets both of those from the stack. That means disassembling is easy:

^code disassemble-inherit (1 before, 1 after)

The interpreter is where the real action happens:

^code interpret-inherit (1 before, 2 after)

**todo: show stack layout**

The top of the stack has the superclass with the new subclass on top of it. We
grab both of those and then do the inherit-y bit. This is where clox takes a
different path than jlox. In our first interpreter, each subclass stored a
reference to its superclass. When a method was accessed, if we didn't find it in
the subclass's method table, we recursed through the inheritance chain looking
at each ancestor's method table until we found it.

This was a lot of work that happened at each method *invocation* time. It was
slow and, worse, the farther an inherited method was up the ancestor chain, the
slower it was to call. Not a great performance story.

The approach here is much faster. We simply *copy* all of the inherited class's
methods right down into the subclass's own method table. We do this right when
the subclass is declared. When calling a method, there is no extra runtime work
needed for inheritance at all. By the time the class is declared, the work is
done. This means inherited method calls are exactly as fast as normal method
calls -- a <span name="two">single</span> hash table lookup.

<aside name="two">

Well, two, I guess. Because first we have to make sure a field on the instance
isn't shadowing the method.

</aside>

I've sometimes heard this technique called "copy-down inheritance". It's simple
and fast, but, like most optimizations, you only get to use it under certain
constraints. It works in Lox because classes in Lox are *closed*. Once a class
declaration is finished executing, the set of methods for that class can never
change.

In languages like Ruby, Python, and JavaScript, it's possible to <span
name="monkey">crack</span> open an existing class and jam some new methods into
it or even remove them. That would break our optimization because if those
modifications happened to a superclass *after* the subclass declaration
executed, the subclass would not pick up those changes. That break's a user's
implicit expectation that inheritance always reflects the current set of methods
in the superclass.

<aside name="monkey">

As you can imagine, changing the set of methods a class defines imperatively at
runtime, can make it pretty hard to reason a program. It is a very powerful
tool, but also a dangerous tool. Those who feel it's maybe a little *too*
dangerous call it the unbecoming name "monkey patching", or the even less
decorous "duck punching".

</aside>

Fortunately for us (but not for users who like the feature, I guess), Lox
doesn't let you patch monkeys or punch ducks, so we can safely apply this
optimization.

What about method overrides? Won't copying the superclass's methods into the
subclass's method table clash with the subclass's own methods? Fortunately, no.
We emit the `OP_INHERIT` after the `OP_CLASS` instruction that creates the
subclass but before any method declarations and `OP_METHOD` instructions have
been compiled. At the point that we copy the superclass's methods down, the
subclass's method table is empty. Any overrides the subclass declares after that
will simply overwrite the superclass's entries in the table.

Our implementation is simple and fast, which is just the way I like my VM code
to go. But it's not *correct*. There's nothing preventing the user from trying
to inherit from an object that isn't a class at all:

```lox
var NotClass = "So not a class";
class OhNo < NotClass {}
```

Obviously, no self-respecting programmer would write that, but we have to guard
against potential Lox users who have no self respect. A simple runtime check
fixes that:

^code inherit-non-class (1 before, 2 after)

If the value we loaded from the identifier in the superclass clause isn't an
ObjClass, we report a runtime error to let the user know what we think of them.

## Storing Superclasses

Did you notice that when we added method inheritance we didn't actually add any
reference from a subclass to its superclass? Once we copy the inherited methods
over, the superclass is forgotten entirely. We don't need to keep a handle on
it, so we don't.

But to support super calls, we need to be able to find methods coming from the
superclass that <span name="may">may</span> have been overridden by the
subclass. We'll have to keep track of each class's superclass somehow. A fun
refresher, here's the gnarly Lox program we examined to work through how super
calls are resolved in jlox:

<aside name="may">

"May" might not a strong enough word. Presumably the method *has* been
overridden. Otherwise, why are you bothering to use super instead of just
calling it directly?

</aside>

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

We create a C and call the `test()` method it inherited from B. Inside the body
of that method `this` is an instance of C. We make a super call to `method()`.
If super calls were resolved relative to the superclass of the *receiver* then
we would look in B since that's C's superclass. But super calls should be
resolved relative to the superclass of the class *where the super call occurs*.
In this case, we are in B's `test()` method, so the superclass is A. A correct
Lox implementation should thus print "A method".

This means that super calls are not resolved dynamically. The superclass used to
look up the method is a static -- practically lexical -- property of where the
call occurs. When we added inheritance to jlox, we took advantage of that by
storing the superclass in the same Environment structure we used for all lexical
scopes.

It's almost as if the interpreter saw the above program like:

```lox
class A {
  method() {
    print "A method";
  }
}

var Bs_super = A;
class B < A {
  method() {
    print "B method";
  }

  test() {
    runtimeSuperCall(Bs_super, "method");
  }
}

var Cs_super = B;
class C < B {}

C().test();
```

Each subclass has a hidden variable that stores a reference to its superclass.
Whenever we need to perform a super call, we look up the class in that variable
and tell the runtime to start looking for methods there.

We'll take the same path with clox. The difference is that instead of jlox's
heap-allocated Environment class, we have the bytecode VM's value stack and
upvalue system. The machinery is a little different, but the overall effect is
the same.

### A superclass local variable

Over in the compiler, after we compile the superclass clause, we have emitted
bytecode that will leave the superclass object on top of the stack. Instead of
leaving it as a temporary, we tell the compiler to declare that slot as a local
variable:

^code superclass-variable (2 before, 2 after)

First, we create a new lexical scope. First, it ensures we are in a *local*
scope. At the top level, variables go into the global table, not the stack.
Also, it makes sure that if we declare two classes in the same scope, each has
as different local slot to store its superclass. We always name this hidden
local variable "super", so if we didn't make a scope for each subclass, the
variables would collide.

We use "super" as the name for the variable for the same reason we use "this" as
the name of the hidden local variable that `this` expressions resolve to: it's a
reserved word, which guarantees the compiler's hidden variable won't collide
with a user-defined one.

The difference is that with `this` expressions, we did actually have a token
sitting around whose lexeme was "this". We aren't so lucky here. Instead, we add
a little helper function to create a synthetic token for the given <span
name="constant">constant</span> string:

^code synthetic-token

<aside name="constant">

I say "constant string" because tokens don't do any memory management of their
lexeme. If we tried to use a heap-allocated string for this, we'd end up leaking
memory because it never gets freed. But the memory for C string literals lives
in the executable's constant data section and never needs to be freed, so we're
fine.

</aside>

Since we've opened a new local scope for the superclass variable, we need to
close that scope:

^code end-superclass-scope (1 before, 2 after)

We pop it after compiling the class body and its methods. That way, the variable
is accessible in all of the methods of the subclass. It's a somewhat pointless
optimization, but we only create the scope if there *is* a superclass clause.
Thus we need to only close the scope if there is one.

To track that, we could declare a little local variable in `classDeclaration()`.
But soon other functions in the compiler will need to know whether the
surrounding class is a subclass or not. So we may as well give our future selves
a hand and store this as a field in the ClassCompiler instead:

^code has-superclass (3 before, 1 after)

When we first initialize a ClassCompiler, we assume it is not a subclass:

^code init-has-superclass (1 before, 1 after)

Then, we if see a superclass clause, we know we are compiling a subclass:

^code set-has-superclass (1 before, 1 after)

That's enough for the first step. With this, each class that uses inheritance
gets a local variable named "super" that reliably stores a reference to the
class's superclass. That variable is available on the stack to all of the
methods. Because it uses our existing machinery for capturing local variables in
closures, the VM can even hoist the superclass reference into an upvalue if a
method needs to capture it.

## Super Calls

With that superclass reference hanging around in memory where we can get to it,
we are ready to implement super calls. As <span name="last">usual</span>, we go
front to back, starting with the new syntax. A super call begins, naturally
enough, with the `super` keyword:

<aside name="last">

This is it, friend. The very last entry you'll add to the parsing table.

</aside>

^code table-super (1 before, 1 after)

When the expression parser lands on that keyword, it hands off control to this
parsing function:

^code super

This is pretty different from how we compiled `this` expressions. Unlike `this`,
a `super` <span name="token">token</span> is not a standalone expression.
Instead, the dot and method name following it are inseparable parts of the
expression. However, the parenthesized argument list is *not* core to the
expression. As with methods, Lox supports getting a reference to a superclass
method as a closure without immediately invoking it:

<aside name="token">

If a bare `super` token *was* an expression, what kind of object would it
evaluate to?

</aside>

```lox
var closure = super.method();
closure();
```

In other words, Lox doesn't really have super *call* expressions, it has super
*access* expressions. You can then just choose to immediately invoke the result
of that if you want. So when the compiler hits a `super` token, we consume the
subsequent `.` token and then look for a method name. Methods are looked up
dynamically, so we use `identifierConstant()` to take the lexeme of the method
name token and store it in the constant table just like we do for property
access expressions.

Then the compiler has a few interesting lines of code. In order to access a
*superclass method* on *the current instance*, we need access to both the
receiver *and* the superclass of the surrounding method's class. The first
`namedVariable()` call generates code to look up the current receiver which is
stored in the hidden variable named "this" and push it onto the stack. The
second `namedVariable()` call emits code to look up the superclass from its
"super" hidden variable and push that onto the stack next.

Finally, we emit a new `OP_GET_SUPER` instruction with the constant table index
of the method being accessed as the instruction's operand. This way, the runtime
has access to the three pieces of information it needs:

*   **The instance,** stored one slot down from the top of the stack.
*   **The superclass where it starts looking for the method,** on top of
    the stack.
*   **The name of the method to resolve,** from the instruction's operand.

**todo: show stack**

We're almost ready to implement this instruction in the runtime. But before we
do, the compiler has some errors it is responsible for reporting:

^code super-errors (1 before, 1 after)

A super call is only meaningful inside the body of a method (or in a function
nested inside a method), and only inside the method of a class that has a
superclass. We can detect both of these cases using `currentClass`. If that's
`NULL` or points to a class with no superclass, we report those errors.

### Executing super accesses

Assuming the user didn't make one of those errors, their code passes from the
compiler over to the runtime. We've got ourselves a new instruction:

^code get-super-op (1 before, 1 after)

It takes a single operand that indexes into the constant table, so we
disassemble it like other similar opcodes:

^code disassemble-get-super (1 before, 1 after)

You might expect more work, but interpreting the new instruction is very similar
to executing a normal property access:

^code interpret-get-super (1 before, 2 after)

As with properties, we read the method name from the constant table. Then we
pass that to `bindMethod()` which looks up the method in the class's method
table and creates an ObjBoundMethod to bundle the resulting closure to the
current instance.

One obvious difference with the code here is that we don't try to look for a
field on the instance first. Fields are not <span name="proto">inherited</span>
so super expressions always access methods.

The more useful difference is *which* class we pass to `bindMethod()`. With a
normal property access, we use the ObjInstances's own class, which gives us the
polymorphic dynamic dispatch we want there. For a super call, we don't use the
instance's class. Instead, we use the statically resolved superclass of the
containing class, which the compiler has conveniently ensured is sitting on top
of the stack waiting for us.

We pop the superclass from the stack and pass it to `bindMethod()`, which does
the method lookup on that class. That correctly skips over any overriding
methods in any of the subclasses between that superclass and the instance's own
class.

Aside from those difference, the rest of the behavior is the same. Popping the
superclass leaves the instance sitting on top of the stack. If `bindMethod()`
succeeds, it pops the instance and replaces it with the bound method it created.
Otherwise, it reports a runtime error and returns `false`. In that case, we
abort the interpreter.

### Faster super calls

We have superclass method accesses working now. And since the returned object is
an ObjBoundMethod that you can then invoke, we've got super *calls* working too.
Just like last chapter, we've reached a point where our VM has the complete
correct semantics.

But, also like last chapter, it's pretty slow. Again, we're heap allocating an
ObjBoundMethod for each super call even though most of the time the very next
instruction is an `OP_CALL` that immediately unpacks that bound method, invokes
it, and then discards it. In fact, this is even more likely to be true for
super calls than for regular method calls. At least with method calls there is
a chance that the user is actually invoking a function stored in a field. With
super calls, you're *always* looking up a method. The only question is whether
you invoke it immediately or not.

The compiler can certainly answer that question for itself if it sees a left
parenthesis after the superclass method name, so we'll go ahead and perform the
same optimization we did for method calls. Take out the two lines of code that
load the superclass and emit `OP_GET_SUPER` and replace them with this:

^code super-invoke (1 before, 1 after)

Now before we emit anything, we look for a parenthesized argument list. If we
find one, we compile that. Then we load the superclass. After that, we emit a
new `OP_SUPER_INVOKE` instruction. This <span
name="superinstruction">superinstruction</span> combines the behavior of
`OP_GET_SUPER` and `OP_CALL`, so it takes two operands: the constant table index
of the method name to lookup and the number of arguments to pass to it.

<aside name="superinstruction">

This is a particularly *super* superinstruction, if you get what I'm saying.
I... I'm sorry for this terrible joke.

</aside>

Otherwise, if we don't find a `(`, then we continue to compile the expression as
a super access like we did before and emit an `OP_GET_SUPER`.

Taking a quick tour through the compilation pipeline, first up on the left is
a new instruction:

^code super-invoke-op (1 before, 1 after)

And to your right, its disassembler support:

^code disassemble-super-invoke (1 before, 1 after)

A super invocation instruction has the same set of operands as `OP_INVOKE`, so
we reuse the same helper to disassemble it. Finally, our tour disembarks at the
interpreter:

^code interpret-super-invoke (2 before, 1 after)

This handful of code is basically our implementation of `OP_INVOKE` mixed
together with a dash of `OP_GET_SUPER`. First, we pull out the method name and
argument count operands.

Then we pop the superclass off the top of the stack so that we can look up the
method in its method table. This is a little different from how an
`OP_GET_SUPER` followed by `OP_CALL` looks. In that latter case, the superclass
gets popped and replaced by the ObjBoundMethod for the resolved function
*before* any of the arguments to the call get compiled. Here, the superclass
ends up on the stack on top of the arguments. But it doesn't make a big
difference because we pop it off before the arguments get used.

**todo: show stack through call.**

We pass all of that to our existing `invokeFromClass()` function. That function
looks up the given method on the given class and attempts to create a call to it
with the given arity. If a method could not be found, it returns `false` and we
bail out of the interpreter. Otherwise, `invokeFromClass()` pushes a new
CallFrame onto the call stack for the method's closure. That invalidates the
interpreter's cached CallFrame pointer, so we refresh that.

Take a look back at what we've created. By my count, we wrote around 2,500 lines
of fairly clean, straightforward C. That contains a complete implementation of
the Lox language with a whole precedence table full of expression types, and a
suite of flow statements. We implemented variables, functions, closures,
classes, fields, methods, and inheritance.

Even more impressive, our implementation is portable to any platform with a C
compiler -- read, just about every one -- and is fast enough for real-world
production use. We have a single-pass bytecode compiler, a tight virtual machine
interpreter for our internal instruction set, and compact object
representations, a stack for storing variables without heap allocation, and a
precise garbage collector.

If you go out and start poking around in the implementations of Lua, Python, or
Ruby, you will be surprised by how much of it now looks familiar to you. You
have seriously leveled up your knowledge of how programming languages work,
which in turn gives you a deeper understanding of programming itself. It's like
you used to be a taxi driver and now you can pop the hood and repair the engine
on your car too.

You can stop here if you like. The two implementations of Lox you have are
complete and full-featured. You built the car and can drive it wherever you like
now. But if you want to have some more fun tuning and tweaking it for even
greater performance out on the racetrack, there is one more chapter. We don't
add any new capabilities, but we roll in a couple of classic optimizations to
squeeze even more perf out of it. If that sounds fun, [keep reading][opt]...

[opt]: optimization.html

<div class="challenges">

## Challenges

1.  A tenet of object-oriented programming is that a class should ensure new
    objects are in a valid state. In Lox, that means defining an initializer
    that populates the instance's fields. Inheritance complicates this because
    the instance must be in a valid state according to all of the classes in
    the object's inheritance chain.

    The easy part is remembering to call `super.init()` in each subclass's
    `init()` method. The harder part is fields. There is nothing preventing two
    classes in the inheritance chain from accidentally claiming the same field
    name. When this happens, they will step on each other's state and possibly
    leave you with an instance in a broken state.

    If Lox was your language, how would you address this, if at all? If you
    would change the language, implement your change.

2.  Our copy-down inheritance optimization only works in Lox because it's not
    possible to modify a class's methods after its declaration. This means we
    don't have to worry about the copied methods in the subclass getting out of
    sync with later changes to the superclass.

    Other languages like Ruby *do* allow classes to be modified after the fact.
    How do implementations of languages like that balance supporting class
    modification with wanting efficient inherited method resolution?

3.  In the [jlox chapter on inheritance][inheritance], we had a challenge to
    implement the BETA language's approach to method overriding. Solve the
    challenge again, but this time in clox. Here's the description of the
    previous challenge:

    In Lox, as in most other object-oriented languages, when looking up a
    method, we start at the bottom of the class hierarchy and work our way up --
    a subclass's method is preferred over a superclass's. In order to get to the
    superclass method from within an overriding method, you use `super`.

    The language [BETA][] takes the [opposite approach][inner]. When you call a
    method, it starts at the *top* of the class hierarchy and works *down*. A
    superclass method wins over a subclass method. In order to get to the
    subclass method, the superclass method can call `inner`, which is sort of
    like the inverse of `super`. It chains to the next method down the
    hierarchy.

    The superclass method controls when and where the subclass is allowed to
    refine its behavior. If the superclass method doesn't call `inner` at all,
    then the subclass has no way of overriding or modifying the superclass's
    behavior.

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
            print "Place in a nice box.";
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

    Since clox is about not just implementing Lox, but doing so with good
    performance, this time around try to solve the challenge with an eye towards
    efficiency.

[inheritance]: inheritance.html
[inner]: http://journal.stuffwithstuff.com/2012/12/19/the-impoliteness-of-overriding-methods/
[beta]: http://cs.au.dk/~beta/

</div>
