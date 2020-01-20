^title Methods and Initializers
^part A Bytecode Virtual Machine

> When you are on the dancefloor, there is nothing to do but dance.
>
> <cite>Umberto Eco, <em>The Mysterious Flame of Queen Loana</em></cite>

It is time for our virtual machine to bring its nascent support for objects to
life by adding behavior. That means methods and method calls. And, since they
are a special kind of method, we will add initializers too.

All of this is familiar territory from our previous jlox interpreter. What's
new in this second take is an important optimization we will implement to make
method calls over four times faster than the baseline performance and almost
six times faster than jlox. But before we get to that, we need to get the basic
support in place.

## Method Declarations

We can't optimize method calls before we have method calls, and we can't call
methods without having any methods to call, so we'll start with declarations.

### Representing methods

We'll get to compiling them, but first let's get the object model working. The
runtime representation in clox is similar to jlox. Each class stores a hash
table of methods. Keys are method names and values are functions -- ObjClosures
specifically -- that represent the bodies of each method.

We already have a hash table implementation, so this part is easy:

^code class-methods (3 before, 1 after)

Whenever the runtime creates a new class, it initializes the empty method table:

^code init-methods (1 before, 1 after)

The ObjClass struct owns the memory for this table, so when the class is freed
by the memory manager, the table should be too:

^code free-methods (1 before, 1 after)

Speaking of memory managers, the GC needs to trace through classes into the
method table. If a class is still reachable (likely through some instance),
then all of its methods certainly need to stick around too:

^code mark-methods (1 before, 1 after)

We use the existing `markTable()` function which traces through both the key
strings -- since strings are managed too -- and the value for each entry.
The representation is pretty obvious. The interesting part is how we *create*
it.

In jlox, the interpreter had access to the entire AST node for the class
declaration and all of the methods it contains. At runtime, the interpreter
simply walked that list of declarations.

Now every piece of information the compiler wants to shunt over to the runtime
has to squeeze through the interface of a flat series of bytecode instructions.
How do we take a class declaration, which can contain an arbitrarily large set
of methods, and represent it as bytecode? Let's see...

### Compiling method declarations

The last chapter left us with a compiler that parses classes but only allows an
empty body. Now we expand that to compile a series of method declarations
between the braces:

^code class-body (1 before, 1 after)

Lox doesn't have field declarations, so anything before the closing brace that
ends the class body must be a method declaration. We stop compiling methods when
we hit that final `}` or if we reach the end of the file. The latter check
ensures our compiler doesn't get stuck in an infinite loop if the user
accidentally forgets the closing brace.

Before we dig into the implementation of `method()`, let's think about how it
might work. The problem is that a class may declare any number of methods
and somehow the runtime needs to look up and bind all of them. Packing all of
that information into a single `OP_CLASS` instruction would be a challenge.

Instead, the bytecode representation of a class declaration splits creating a
class and its methods into a <span name="series">*series*</span> of
instructions.

<aside name="series">

We used a similar approach when we implemented closures. The `OP_CLOSURE`
instruction needs to know the type and index for each captured upvalue. We
encoded that using a series of pseudo-instructions following the main
`OP_CLOSURE` instruction. The VM processed all of those in a loop when it
found the first first `OP_CLOSURE` instruction.

Here our approach is a little different because from the VM's perspective,
each instruction to define a method is a separate standalone operation.

</aside>

First, the compiler emits an `OP_CLASS` instruction. That creates a new empty
ObjClass object. After that, the existing class declaration code emits
instructions to store that in the appropriate named variable.

Then, for each method declaration, we emit a new `OP_METHOD` instruction that
adds a single method to that class. When all of the `OP_METHOD` instructions
have executed, we're left with a fully formed class. In other words, while the
user sees a class declaration as a single atomic operation, the VM will
implement it as a series of mutations.

To bind a method, the VM needs three things:

1.  The name of the method.
1.  The closure for the method body.
1.  The class to bind the method to.

We'll build up our implementation to see how those all get plumbed through to
the runtime, starting with this:

^code method

Like `OP_GET_PROPERTY` and other instructions that need names at runtime, the
compiler adds the identifier token's lexeme to the constant table, gettin back
the index. Then we emit an `OP_METHOD` instruction with that index as the
operand. That's the name. Next is the method body:

^code method-body (1 before, 1 after)

We use the same `function()` helper that we use when compiling function
declarations. That utility function compiles subsequent the parameter list and
function body. Then it emits the right code to end up with the resulting
ObjClosure on top of the stack. At runtime, the VM will find the closure there.

Last is the class to bind the method to. Where can the VM find that?
Unfortunately, by the time we reach the `OP_METHOD` instruction, we don't know
where it is. It <span name="global">could</span> be on top of the stack, if the
user declared the class inside a function or block. But if it's at the
top-level, the ObjClass is no longer on the stack. It's been stored in the
global variable table and then popped.

<aside name="global">

