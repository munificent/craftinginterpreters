^title Classes and Instances
^part A Bytecode Virtual Machine

**todo: figure out illustrations**

**todo: the jlox chapter makes a distinction between "properties" and "fields".
also uses "get expressions" (as well as "getter" sometimes), "set expressions".
make this consistent with that.**

**todo: explain property/field distinction.**

> Caring too much for objects can destroy you. Only -- if you care for a thing
> enough, it takes on a life of its own, doesn’t it? And isn’t the whole point
> of things -- beautiful things -- that they connect you to some larger beauty?
>
> <cite>Donna Tartt, <em>The Goldfinch</em></cite>

The last major feature that we have left to implement in clox is object-oriented
programming. It's really a bundle of intertwined features: classes, instances,
fields, methods, initializers, and inheritance. Using relatively high-level
Java, we managed to get through all of those in two chapters. Now that we're
coding in C, which feels like building a model of the Eiffel tower using
toothpicks, we will spend three chapters to cover the same territory.

This should make for a relaxed, leisurely stroll through the implementation.
After streneous chapters like [closures][] and the [garbage collector][], we
have earned a rest. And, in fact, the rest of the book should be easy from here
on out. You can't do anything in object-orientation without objects, so we'll
start there. In this chapter, we'll cover the first three sub-features: classes,
instances, and fields. This is the stateful side of object-orientation. Then in
the next two chapters, we will hang behavior and code reuse off of those
objects.

[closures]: closures.html
[garbage collection]: garbage-collection.html

## Class Objects

In a class-based object-oriented language, everything begins with classes. They
define what sorts of objects exist in the program and are the factories used to
produce new instances. We'll start with their runtime representation and then
hook that into the language.

By this point, we're well-acquainted with the process of adding a new object
type to the VM. We start with a struct:

^code obj-class (1 before, 2 after)

After the header, we store the class's name. This isn't strictly needed for the
user's program, but let's us print the class's name at runtime for things like
stack traces.

This new type needs a corresponding case in the ObjType enum:

^code obj-type-class (1 before, 1 after)

And that type gets a corresponding pair of macros. First for testing an object's
type:

^code is-class (2 before, 1 after)

And then for casting a Value to an ObjClass point:

^code as-class (2 before, 1 after)

The VM creates new class object using this function:

^code new-class-h (2 before, 1 after)

The implementation lives over here:

^code new-class

Pretty much all boilerplate. It takes in the <span name="klass">class's</span>
name as a string and stores it.

<aside name="klass">

I named the local variable "klass" not just to give the VM a zany preschool
"kidz korner" feel. While "class" is not a reserved word in C, it is in C++. You
can compile clox as both C and C++, so we need to not step on any of C++'s
reserved words either.

</aside>

When the VM no longer needs a class, it frees it like so:

^code free-class (1 before, 1 after)

We have a memory manager know, so we also need to support tracing through class
objects:

^code blacken-class (1 before, 1 after)

When the GC reaches a class object, we mark the name to keep that string alive
too.

The last operation the VM can perform on a class is printing it:

^code print-class (1 before, 1 after)

## Class Declarations

Runtime representation in hand, we are ready to add support for classes to the
language. As usual, we start at the front-end in the parser:

^code match-class (1 before, 1 after)

Class declarations are statements and the parser recognizes one by the leading
`class` keyword. The rest of the parsing happens over in:

^code class-declaration

Immediately after the `class` keyword is the class's name. We take that
identifier and add it to the surrounding function's constant table as a string.
Classes are first-class objects in Lox and if you print one, we want to show
the class's name, so the compiler needs to stuff it somewhere that the runtime
can find. The constant table is the canonical way to do that.

The class's <span name="variable">name</span> is also used to bind the class
object to a variable with that same name. So we declare that name as a variable
right after parsing it.

<aside name="variable">

We could have made class declarations be *expressions* instead of statements.
Then users would have to explicitly bind the class to a variable themselves
like:

```lox
var Pie = class Pie {}
```

Sort of like lambda functions but for classes. But since we generally want
classes to be named anyway, it makes sense to treat them as declarations.

</aside>

Next we emit a new instruction to actually create the class object at runtime.
That instruction takes the constant table index of the class's name as an
operand.

