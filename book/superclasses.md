^title Superclasses
^part A Bytecode Virtual Machine

> You can choose your friends but you sho' can't choose your family, an' they're
> still kin to you no matter whether you acknowledge 'em or not, and it makes
> you look right silly when you don't.
>
> <cite>Harper Lee, <em>To Kill a Mockingbird</em></cite>

This is the very last chapter where we add new functionality to our
nearly-mature bytecode VM. We've packed almost the entire Lox language in there
already. All that remains are:

*   Inheriting methods from superclasses.
*   Calling superclass methods.

We have [another chapter][optimization] after this one, but it introduces no new
behavior. It only makes existing stuff <span name="faster">faster</span>. Make
it to the end of this one, and you'll have a complete VM.

<aside name="faster">

That's not to say making stuff faster isn't important! After all, the whole
purpose of our entire second virtual machine is better performance over jlox.
You could argue that *all* of the past fifteen chapters are "optimization".

</aside>

[optimization]: optimization.html

Some of the material in this chapter will remind you of jlox. The way we resolve
super calls is pretty much the same, though viewed through clox's more complex
mechanism for storing state on the stack. But we have an entirely different,
much faster, way of handling inherited method calls this time around.

## Inheriting Methods

We'll kick things of with method inheritance since it's the simpler piece. To
refresh your memory, in Lox users specify a superclass with a less-than sign
after the class name followed by the name of the superclass:

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

After we compile the class name, if the next token is a `<` then we must have a
superclass clause. We consume the superclass's identifier token, then call
`variable()`. That function takes the previously consumed token, treats it as a
variable reference, and emits code to load the variable's value. In other words,
it looks up the superclass by name and pushes it onto the stack.

After that, we call `namedVariable()` to load the new subclass onto to the
stack, followed by an `OP_INHERIT` instruction. That instruction wires up the
superclass to the new subclass. This is similar to how we implement method
declarations where we use a series of `OP_METHOD` instructions to incrementally
build up the full class declaration.

Before we implement that new instruction, we have an edge case to detect:

^code inherit-self (1 before, 1 after)

<span name="cycle">A</span> class cannot be its own superclass. Unless you have
access to a deranged nuclear physicist and a very heavily modified DeLorean, you
cannot inherit from yourself.

<aside name="cycle">

Interestingly, with the way we implement method inheritance, I don't think
allowing cycles would actually cause any problems in clox. It wouldn't do
anything *useful*, but I don't think it would cause a crash or infinite loop.

</aside>

### Executing inheritance

Now onto the new instruction:

^code inherit-op (1 before, 1 after)

There are no operands to worry about. The two values we need -- superclass and
subclass -- are both found on the stack. That means disassembling is easy:

^code disassemble-inherit (1 before, 1 after)

The interpreter is where the action happens:

^code interpret-inherit (1 before, 2 after)

**todo: show stack layout**

The top of the stack contains the superclass and subclass, in that order. We
grab both of those and then do the inherit-y bit. This is where clox takes a
different path than jlox. In our first interpreter, each subclass stored a
reference to its superclass. When a method was accessed, if we didn't find it in
the subclass's method table, we recursed through the inheritance chain looking
at each ancestor's method table until we found it.

That's a lot of work to perform at each method *invocation* time. It's slow and,
worse, the farther an inherited method is up the ancestor chain, the slower it
gets. Not a great performance story.

The approach here is much faster. We copy all of the inherited class's methods
right down into the subclass's own method table. We do this right when the
subclass is declared. When calling a method, any method declared on a superclass
will be found immediately right in the subclass's own method table. There is no
extra runtime work needed for inheritance at all. By the time the class is
declared, the work is done. This means inherited method calls are exactly as
fast as normal method calls -- a <span name="two">single</span> hash table
lookup.

<aside name="two">

Well, two, I guess. Because first we have to make sure a field on the instance
isn't shadowing the method.

</aside>

I've sometimes heard this technique called "copy-down inheritance". It's simple
and fast, but, like most optimizations, you only get to use it under certain
constraints. It works in Lox because Lox classes are *closed*. Once a class
declaration is finished executing, the set of methods for that class can never
change.

In languages like Ruby, Python, and JavaScript, it's possible to <span
name="monkey">crack</span> open an existing class and jam some new methods into
it or even remove them. That would break our optimization because if those
modifications happened to a superclass *after* the subclass declaration
executed, the subclass would not pick up those changes. That breaks a user's
expectation that inheritance always reflects the current state of the
superclass.

<aside name="monkey">

As you can imagine, changing the set of methods a class defines imperatively at
runtime can make it hard to reason about a program. It is a very powerful tool,
but also a dangerous tool. Those who this tool maybe a little *too* dangerous
gave it the unbecoming name "monkey patching", or the even less decorous "duck
punching".

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

### Invalid superclasses

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
ObjClass, we report a runtime error to let the user know what we think of them
and their code.

## Storing Superclasses

Did you notice that when we added method inheritance we didn't actually add any
reference from a subclass to its superclass? After we copy the inherited methods
over, we forget superclass entirely. We don't actually need to keep a handle on
it, so we don't.