It Lox only supported declaring classes at the top level, the VM could assume
that any class could be found by looking it up directly from the global
variable table. Alas, because we support local classes, we need to handle that
case too.

</aside>

Fear not. The compiler does know the *name* of the class. We can capture it
right when we parse the name token:

^code class-name (1 before, 1 after)

And we know that no other declaration with that name could possibly shadow it.
So we'll do the easy fix. Right before we start binding methods, we'll emit
whatever code is necessary to load the class back on top of the stack:

^code load-class (2 before, 1 after)

Right before compiling the class body, we <span name="load">call</span>
`namedVariable()`. That helper function generates whatever code is needed to
load a variable with the given token's lexeme onto the stack. Then we compile
the methods.

<aside name="load">

The preceding call to `defineVariable()` pops the class, so it seems silly to
call `namedVariable()` to load it right back onto the stack. Why collapse those
two and leave it on the stack?

We could, but in the [next chapter][super] we will insert code between these two
calls to support inheritance. At that point, it will be simpler if the class
isn't sitting around on the stack.

[super]: superclasses.html

</aside>

This means that when we execute each `OP_METHOD` instruction, the top of the
stack will contain the class followed by the closure for the method. Once we've
reached the end of the methods, we no longer need the class and pop it off the
stack:

**todo: illustrate**

^code pop-class (1 before, 2 after)

The compiler is doing its job, so let's move over to the runtime.

### Executing method declarations

First we have a new instruction to define:

^code method-op (1 before, 1 after)

We disassemble it like other instructions that have string constant operands:

^code disassemble-method (2 before, 1 after)

The interpreter delegates to a new helper function:

^code interpret-method (1 before, 1 after)

It reads the method name from the constant table and then passes it to:

^code define-method

The closure is on top of the stack with the class being modified right under it.
We read from those two stack slots and then store the closure in the class's
method table. Then we pop the closure since we're done with it.

We don't pop the class because there may be other method declarations after
this one. The compiler inserted a final `OP_POP` at the end of the class body
to clean up that stack slot.

Note that we don't do any <span name="verify">runtime</span> type checking on
the closure or class object. That `AS_CLASS()` call is safe because the compiler
itself generated the code that ensures the object in that stack slot is actually
a class. The VM trusts its own compiler.

<aside name="verify">

The VM can trust that the bytecode it executes is valid because the *only* way
to get code to it is by going through clox's own compiler. Many bytecode VMs
like the JVM and CPython support executing bytecode that has been compiled
separately.

That leads to a different security story. Maliciously crafted bytecode could
crash the VM or worse. To avoid that, the JVM does a bytecode verification pass
before it executes any loaded code. CPython says it's up to the user to ensure
any bytecode they run is safe.

</aside>

Once the series of `OP_METHOD` instructions is run, the VM will have a nicely
populated class ready to start doing things. Next is pulling those methods back
out.

## Method References

Most of the time, methods are accessed and immediately called, leading to this
familiar syntax:

```lox
instance.method(argument);
```

But remember in Lox (and some other languages), those two steps are distinct
and can be separated:

```lox
var closure = instance.method;
closure(argument);
```

Since users *can* separate the operations, we need to implement separately. The
first is using our existing dotted property syntax to access a method defined
on the instance's class. That should return some kind of object that the user
can then call like a function.

The obvious approach would be to look up the method in the class's method table
and return the ObjClosure associated with that name. But we also need to
remember that when you access a method, `this` gets bound to the instance the
method was accessed from. Here's the example from when we added methods to jlox:

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

This needs to print "Jane", so whatever object that gets called on the last
line, it somehow needs to remember the instance from the previous line. In jlox,
we implemented that "memory" using that interpreter's existing heap-allocated
Environment class, which was used for all variable storage.

Our bytecode VM has a more complex architecture for storing state. Local
variables and temporaries are the stack, globals in a hash table, and variables
in closures use upvalues. That necessitates a somewhat more complex solution,
and a new runtime type.

**todo: link to previous chapters**

### Bound methods

When the user executes a method access, we'll find the closure for that method
and wrap it in a new <span name="bound">"bound method"</span> object that stores
the closure and the receiver that the method was accessed from. This bound
object can be called like a function. When it is, the VM will do some
shenanigans to correctly wire up `this` to point to the receiver when the method
body executes.

**todo: illustrate bound fn pointing to method closure**

<aside name="bound">

I took the name "bound method" from CPython. Python behaves similar to Lox here
and I used its implementation for inspiration.

</aside>

Here's the new object type:

^code obj-bound-method (2 before, 1 after)

It wraps the receiver and the method closure together. The type of the receiver
is Value. We could use a pointer to an ObjInstance because those are the only
kind of objects that can ever be the receivers of method calls. But by the time
we get to bound methods, VM doesn't actually care what kind of receiver it has,
so we'll go with the more general type so we don't have to keep converting the
pointer back to a Value.

Next we have the usual boilerplate for adding a new object type. A new case
in the object type enum:

^code obj-type-bound-method (1 before, 1 after)

A macro to check a value's type:

^code is-bound-method (2 before, 1 after)

One to cast it to an ObjBoundMethod pointer:

^code as-bound-method (2 before, 1 after)

We declare a function to create a new ObjBoundMethod:

^code new-bound-method-h (2 before, 1 after)

And implement it here:

^code new-bound-method

It simply stores the given closure and receiver. When the bound method is no
longer needed, we free it:

^code free-bound-method (1 before, 1 after)

The bound method has a couple of references, but it doesn't *own* them, so it
frees nothing but itself. However, those references do need to be traced by the
garbage collector:

^code blacken-bound-method (1 before, 1 after)

This <span name="trace">ensures</span> that a handle to a method keeps the
receiver around in memory so that `this` can still access it when you invoke the
handle later. We also trace the method closure.

<aside name="trace">

Tracing the method closure isn't really necessary. The receiver is an
ObjInstance which has a pointer to its ObjClass, which has a table for all of
the methods. But it feels dubious to me in some vague to have ObjBoundMethod
rely on that.

</aside>

The last operation all objects support is printing:

^code print-bound-method (1 before, 1 after)

A bound method prints exactly the same was a function. From the user's
perspective, a bound method *is* a function. It's another object they can call.
We don't expose that the VM implements using a special object type.

Speaking last things, ObjBoundMethod is the very last runtime type we'll add to
clox. You've written your last `IS_` and `AS_` macros. We're only a few chapters
from the end of the book and we're getting close to a complete VM.

### Accessing methods

Let's bolt our new object type doing something. Methods are accessed using the
same "dot" property syntax we implemented in the last chapter. So the compiler
is already parsing the right expressions and emitting `OP_GET_PROPERTY`
instructions for them. The only changes we need to make are in the runtime.

When that instruction executes, the receiver is on top of the stack. The
instruction's job is to find a field or method with the given name and replace
the top of the stack with the accessed property.

The interpreter handles fields, so we simply extend the `OP_GET_PROPERTY` case
another section:

^code get-method (5 before, 1 after)

We insert this after the code to look up a field on the receiver instance.
Fields take priority over and shadow methods, so we check for them first. If
the instance does not have a field with the given property name, then it may
instead refer to a method.

We take the instance's class and pass it to a new `bindMethod()` helper. That
function returns `true` if it found a method and placed the result on the stack.
Otherwise it returns `false` to indicate a method with that name couldn't be
found. Since the name also wasn't a field, that means we have a runtime error,
which aborts the interpreter.

The fun stuff happens here:

^code bind-method

First we look for a method with the given name in the class's method table. If
we didn't find one, we report a runtime error and bail out. Otherwise, we take
the method and wrap it in a new ObjBoundMethod. We grab the receiver that the
method was accessed on from the top of the stack where the compiler has ensured
we can find it. Finally, we pop the instance and replace the top of the stack
with the bound method.

**todo: illustrate**

### Calling methods

That gets a bound method onto the stack, but the user can't <span
name="do">*do*</span> anything useful with it. The operation we're missing is
being able to call a bound method like a function. That operation is implemented
in `callValue()`, so we add a case there for the new object type:

<aside name="do">

It *is* a first class value, so they can store it in variables, pass it to
functions, and otherwise do "value"-y stuff with it.

</aside>

^code call-bound-method (1 before, 1 after)

We pull the raw closure back out of the ObjBoundMethod and use the existing
`call()` helper to being invoking that closure by pushing a CallFrame for it
onto the call stack. With that, we can now run Lox programs like:

```lox
class Scone {
  topping() {
    print "clotted cream";
  }
}

var scone = Scone();
scone.topping();
```

That's three big steps. We can declare, access, and invoke methods. But
something is missing. We went to all that trouble to wrap the method closure in
an object that binds the receiver, but when we invoke the method here, we don't
use that receiver at all.

## This

Before we bind the receiver in method calls, the compiler needs to actually
make them useful. Right now, method bodies have no way to access the receiver.
The way Lox exposes that is through `this` expressions. Until we add those,
there's no point in binding the receiver in a method call because the method
can't get to the receiver anyway.

The lexer already treats `this` as a special token type, so the first step is
wiring that up in the parse table:

^code table-this (1 before, 1 after)

When the parser encounters a `this` in prefix position, it dispatches to:

^code this

We'll use the same approach for `this` in clox as we did in jlox. We'll treat
it as a lexically-scoped local variable whose value gets magically initialized.
Treating it like a local variable means we get a lot of behavior for tree. In
particular, closures that close over `this` will do the right thing and capture
the receiver in an upvalue.

When this function gets called, the `this` token has just been consumed and is
stored as the previous token. We call the existing `variable()` function which
we use to compile identifier expressions for variable accesses. It takes a
single Boolean parameter for whether the compiler should look for a following
`=` operator and parse a setter. You can't assign to `this`, so we pass `false`
to disallow that.