After that, but before compiling the body of the class, we define the variable
for the class's name. Declaring the variable adds it to the scope but remember
from the previous chapter that we can't *use* the variable until its defined.
For classes, we define the variable before the body so that methods can refer to
their containing class. That's useful for things like factory methods.

Finally, we compile the body. We don't have methods yet, so right now it's
simply an empty pair of braces. Lox doesn't require fields to be declared in
the class, so we're done with the body for now.

The compiler is emitting a new instruction, so let's define that:

^code class-op (1 before, 1 after)

And add it to the disassembler:

^code disassemble-class (2 before, 1 after)

For such a large-seeming feature, the interpreter support is easy:

^code interpret-class (3 before, 1 after)

We load the string for the class's name from the constant table and pass that
to `newClass()`. That creates a new class object with the given name. We push
that onto the stack and we're done. If the class is bound to a global variable,
then the compiler's call to `defineVariable()` will emit code to store that
object from the stack into the global variable table. Otherwise, it's right
where it needs to be on the stack for a new local variable.

Our VM supports classes now. You can do this:

```lox
class Brioche {}
print Brioche;
```

Unfortunately, that's about *all* you can do with classes, so next is making
them more useful.

## Instances of Classes

Classes serve two main purposes in a language:

1.  **They are how you create new instances.** Sometimes this involves a `new`
    keyword, other times it's a method call on the class object, but you usually
    mention the class by name *somehow* to get a new instance.

2.  **They contain methods.** These define how all instances of the class
    behave.

We won't get to methods until the next chapter, so for now we will just handle
the first part. Before classes can create instances, we need a representation
for them:

^code obj-instance (1 before, 2 after)

Each instance has a pointer to the class that it is an instance of. Instances
know their class. We won't use this much in this chapter, but it will become
important when we add methods.

More critically in this chapter is how instances store their state. Lox lets
users <span name="free">freely</span> add fields to an instance at runtime. This
means we need a storage mechanism that is flexible and can grow. We could use a
dynamic array, but we also want to be able to look up fields by name as quickly
as possible. There's a data structure that's just perfect for quickly accessing
a set of values by name and -- even more convenient -- we've already implemented
it. Each instance stores its fields using a hash table.

<aside name="free">

Being able to freely add fields to an object at runtime is a big practical
difference between most dynamic and static languages. Statically-typed languages
usually require fields to be explicitly declared. This way, the compiler knows
exactly what fields each instance has. That means at compile time it can
determine the exact amount of memory needed for each instance and the offsets in
that memory where each field can be found.

In Lox and other dynamic languages, accessing a field is usually a hash table
lookup. Constant time, but still pretty heavyweight. In a language like C++,
accessing a field is as fast as adding a constant integer to a point to get to
the field's address in the object.

**todo: overlap**

</aside>

We only need to add an include and we've got it:

^code object-include-table (1 before, 1 after)

This new struct gets a new object type:

^code obj-type-instance (1 before, 1 after)

This point is where Lox's notion of "type" and the VM's *implementation* notion
of type brush against each other in ways that can be confusing. Inside the C
code that makes clox, there are a number of different types of Obj -- ObjString,
ObjClosure, etc. In the Lox *language*, users can define their own classes and
then there are instances of many different classes.

But, from the VM's perspective every class the user defines is simply another
value of type ObjClass. Likewise, each instance in the user's program, no matter
what class it is an instance of, is an ObjInstance. That one VM object type
covers instances of all classes.

We also get our usual macros:

^code is-instance (1 before, 1 after)

And:

^code as-instance (1 before, 1 after)

Since fields are added after the instance is created, the "constructor" function
only needs to know the class:

^code new-instance-h (1 before, 1 after)

We implement it here:

^code new-instance

We store a reference to the instance's class. Then we initialize the field
table to an empty hash table. A new baby object is born!

At the sadder end of an instance's lifespan, it gets freed:

^code free-instance (4 before, 1 after)

The instance owns its field table, when freeing memory for the instance, we also
free the table.

Speaking of memory management, we also need to support instances in the garbage
collector. Fields are the most important source of indirect references to other
objects. Most live objects that are not roots are alive because some instance
references the object in a field. So tracing instances is paramount:

^code blacken-instance (4 before, 1 after)