That won't be sufficient to support super calls. Since a subclass <span
name="may">may</span> override the superclass method, we'll need to be able to
get out hands on superclass method tables. Before we get to mechanis, I want
to refresh your memory on how super calls are statically resolved.

<aside name="may">

"May" might not a strong enough word. Presumably the method *has* been
overridden. Otherwise, why are you bothering to use `super` instead of just
calling it directly?

</aside>

Back in the halycon days of jlox, I showed you [this tricky example][example] to
explain the way super calls are dispatched:

[example]: http://localhost:8000/inheritance.html#semantics

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

Inside the body of the `test()` method, `this` is an instance of C. If super
calls were resolved relative to the superclass of the *receiver* then we would
look in B since that's C's superclass. But super calls are resolved relative to
the superclass of the class *where the super call occurs*. In this case, we are
in B's `test()` method, so the superclass is A and the program should print "A
method".

This means that super calls are not resolved dynamically. The superclass used to
look up the method is a static -- practically lexical -- property of where the
call occurs. When we added inheritance to jlox, we took advantage of that by
storing the superclass in the same Environment structure we used for all lexical
scopes. Almost as if the interpreter saw the above program like:

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

Each subclass had a hidden variable that stored a reference to its superclass.
Whenever we needed to perform a super call, we looked up the class in that
variable and told the runtime to start looking for methods there.

We'll take the same path with clox. The difference is that instead of jlox's
heap-allocated Environment class, we have the bytecode VM's value stack and
upvalue system. The machinery is a little different, but the overall effect is
the same.

### A superclass local variable

Over in the front end, compiling the superclass clause emits bytecode that loads
the superclass onto the stack. Instead of leaving that slot as a temporary, we
create a new scope and make it a local variable:

^code superclass-variable (2 before, 2 after)

Creating a new lexical scope ensures that if we declare two classes in the same
scope, each has as different local slot to store its superclass. We always name
this hidden local variable "super", so if we didn't make a scope for each
subclass, the variables would collide.

We name the variable "super" for the same reason we use "this" as the name of
the hidden local variable that `this` expressions resolve to: "super" is a
reserved word, which guarantees the compiler's hidden variable won't collide
with a user-defined one.

The difference is that when compiling `this` expressions, we conveniently have a
token sitting around whose lexeme is "this". We aren't so lucky here. Instead,
we add a little helper function to create a synthetic token for the given <span
name="constant">constant</span> string:

^code synthetic-token

<aside name="constant" class="bottom">

I say "constant string" because tokens don't do any memory management of their
lexeme. If we tried to use a heap-allocated string for this, we'd end up leaking
memory because it never gets freed. But the memory for C string literals lives
in the executable's constant data section and never needs to be freed, so we're
fine.

</aside>

Since we opened a local scope for the superclass variable, we need to close it:

^code end-superclass-scope (1 before, 2 after)

We pop the scope and discard the "super" variable after compiling the class body
and its methods. That way, the variable is accessible in all of the methods of
the subclass. It's a somewhat pointless optimization, but we only create the
scope if there *is* a superclass clause. Thus we need to only close the scope if
there is one.

To track that, we could declare a little local variable in `classDeclaration()`.
But soon other functions in the compiler will need to know whether the
surrounding class is a subclass or not. So we may as well give our future selves
a hand and store this fact as a field in the ClassCompiler now:

^code has-superclass (3 before, 1 after)

When we first initialize a ClassCompiler, we assume it is not a subclass:

^code init-has-superclass (1 before, 1 after)

Then, if we see a superclass clause, we know we are compiling a subclass:

^code set-has-superclass (1 before, 1 after)

That's the first step. Now each class that uses inheritance gets a local
variable named "super" that reliably stores a reference to the class's
superclass. That variable is available on the stack to all of the methods.
Because it uses our existing machinery for capturing local variables in
closures, the VM can even hoist the superclass reference into an upvalue if a
function inside a method captures it.

## Super Calls

We have the data we need to implement super calls. As usual, we go front to
back, starting with the new syntax. A super call <span
name="last">begins</span>, naturally enough, with the `super` keyword:

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
expression. However, the parenthesized argument list is separate. As with normal
method access, Lox supports getting a reference to a superclass method as a
closure without invoking it:

<aside name="token">

If a bare `super` token *was* an expression, what kind of object would it
evaluate to?

</aside>

```lox
class A {
  method() {
    print "A";
  }
}

class B < A {
  method() {
    var closure = super.method;
    closure(); // Prints "A".
  }
}
```

In other words, Lox doesn't really have super *call* expressions, it has super
*access* expressions. You can choose to immediately invoke the result of that if
you want. So when the compiler hits a `super` token, we consume the subsequent
`.` token and then look for a method name. Methods are looked up dynamically, so
we use `identifierConstant()` to take the lexeme of the method name token and
store it in the constant table just like we do for property access expressions.

Then the compiler has a few interesting lines of code. In order to access a
*superclass method* on *the current instance*, we need access to both the
receiver *and* the superclass of the surrounding method's class. The first
`namedVariable()` call generates code to look up the current receiver stored in
the hidden variable "this" and push it onto the stack. The second
`namedVariable()` call emits code to look up the superclass from its "super"
variable and pushes that on top.