The `variable()` function doesn't care that `this` has its own token type and
isn't an identifier. It is happy to treat the lexeme "this" as if it were a
variable name and then look it up using the existing scope resolution machinery.

Of course, that lookup will fail because we never declared any variable whose
name is "this". To do that, we need to think about where the receiver should
live in memory.

At least until they get captured by closures, clox stores every local variable
on the stack. When the compiler begins compiling a function, it sets aside stack
slot zero by declaring a local variable named "". At runtime, this slot holds
the function being called.

Storing the function there isn't super useful -- we could have simply shifted
the called function's stack window one slot forward to exclude the function.
But you can guess where this is leading. For *method* calls, we can instead use
that slot to store the receiver. We don't need to keep the closure for the
method on the value stack since it will be in the CallFrame stack too.

Slot zero stores the instance that `this` is bound to. In order to compile
`this` expressions, the compiler simply needs to give the right name to that
local variable slot:

^code slot-zero (1 before, 1 after)

We only want to do this for methods. Function declarations don't have a `this`.
And, in fact, they need to *not* declare a variable named "this", so that if
you write a `this `expression inside a function declaration which is itself
inside a method, the `this` expression resolves to the outer method:

```lox
class Nested {
  method() {
    fun function() {
      print this;
    }

    function();
  }
}

Nested().method();
```

This program should print "Nested instance". This implies the compiler must
know whether it's compiling a function declaration or method declaration, so
we add a new case to our FunctionType enum to distinguish those:

^code method-type-enum (1 before, 1 after)

When we compile a method, we use that type:

^code method-type (2 before, 1 after)

Now the compiler thinks that "this" is a local variable stored in slot zero. We
can correctly compile references to that variable and the compiler will emit
the right `OP_GET_LOCAL` instructions to access it. Closures can even capture
`this` and store the receiver in upvalues. Pretty cool.

Except that the receiver isn't actually *in* slot zero. The compiler depends on
the runtime to set that up when a bound method is invoked. It looks like this:

^code store-receiver (2 before, 2 after)

**todo: illustrate stack layout for function versus method call**

When a method is called, the top of the stack contains all of the parameters
and then just under those is the closure of the called method. That's where
slot zero in the new CallFrame will be. Since `stackTop` points just *past* the
last used stack slot, we count back one to get to the parameters and then skip
past those too.

With that, methods now feel like real methods since they can access and modify
the state of the instance their bound to. We have some real object-orientation
going on.

### Misusing this

Our VM correctly supports users correctly using `this`, but we also need to make
sure it properly handles users *mis*-using `this`. Lox says it is a compile
error for a `this` expression to appear outside of the body of a method. These
two wrong uses should be caught by the compiler:

```lox
print this; // At top level.

fun notMethod() {
  print this; // In a function.
}
```

So how does the compiler know if it's inside a method? The obvious answer is to
look at the FunctionType of the current Compiler. We did literally just add an
enum case there to treat methods specially. That wouldn't correctly handle
code like the earlier example where you are inside a function which is itself
nested inside a method. To fix that, we could walk the chain of Compilers to
see if any of them have type `TYPE_METHOD`.

That would fine. However, before too long we're going to need to access
information about the enclosing class, so now is a good time to put some extra
machinery into the compiler to track the current enclosing class, if any:

^code current-class (1 before, 2 after)

We can then use that to implicitly tell if you're inside a method. It's overkill
right now, but it will be worth it when we add inheritance. The new type looks
like this:

^code class-compiler-struct (1 before, 2 after)

Right now we store only the class's name. We also keep a pointer to the
ClassCompiler for the enclosing class, if any. Nesting a class declaration
inside a method in some other class is a strange thing to do, but Lox supports
it. Just like the Compiler struct, this means ClassCompiler forms a linked list
from the current innermost class being compiled out through all of the enclosing
classes.

If we aren't inside any class declaration at all, the module variable
`currentClass` is `NULL`. When the compiler begins compiling a class, it pushes
a new ClassCompiler onto that implict linked stack:

^code create-class-compiler (2 before, 1 after)

The memory for the ClassCompiler struct lives right on the C stack, a handy
capability we get by writing our compiler as a recursive descent parser. At the
end of the class body, we pop that compiler off the stack and restore the
enclosing one:

^code pop-enclosing (1 before, 1 after)

When an outermost class body ends, `enclosing` will be `NULL`, so this resets
`currentClass` to `NULL`. This we can check for that to see if we are inside a
class -- and thus inside a method -- when attempting to compile a `this`
expression:

^code this-outside-class (1 before, 1 after)

And now `this` outside of a class is correctly forbidden.

## Instance Initializers

The reason object-oriented languages bundle state and behavior together -- one
of the real tenets of the paradigm -- is so to ensure that objects are always in
a valid, meaningful state. If the only way to touch an object's state is <span
name="through">through</span> its methods, the methods can make sure nothing
goes awry. But that presumes the object is *already* in a proper state. What
about when it's first created?