If the instance is alive, we need to keep its class around too. And, more
importantly, we need to keep every object it references in a field. Fortunately,
we already have a nice `markTable()` to make that easier.

Less critical but still important is printing:

^code print-instance (3 before, 1 after)

<span name="print">An</span> instance prints its name followed by "instance".
(The "instance" part is mainly so that classes and instances don't print the
same.)

<aside name="print">

Most object-oriented languages let a class define some sort of `toString()`
method that lets the class specify how its instances are converted to a string
and printed. If Lox was less of a toy language, I would want to support that
too.

</aside>

The real fun happens over in the interpreter. Lox has no special `new` keyword.
The way to create an instance of a class is to invoke the class itself as if it
were a function. The runtime already supports function calls and in there it
tests the type of object being called to ensure you don't try to invoke a number
or other invalid type.

We extend that with a new valid case:

^code call-class (1 before, 1 after)

If the value being called -- the object that results when evaluating the
expression to the left of the opening parenthesis -- is a class, then we treat
it as a constructor call. We <span name="args">create</span> a new instance of
the called class and store the result in the stack.

<aside name="args">

We ignore any arguments passed to the class for now. We'll revisit this in the
[next chapter][next] when we add support for initializers.

[next]: methods-and-initializers.html

</aside>

We're one step farther. Now we can define classes and create instances of them:

```lox
class Brioche {}
print Brioche;
```

Note the parentheses after `Brioche` on the second line now. This prints
"Brioche instance".

## Fields, Getters, and Setters

The past piece in the object-oriented puzzle we'll place in this chapter is
state. The ObjInstance struct has a field table already, so we simply need to
hook it up to the language. Lox uses the classic "dot" syntax for accessing and
assigning fields:

```lox
eclair.filling = "pastry creme";
print eclair.filling;
```

The period works <span name="sort">sort</span> of like an infix operator. There
is an expression to the left that is evaluated first and produces an instance.
After that is the `.` followed by a field name. Since there is a preceding
operand, we hook this into the parse table as an infix expression:

<aside name="sort">

I say "sort of" because the right-hand side after the `.` is not an expression,
but a single identifier whose semantics are handled by the getter or setter
expression itself. It's really closer to a postfix expression.

</aside>

^code table-dot (1 before, 1 after)

Like in other languages, the `.` operator binds tightly, with precedence as
high as the `(` in a function call. After the parser consumes a `.` token, it
dispatches to:

^code compile-dot

The parser expects to find a field name immediately after the dot. Much like
with the class name, we load that token's lexeme into the constant table as a
string so that the name is available at runtime.

We have two new expression forms -- getters and setters -- that this one
function handles. If we see an equals sign after the field name, it must be a
setter expression that is assigning to a field. We don't *always* allow an
equals sign after the field to be compiled though. Consider:

```lox
a + b.c = 3
```

This is syntactically invalid according to Lox's grammar. If `dot()` silently
parsed the `= 3` part, it would incorrectly interpret this as if you'd written:

```lox
a + (b.c = 3)
```

The problem is that a setter expression has much lower precedence than a simple
field access getter expression. The parser may call `dot()` in a context that is
too high precedence to permit a setter to appear. To avoid incorrectly allowing
that, we only parse and compile a subsequent `=` if `canAssign` is true. If an
equals token appears when `canAssign` is false, `dot()` leaves it alone and
returns. That eventually unwinds up to `parsePrecedence()` which reports an
error on the unexpected `=`, which is what we want.

If we found an `=` in a context where it is allowed, we next compile the
right-hand expression being stored in the field. Then we emit an
`OP_SET_PROPERTY` instruction. That takes a single operand for the index of the
field name in the constant table. If we didn't compile a setter, we assume it's
a field access and emit an `OP_GET_PROPERTY` instruction, which also takes an
operand for the field name.

We should probably define these two new instructions:

^code property-ops (1 before, 1 after)

And go ahead and add support for disassembling them:

^code disassemble-property-ops (1 before, 1 after)

### Interpreting getter and setter expressions

Now we'll slide over to the runtime. We'll start with getters since those are
a little simpler.

^code interpret-get-property (3 before, 2 after)