Finally, we emit a new `OP_GET_SUPER` instruction with the constant table index
of the method being accessed as the instruction's operand. This way, the runtime
has access to the three pieces of information it needs:

*   **The instance,** stored one slot down from the top of the stack.
*   **The superclass where the method is resolved,** on top of
    the stack.
*   **The name of the method to access,** from the instruction's operand.

**todo: show stack**

We're almost ready to implement this instruction in the runtime. But before we
do, the compiler has some errors it is responsible for reporting:

^code super-errors (1 before, 1 after)

A super call is only meaningful inside the body of a method (or in a function
nested inside a method), and only inside the method of a class that has a
superclass. We detect both of these cases using the value of `currentClass`. If
that's `NULL` or points to a class with no superclass, we report those errors.

### Executing super accesses

Assuming the user didn't put a super expression in the wrong place, their code
passes from the compiler over to the runtime. We've got ourselves a new
instruction:

^code get-super-op (1 before, 1 after)

We disassemble it like other opcodes that take a constant table index operand:

^code disassemble-get-super (1 before, 1 after)

You might expect more work, but interpreting the new instruction is similar to
executing a normal property access:

^code interpret-get-super (1 before, 2 after)

As with properties, we read the method name from the constant table. Then we
pass that to `bindMethod()` which looks up the method in the given class's
method table and creates an ObjBoundMethod to bundle the resulting closure to
the current instance.

One obvious difference with the code here is that we don't try to look for a
field on the instance first. Fields are not <span name="proto">inherited</span>
so super expressions always resolve to methods.

The more useful difference is *which* class we pass to `bindMethod()`. With a
normal property access, we use the ObjInstances's own class, which gives us the
polymorphic dynamic dispatch we want. For a super call, we don't use the
instance's class. Instead, we use the statically resolved superclass of the
containing class, which the compiler has conveniently ensured is sitting on top
of the stack waiting for us.

We pop the superclass from the stack and pass it to `bindMethod()`, which does
the method lookup using the superclass's method table. That correctly skips over
any overriding methods in any of the subclasses between that superclass and the
instance's own class. It also correctly includes any methods inherited by the
superclass from any of *its* superclasses.

Aside from those differences, the rest of the behavior is the same. Popping the
superclass leaves the instance sitting on top of the stack. If `bindMethod()`
succeeds, it pops the instance and pushes the new bound method. Otherwise, it
reports a runtime error and returns `false`. In that case, we abort the
interpreter.

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

Drifting down compilation pipeline, our first stop is a new instruction:

^code super-invoke-op (1 before, 1 after)

And just past that, its disassembler support:

^code disassemble-super-invoke (1 before, 1 after)

A super invocation instruction has the same set of operands as `OP_INVOKE`, so
we reuse the same helper to disassemble it. Finally, pipeline dumps us into the
interpreter:

^code interpret-super-invoke (2 before, 1 after)

This handful of code is basically our implementation of `OP_INVOKE` mixed
together with a dash of `OP_GET_SUPER`. First, we pull out the method name and
argument count operands. Then we pop the superclass off the top of the stack so
that we can look up the method in its method table.

There is a minor difference from an `OP_GET_SUPER` followed by `OP_CALL`. In
that case, the superclass gets popped and replaced by the ObjBoundMethod for the
resolved function *before* the arguments to the call are executed. Here, the
superclass ends up on the stack on top of the arguments. But it doesn't make a
big difference because we pop it off before the arguments get passed to the
call.

**todo: show stack through call.**

We pass the superclass, method name, and argument count to our existing
`invokeFromClass()` function. That function looks up the given method on the
given class and attempts to create a call to it with the given arity. If a
method could not be found, it returns `false` and we bail out of the
interpreter. Otherwise, `invokeFromClass()` pushes a new CallFrame onto the call
stack for the method's closure. That invalidates the interpreter's cached
CallFrame pointer, so we refresh that.

## A Complete Virtual Machine

Take a look back at what we've created. By my count, we wrote around 2,500 lines
of fairly clean, straightforward C. That little program contains a complete
implementation of the Lox language with a whole precedence table full of
expression types and a suite of control flow statements. We implemented
variables, functions, closures, classes, fields, methods, and inheritance.

Even more impressive, our implementation is portable to any platform with a C
compiler, and is fast enough for real-world production use. We have a
single-pass bytecode compiler, a tight virtual machine interpreter for our
internal instruction set, compact object representations, a stack for storing
variables without heap allocation, and a precise garbage collector.

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
    name. When this happens, they will step on each other's fields and possibly
    leave you with an instance in a broken state.

    If Lox was your language, how would you address this, if at all? If you
    would change the language, implement your change.

2.  Our copy-down inheritance optimization only works in Lox because it's not
    possible to modify a class's methods after its declaration. This means we
    don't have to worry about the copied methods in the subclass getting out of
    sync with later changes to the superclass.

    Other languages like Ruby *do* allow classes to be modified after the fact.
    How do implementations of languages like that support class
    modification while keeping method resolution efficient?

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