<aside name="through">

Of course, Lox does let outside code directly access and modify an instance's
fields without going through its methods. This is unlike Ruby and Smalltalk
which tightly encapsulate state inside objects. Our toy scripting language,
alas, isn't quite so principled.

</aside>

Object-oriented languages also need a mechanism so that brand new objects are
properly set up and they do that through constructors which both produce a new
instance and initialize its state. In Lox, allocating the raw instance is
handled by the runtime and the user's class may provide an initializer method
that is called to set up any fields on the bare instance.

Initializers work mostly like normal methods, with a few tweaks:

*   The runtime automatically invokes the initializer method whenever an
    instance of a class is created.

*   After the initializer finished, the new <span name="return">instance</span>
    is returned to the code that constructed the object. The initializer method
    doesn't need to explicitly return `this`.

*   In fact, an initializer is prohibited from returning any value at all since
    the runtime handles that for you.

<aside name="return">

It's almost like the initializer is implicitly wrapped in a bundle of code like:

```lox
fun create(klass) {
  var obj = newInstance(klass);
  obj.init();
  return obj;
}
```

Note how the value returned by `init()` is discarded.

</aside>

Since we already support methods, to support initializers, we just need to
implement those three special rules. We'll go in order.

### Invoking initializers

First, automatically calling `init()` on new instances:

^code call-init (1 before, 1 after)

After the runtime allocates the new instance, we look for an `init()` method on
the class. If it defines one, we initiate a call to it. The existing `call()`
function pushes a new CallFrame for the initializer's closure. Any arguments
passed to the class when we called it to construct the instance are still
sitting on the stack above the instance. The new CallFrame for the `init()`
method shares that same stack window so those arguments implictly get forwarded
to `init()`.

**todo: illustrate**

Lox doesn't require classes to define an initializer. If omitted, the runtime
simply returns the new uninitialized instance. However, if there is no `init()`
method, then it doesn't make any sense to pass arguments to the class when
creating the instance. That's an error:

^code no-init-arity-error (2 before, 1 after)

When there's no `init()` method you can't pass arguments when constructing an
instance. It's as if you get a default `init()` method defined like:

```lox
init() {}
```

We also need to ensure that the expected number of arguments are passed when a
class *does* provide an initializer. Forunately, the `call()` to that does that
for us already.

To call the initializer, the runtime looks up the `init()` method by name. We
want that to be fast since it happens every time an instance is constructed.
That means it would be good to take advantage of the string interning we've
already implemented. To do that, the VM creates an ObjString for "init" and
keeps it around so it can reuse it. It lives right in the VM struct:

^code vm-init-string (1 before, 1 after)

The runtime creates the string when the VM boots up:

^code init-init-string (1 before, 2 after)

We want it to stick around, so the GC considers it a root:

^code mark-init-string (1 before, 1 after)

And we don't free it until the entire VM is shutting down:

^code clear-init-string (1 before, 1 after)

That's the first bullet point.

### Initializer return values

The next step is ensuring that the initializer returns the new instance and not
`nil` or something else. Right now, if a class defines an initializer then when
an instance is constructed, the VM pushes a call to that initializer onto the
CallFrame stack.

Then it just keeps on trucking. The user's invocation on the class to create the
instance will complete whenever that initializer method returns and will leave
on the stack whatever value the initializer returns. That means that unless the
user takes care to put `return this;` at the end of the initializer, no instance
will come out. Not very helpful.

To fix this, whenever the front end compiles an initializer method, it will
emit different bytecode at the end of the body to return `this` from the method
instead of the usual implicit `nil` most functions return. In order to do *that*,
the compiler needs to actually know when it is compiling an initializer.

We detect that by checking to see if the name of the method we're compiling is
"init":

^code initializer-name (1 before, 1 after)

Because this <span name="mouthful">function</span> in the compiler is for
compiling methods and not functions, we don't need to worry about accidentally
applying this logic when compiling a *function* named "init". We use a new
function type to represent the fact that we're compiling an initializer:

<aside name="mouthful">

Wow, that sentence is a mouthful.

</aside>

^code initializer-type-enum (1 before, 1 after)

Then whenever the compiler emits the implicit return at the end of a body, we
use that to insert the initializer-specific behavior:

^code return-this (1 before, 1 after)

In an initializer, instead of pushing `nil` onto the stack before returning,
we load slot zero, which contains the instance. This `emitReturn()` function is
also called when compiling a return statement without a value, so this also
correctly handles cases where the user does an early return inside an
initializer.

The last step, the last bullet point in our list of special features of
initializers, is making it an error to try to return anything *else* from an
initializer. Now that the compiler tracks the method type, this is
straightforward:

^code return-from-init (3 before, 1 after)

We report an error if a return statement in an initializer has a value. We still
go ahead and compile the value afterwards so that the compiler doesn't get
confused by the trailing expression and report a bunch of cascaded errors.