The the interpreter reaches this instruction, the expression to the left of the
`.` has already been executed and left the resulting instance on top of the
stack. We read the field name from the constant table and look up the named
field in the instance's field table. If the hash table contains a field with
that name, we pop the instance and push the field's value as the result.

Of course, the field might not exist. In Lox, we've defined that to be a runtime
error. So we add a check for that and abort if it happens:

^code get-undefined (3 before, 3 after)

There is another failure mode to handle which you've probably noticed. The above
code assumes the expression to the left of the dot did evaluate to an
ObjInstance. But there's nothing preventing a user from writing:

```lox
var obj = "not an instance";
print obj.field;
```

<span name="field">The</span> user's program is wrong, but the VM still has to
handle it with some grace. Right now, it will misinterpret the bits of the
ObjString as an ObjInstance and, I don't know, catch on fire or something
definitely not graceful.

In Lox, only instances are allowed to have fields. You can't stuff a field onto
a string or number. So we need to check that the value is an instance before
accessing any fields on it:

<aside name="field">

We *could* support adding fields to values of other types. It's our language and
we can do what we want. But it's very likely a bad idea. It significantly
complicates the implementation in ways and hurt performance. And it raises very
tricky semantic questions around the equality and identity of values.

If I attach a field to the number `3`, does the result of `1 + 2` have that
field as well? If so, how does the implementation track that? If not, are those
two resulting "threes" still considered equal?

**todo: overlap**

</aside>

^code get-not-instance (1 before, 1 after)

If the value on the stack isn't an instance, we report a runtime error and
safely exit.

Of course, getter expressions are not very useful when no instances have any
fields. For that we need setters:

^code interpret-set-property (3 before, 2 after)

This is a little more complex than `OP_GET_PROPERTY`. On top of the stack, we
first have the instance whose field is being set and then above that is the
value that is being stored in the instance's field. Like before, we read the
instruction's operand and find the field name string. Using that, we store the
value on top of the stack into the instance's field table.

After that is a little stack juggling. We pop the stored value off, then pop
the instance, and finally push the value back on. In other words, we remove
the *second* element from the stack while leaving the top alone. This is the
result of a set expression is the assigned value, so it needs to stay on the
stack:

**todo: illustrate**

```lox
class Toast {}
var toast = Toast();
print toast.jam = "raspberry";
```

This prints "raspberry".

Unlike when reading a field, we don't need to worry about the hash table not
containing the field. A setter implicitly creates the field if needed. We do
need to handle the user incorrectly trying to store a field on a value that
isn't an instance:

^code set-not-instance (1 before, 1 after)

Exactly like with get expressions, we check the value's type and report a
runtime error if it's invalid. And, with that, the stateful side of Lox's
support for object-oriented programming is in place. Give it a try:

```lox
class Box {}

var box = Box();
box.a = 1;
box.b = 2;
print box.a + box.b; // 3.
```

This doesn't really feel very *object*-oriented. It's more like a strange
dynamically-typed variant of C where objects are loose struct-like bags of data.
Sort of a dynamic procedural language. But this is a big step in expressiveness.
Our Lox implementation now lets users freely aggregate data into bigger
collections. In the next chapter, we will breathe life into those objects.

### Challenges

1.  Trying to access a non-existent field on an object immediately aborts the
    entire VM. The user has no way to recover from this runtime error, nor is
    there any way to see if a field exists *before* trying to access it. It's up
    to the user to ensure on their own that only valid fields are read.

    How do other dynamically-typed languages handle missing fields? What do you
    think Lox should do? Implement your solution.

2.  Fields are accessed at runtime by their string name. But that name must
    always appear directly in the source code as an identifier token. A user
    program cannot imperatively build a string value and then use that as the
    name of a field. Do you think they should be able to? Come up with some
    language feature to add to Lox that enables that and implement it.

3.  Conversely, Lox offers no way to *remove* a field from an instance. You can
    set its value to `nil`, but the entry in the hash table is still there. How
    do other languages handle this? Choose and implement a strategy for Lox.

4.  Because fields are accessed by name at runtime, working with instance state
    is slow. It's technically a constant-time operation -- thanks hash tables --
    but the constant factors are relatively large. This is a major component of
    why dynamic languages are slower than statically-typed ones.

    How do sophisticated implementations of dynamically-typed languages cope
    with and optimize this?

