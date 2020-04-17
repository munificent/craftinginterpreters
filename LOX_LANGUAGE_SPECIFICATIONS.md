# The Lox Language

## Typing
Lox is dynamically typed. Variables can store values of any type, and a single variable can even store values of different types at different times. If you try to perform an operation on values of the wrong type - say, dividing a number by a string - then the error is detected and reported at runtime.


---
## The Standard Library

### print
You can use the `print` statement to output the value of an expression.
```
print "Hello, world!";
print 1 + 2;
```

### clock()
You can use the `clock()` function to retrieve the number of seconds since the application started.


---
## Comments
Comments can be introduced by two slashes. Those comments reach until the end of the line they are begun in.
```
// This is a comment
```


---
## Built-In Data Types
There are only a few built-in data types.

### Booleans
There are two Boolean values (obviously), and a literal for each one.
```
true;   // Not false.
false;  // Not *not* false.
```

### Numbers
Lox has only one kind of numbers: double-precision floating point. They are capable of holding decimal as well as integer values.
```
1234;   // An integer.
12.34;  // A decimal number
```

### Strings
String literals are enclosed in double quotes. 
```
"I am a string";
"";     // The empty string.
"123";  // This is a string, not a number.
```

### Nil
Used to represent "no value". 
```
nil;
```


---
## Expressions
The job of an expression is to produce a *value*. There are different types of expression that can be evaluated.


### Arithemtic
Lox features the basic arithmetic operators you know and love from C and
other languages.
```
add + me;
subtract + me;
multiply * me;
divide / me;
```
Furthermore, the minus operator ( - ) can also be used as a prefix operator to negate a number. 
All of these operators can only be used on numbers, with the exception of the plus operator ( + ) which can also be used to concatenate two strings. 

### Comparison and Equality
Numbers can be compared against each other using the four well known comparison operators.
```
less < then;
lessThan <= orEqual;
greater > than;
greaterThan >= orEqual;
```

Two values of any kind can be tested for equality or inequality.
```
1 == 2;         // false.
"cat" != "dog"; // true.
```

Different value types can be compared as well.
```
314 == "pi";    // false.
```

Values of different types are *never* equivalent:
```
123 == "123";   // false.
```

### Logical operators
The not operator, a prefix !, returns false if its operand is true, and vice versa.
```
!true;  // false.
!false; // true.
```

An `and` expression determines if two values are *both* true. It returns the left operand if it's false, or the right operand otherwise.
```
true and false; // false.
true and true;  // true.
```

An `or` expression determines if *either* of two values (or both) are true. It returns the left operand if it's true, or the right operand otherwise.
```
false or false; // false.
true or false;  // true.
```

The `and` as well as the `or` expression do **shortcircuit**. 
The `and` expression does not evaluate the right operator if the left one is false, because the result cannot become true anymore, not matter what the left operands value is. Conversely, the `or` expression does not evaluate the right operand if the left one is true, because the result cannot become false anymore. no matter what the left operands value is. 

### Operator precedence and grouping
The operands have the same precedence than they have in C. 
|Precedence|Operator|Description|
|---|---|---|
|1|+ -<br>!|Unary plus and minus<br>Logial NOT|
|2|* /|Multiplication and division|
|3|+ -|Addition and subtraction|
|4|< <= <br> > >=|Relational operators `less than` and `less or equal` <br> Relational operators `greater than` and `greater or equal`|
|5|== !=|Relational operators `equal` and `unequal`|
|6|and|Logial AND|
|7|or|Logial OR|
|8|=|Simple assignment

In cases where the precedence should be changed, `(` and `)` can be used to form an expressional group:
```
(min + max) / 2;
```


---
## Statements
The job of a statement is to produce an *effect*. Statements do not evaluate to a value.

### Blocks
A series of statments can be wrapped into a block. Those blocks also affect scopes.
```
{
    print "One statement.";
    print "Two statements.";
}
```


---
## Variables
Variables are declared using `var` statements. If no initialization value is provided, the variable's value defaults to `nil`.
```
var iAmVariable = "here is my value";
var iAmNil;
```

Once declared, a variable can be accessed and assigned using its name.
```
var breakfast = "bagels";
print breakfast;    // "bagels".
breakfast = "beignets";
print breakfast;    // "beignets".
```


---
## Control Flow

### Conditions
An `if()` statement executes another statements based on some condition.
```
if (condition) {
    print "yes";
}
```
This can be expanded by an `else` statement which is being executed when the specified condition is *not* fulfilled.
```
if (condition) {
    print "yes";
} else {
    print "no";
}
```

### Looping