Aside from inheritance, which we'll get to soon, we now have a pretty
full-featured class system working in clox:

```lox
class CoffeeMaker {
  init(coffee) {
    this.coffee = coffee;
  }

  brew() {
    print "Enjoy your cup of " + this.coffee;

    // No reusing the grounds!
    this.coffee = nil;
  }
}

var maker = CoffeeMaker("coffee and chicory");
maker.brew();
```

Pretty sophisticated for a few thousand lines of simple C code!

## Optimized Invocations

Our VM correctly implements the language's semantics for method calls. We could
stop here. But a big part of the reason we are implementing a bytecode virtual
machine is to get better performance than our old Java interpreter had. Right
now, method calls even in clox are pretty slow.

Lox's semantics define a method invocation as two operations -- accessing the
method and then calling the result. Our VM must support those as separate
operations because the user *can* separate them. You can access a method without
calling and then invoke the bound method later. Nothing we've implemented so far
is unnecessary.

But *always* treating those as separate operations has a significant cost. Every
single time a Lox program calls a method, the runtime heap allocates a new
ObjBoundMethod, initializes its fields, then pulls them right back out.
Eventually, the GC has to spend some time freeing all of those ephemeral bound
methods.

Most of the time, a Lox program accesses a method and and then immediately calls
it. The bound method is created by one bytecode instruction and then consumed by
the very next one. In fact, it's so immediate that the compiler can even
textually *see* that it's happening -- a dotted property access followed by an
opening parenthesis is most likely a method call.

Since we can recognize this pair of operations in the compiler, we have the
opportunity to emit a <span name="super">new special</span> instruction that
performs an optimized method call.

<aside name="super">

Recognizing that a few specific bytecode instructions often occur one after the
other and creating a new single instruction -- called a **superinstruction** --
that fuses those into one operation is a classic optimization technique in
bytecode VMs.

One of the largest performance drains in a bytecode interpreter is the overhead
of decoding and dispatching each instruction. Fusing several instructions into
one eliminates some of that.

The challenge is determining *which* instruction sequences are common enough to
benefit from this optimization. Every new superinstruction claims an opcode for
its own use and there are only so many of those to go around. Add too many and
you'll need a larger encoding for opcodes which then increases code size and can
make decoding *all* instructions slower.

</aside>

We start in the compiler in the function that parses dotted property
expressions:

^code parse-call (3 before, 1 after)

After the compiler has parsed the identifier after the `.` we look for a left
parenthesis. If we match one, we switch to a new code path. There, we compile
the argument list exactly like we do when compiling a call expression. Then we
emit a single new `OP_INVOKE` instruction. It takes two operands:

1.  The first is the index of the property name in the constant table.
2.  The second is the number of arguments being passed to the method.

In other words, this single instruction combines the operands of the
`OP_GET_PROPERTY` and `OP_CALL` instructions it replaces, in that order. It
really is a fusion of those two instructions. Let's define it:

^code invoke-op (1 before, 1 after)

And add it to the disassembler:

^code disassemble-invoke (2 before, 1 after)

This is a new, special, instruction format, so it needs a little custom
attention in the disassembler:

^code invoke-instruction

We read the two operands and then print out both the method name and the
argument count.

This instruction gets its own case in the interpreter's bytecode dispatch loop:

^code interpret-invoke (4 before, 1 after)

Most of the work happens in `invoke()`, which we'll get to. Here, we look up the
method name from the operand and read the argument count operand. Then we hand
off to `invoke()` to do the heavy lifting. That function returns `true` if the
invocation succeeded. If it returns `false`, a runtime error occurred. We check
for that here and abort the interpreter if it happens.

If the invocation succeeded, then there is a new CallFrame on the stack, so we
need to refresh our cached copy of the current frame.

The interesting work happens here:

^code invoke

**todo: illustrate stack layout**

First we grab the receiver off the stack. The arguments passed to the method are
above it on the stack, so we peek that many slots down. As with
`OP_GET_PROPERTY` instructions, we also need to handle the case where a user
incorrectly tries to call a method on a value that isn't an instance:

^code invoke-check-type (1 before, 1 after)

<span name="helper">That's</span> a runtime error, so we report that and bail
out. Otherwise, we get the instance's class and jump over to this other new
utility function:

<aside name="helper">

As you can guess by now, we split this code into a separate function because
we're going to reuse it later -- in this case for `super` calls.

</aside>

^code invoke-from-class

This function combines the logic of how the VM implements `OP_GET_PROPERTY` and
`OP_CALL` instructions, in that order. First we look up the method by name in
the class's method table. If we don't find one, we report that runtime error and
exit.

Otherwise, we take the method's closure and push a call to it onto the CallFrame
stack. We don't need to heap allocate and initialize an ObjBoundMethod. In fact,
we don't even need to juggle anything on the stack. The receiver and method
arguments are already right where they need to be. This is, in fact, why we used
stack slot zero to store the reciver -- it's how the caller already organizes
the stack for a method call.

If you fire up the VM and run a little program that calls methods now, you
should see the exact same behavior as before. But, if we did our job right, the
*performance* should be much improved. I wrote a little micro-benchmark that
does nothing but a bunch of method calls in a row. On my laptop, without the new
`OP_INVOKE` instruction, it runs in 3.13 seconds. With this new optimization,
it's down to 0.69 seconds. That's 4.5 *times* faster, which is a huge
improvement when it comes to programming language optimization, and particularly
impressive given that our bytecode VM is already much faster than jlox.

### Invoking fields

The fundamental creed of optimization is: "Thou shalt not break correctness."
<span name="monte">Users</span> like it when a language implementation gives
them an answer faster, but only if it's the *right* answer. Alas, our
implementation of faster method invocations fails to uphold that tenet:

```lox
class Oops {
  init() {
    fun f() {
      print "not a method";
    }

    this.field = f;
  }
}

var oops = Oops();
oops.field();
```

The last line looks like a method call. The compiler thinks that it is and
dutifully emits an `OP_INVOKE` instruction for it. However, it's not. What is
actually happening is a *field* access that returns a function which then gets
called. Our VM will do who knows what when it tries to run it.

<aside name="monte">

There are cases where users may be satisfied with a program sometimes returns
the wrong answer in return for running significantly faster or with a better
bound on the performance. These are the field of [**Monte Carlo
algorithms**][monte]. For some use cases, this is a good trade-off.

[monte]: https://en.wikipedia.org/wiki/Monte_Carlo_algorithm

The important part, though, is that the user is *choosing* to apply one of these
algorithms. We language implementers can't unilaterally decide to sacrifice
their program's correctness.

</aside>

Earlier when we implemented `OP_GET_PROPERTY`, we handled both field and method
accesses. To fix this, we need to do the same thing for `OP_INVOKE`:

^code invoke-field (1 before, 1 after)

Pretty simple fix. Before looking up a method on the instance's class, we look
for a field with the same name. If we find a field, then we store it on the
stack in place of the receiver, *under* the argument list. This is how
`OP_GET_PROPERTY` behaves since the latter instruction executes *before* any
subsequent parenthesized list of arguments has been evaluated.

Then we try to call that field's value like the callable that it hopefully is.
The `callValue()` helper will check the value's type and call it as appropriate
or report a runtime error if the field's value wasn't a callable type.

That's all it takes to make our optimization fully safe. We do sacrifice a
little performance, unfortunately. But that's the price you have to pay
sometimes. There is always a frustration with optimizations you *could* do if
only the language wouldn't allow some annoying corner case. But, as language
<span name="designer">implementers</span>, we have to play the game we're given.

<aside name="designer">

As language *designers*, our role is very different. If we do control the
language itself, we may sometimes choose to restrict or change the language in
ways that enable optimizations. Users want expressive languages, but they also
want fast implementations. Sometimes it is good language design to sacrifice a
little power if you can give them perf in return.

</aside>

The code we wrote here is an example of a very common pattern in implementing
language optimizations:

1.  Recognize a common operation or sequence of operations that is performance
    critical. In this case, it is a property access followed by a call.

2.  Add an optimized implementation of that pattern to the VM. That's our
    `OP_INVOKE` instruction.

3.  Before the optimization kicks in, guard it with some conditional logic that
    validates that the pattern actually applies. If it does, stay on the fast
    path. Otherwise, fall back to the slower but more robust unoptimized
    behavior. Here, that means checking that we are actually calling a method
    and not accessing a field.

As your language work moves from getting the implementation working *at all* to
getting it to work *faster*, you will find yourself spending more and more
time looking for patterns like this and adding guarded optimizations for them.
Full-time VM engineers spend much of their careers in this loop.

But we can stop here for now. With this, clox now supports most of the features
of object-oriented programming, and with respectable performance.

<div class="challenges">

## Challenges

1.  Looking up the class's `init()` method every time an instance is created is
    a constant time operation, but still fairly slow. Implement something
    faster. Write a benchmark and measure the performance difference.

1.  In dynamically-typed languages like Lox, a single callsite that invokes a
    method could resolve to a variety of methods on a large number of classes
    throughout a program's run. Even so, in practice, most of the time a single
    a callsite ends up calling the exact same method on the exact same class
    when a program is run. Most calls are actually *not* polymorphic even if the
    language says they can be.

    How do advanced language implementations take advance of that for
    performance?

1.  When interpreting an `OP_INVOKE` instruction, the VM has to do two hash
    table lookups. First it must look for a field that could shadow a method and
    only if that fails can it look for a method. The former check is rarely
    useful -- most fields do not contain functions.

    But it is *necessary* because the language says fields and methods are
    accessed the same way and fields shadow methods. That is a language *choice*
    that affects the performance of our implementation. Was it the right choice?
    If Lox were your language, what would you do?

</div>

<div class="design-note">