A `while` loop executes the body repeatedly as long as the condition expression evaluates to true. As of now, there are no `do-while` loops. 
```
var a = 1;
while (a < 10) {
    print a;
    a = a + 1;
}
```

A `for` loop executes the body repeatedly as long as the condition expression evaluates to true. Other than `while` loops, you can specify and evaluate a variable that is part of this condition expression. 
As of now, there are no `foreach` loops. 
```
for (var a = 1; a < 10; a = a + 1) {
    print a;
}
```


---
## Functions
You can define your own functions using the `fun` keyword, followed by the name of your function, a set of parenthesis and a function body.
```
fun printHello() {
    print "Hello, world!";
}
```

You are free to specify a set of paramenters within the parentesis to hand in values to your function.
```
fun printSum(a, b) {
    print a + b;
}
```

Your function can be executed by specifying its name and - if required - its arguments.
```
printHello();   // "Hello, world!"
printSum(1, 4); // "5".
```

You can furthermore let your function return a result using the `return` statement.
```
fun returnSum(a, b) {
    return a + b;
}
```

If the execution reaches the end of a block without finding a `return` statement, it implicitly returns `nil`.

### Closures
Functions are first class citizens. This means that they can be referenced, stored in a variable and passed around. 
```
fun add(a, b) {
    return a + b;
}

fun identity(a) {
    return a;
}

print identity(addPair)(1, 2);  // Prints "3".
```

You can also declare local functions inside another function.
```
fun outerFunction() {
    fun localFunction() {
        print "I am local!";
    }

    localFunction();
}
```

If you combine local functions, first class functions and block scope you can achieve something like the following:
```
fun returnFunction() {
    var outside = "outside";

    fun inner() {
        print outside;
    } 

    return inner;
} 

var fn = returnFunction();
fn();   // Prints "outside".
```

Local functions "hold on" to references to any surrounding variables that they use so that they stay around even after the outer function has returned.


---
## Classes

### Definition
You can define custom datatypes by the use of classes. A class is meant to hold the functionality and the state of a functional unit of the program. 
```
class Breakfast {
    cook() {
        print "Eggs a-fryin'!";
    }

    serve(toWhom) {
        print "Enjoy your breakfast, " + toWhom + ".";
    }
}
```

The body of a class contains its methods. They look like function, only without the `fun` keyword. 
When the class declaration is executed, it creates a class object and stores it in a variable named after the class. Like functions, classes are first class citizens. 
```
// Store it in a variable.
var someVariable = Breakfast;

// Pass it to a function.
someFunction(Breakfast);
```

### Instantiation
A class acts like a factory function for instances of itself. You can create an instance of a class by calling it like a function. 
```
var breakfast = Breakfast();
breakfast.cook();           // "Eggs a-fryin'!"
breakfast.serve("Robert");  // "Enjoy your breakfast, Robert."
```

### Initialization
Besides methods, a class can also hold fields. Those properties can be freely added onto objects.
```
var breakfast = Breakfast();
breakfast.meat = "sausage";
breakfast.bread = "sourdough";
```
Assigning to a field creates it if it doesn't already exist. 
Accessing a field or method on the current object from within a method required the `this` keyword.
```
class Breakfast {
    serve(toWhom) {
        print "Enjoy your " + this.meat + " and " + 
        this.bread + ", " + toWhom + ".";
    } 
    
    // ...
}
```

To ensure an object is in a valid state when it's created, yoi can define an initializer. If your class has a method named `init()`, it is called automatically when the object is constructed. Any parameters passed to the class are forwarded to its initializer.
```
class Breakfast {
    init(meat, bread) {
        this.meat = meat;
        this.bread = bread;
    }

    // ...
}

var baconAndToast = Breakfast("bacon", "toast");
baconAndToast.serve("dear reader");
// "Enjoy your bacon and toast, dear reader."
```

### Inheritance
Lox supports single inheritance. When you declare a class, you can specify a class that it inherits from using the `<` operator.
```
class Brunch < Breakfast {
    drink() {
        print "How about a Bloody Mary?";
    }
}
```

Here, Brunch is the **derived class** or **subclass**, and Breakfast is the **base class** or **superclass**. Every method defined in the superclass is also available in its subclasses.
```
var benedict = Brunch("ham", "english muffin");
benedict.serve("noble reader");
```

Even the `init()` method gets inherited. Since the subclass wants to define its own `init()` method, but the original one also needs to be called so that the superclass can maintain its state, `super` can be used as a reference to the superclass.
```
class Brunch < Breakfast {
    init(meat, bread, drink) {
        super.init(meat, bread);
        this.drink = drink;
    }
}
```
---