## Design Note: Novelty Budget

I still remember the first time I wrote a tiny BASIC program on a TRS-80 and
made a computer do something it hadn't done before. It felt like a superpower.
The first time I cobbled together just enough of a parser and interpreter to let
me write a tiny program in *my own language* that made a computer do a thing was
like some sort of higher-order meta-superpower. It's a wonderful feeling.

When I realized I could design a language any way I wanted that looked and
behaved however I chose, it was like the shackles of every programming language
I'd ever used fell away. Like I'd been going to a private school that required
uniforms my whole life and then one day transferred to a public school where I
could wear whatever I wanted.

Wait, I don't need to use curly braces for blocks? I could do objects but no
classes. Multiple inheritance and multimethods? A dynamic language that
overloads statically by arity?

Naturally, I took that freedom and ran with it. I made the weirdest, most
arbitrary language design decisions. Apostrophes for generics. No commas between
arguments. Overload resolution that can fail at runtime. I did things
differently just for difference's sake.

This is a very fun experience, that I highly recommend. We need more weird,
avante garde programming languages. I want to see more art languages. I still
make oddball toy languages for fun sometimes.

*However*, if your goal is success where "success" is defined as a large number
of users, then your priorities must be different. In that case, your primary
goal is to have your language loaded into the brains of as many people as
possible. That's *really hard*. It takes a lot of human effort to move a
language's syntax and semantics from a computer into trillions neurons.

Programmers are naturally conservative with their time and cautious about what
languages work to upload into their wetware. They don't want to waste their time
on a language that ends up not being useful to them. As a language designer,
your goal is thus to give them as much language power as you can with as little
required learning as possible.

One natural approach is *simplicity*. The fewer concepts and features your
language has, the less total volume of stuff there is to learn. This is one of
the reasons minimal <span name="dynamic">scripting</span> languages often find
success even though they aren't as power as the big industrial languages -- they
are easier to get started with.

<aside name="dynamic">

In particular, this is a big advantage of dynamically-typed languages. A static
language requires you to learn *two* languages -- the static and runtime
semantics -- before you can get to the point where you are making the computer
do stuff. Dynamic languages only require you to learn the latter.

Eventually, programs gets big enough that the value of static analysis makes it
worth the effort to learn that second static language, but the value proposition
isn't as obvious at the outset.

</aside>

The problem with simplicity is that simply cutting features often sacrifices
power and expressiveness. There is an art to finding features that punch above
their weight, but often minimal languages simply do less.

There is another path that avoids much of that problem. The trick is to realize
that a user doesn't have to load your entire language into their head, *just the
part they don't already have in there.* As I mentioned in an [earlier design
note][note], learning is about transferring the *diff* between what they already
know and what they need to know.

[note]: parsing-expressions.html#design-note

Many potential users of your language already know some other programming
language. Any features your language shares with that language are essentially
"free" when it comes to learning. It's already in their head, they just have to
recognize that your language does the same thing.

In other words, *familiarity* is another key tool to lower the adoption cost of
your language. Of course, if you fully maximize that attribute, the end result
is a language that is completely identical to some existing language. That's
not a recipe for success because at that point there's no incentive for users
to switch to your language at all.

So you do need to provide some compelling differences. Some things your language
can do that other languages can't or at least can't do as well. I believe this
is one of the fundamental balancing acts of language design:

*   Reducing differences from other languages lowers learning cost.
*   Increases them raises the compelling advantage of the language.

I think of this balancing act in terms of a "novelty budget", or as Steve
Klabnik calls it, a "[strangeness budget][]". Users have a low threshold for the
total amount of new stuff they are willing to accept to learn a new language.
Exceed that and they won't show up.

[strangeness budget]: https://words.steveklabnik.com/the-language-strangeness-budget

Anytime you add something new to your language that other languages don't have,
or anytime you do something other languages can do but in a different way, you
spend some of that budget. That's OK -- you *need* to spend it to make your
language compelling. But your goal is to spend it *wisely*. For each feature or
difference, ask yourself how much compelling power it adds to your language and
then evaluate critically whether it pays its way. Is the change so valuable that
it is worth blowing some of your novelty budget?

In practice, I find this means that you end up being pretty conservative with
syntax and more adventurous with semantics. As fun as it is to put on a new
change of clothes, swapping out curly braces for blocks with some other
punctuation characters is very unlikely to add much real power and
expressiveness to the language. But it does spend some novelty. So it's hard for
syntax differences to carry their weight.

On the other hand, new semantics can significantly increase the power of the
language. Multimethods, mixins, traits, dependent types, metaprogramming, etc.
can radically level up what a user can do with the language.

Alas, being conservative like this is not as fun as just changing everything.
But it's up to you to decide whether you want to chase mainstream success or not
in the first place. We don't all need to be radio-friendly pop bands. If you
want your language to be like free jazz or drone metal and are happy with the
proportionally smaller (but likely more devoted) audience size, go for it.

</div>
