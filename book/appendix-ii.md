For your edification, here is the code produced by [the little script
we built][generator] to automate generating the syntax tree classes for jlox.

[generator]: representing-code.html#metaprogramming-the-trees

## Expressions

Expressions are the first syntax tree nodes we see, introduced in "[Representing
Code](representing-code.html)". The main Expr class defines the visitor
interface used to dispatch against the specific expression types, and contains
the other expression subclasses as nested classes.

^code expr

### Assign expression

Variable assignment is introduced in "[Statements and
State](statements-and-state.html#assignment)".

^code expr-assign

### Binary expression

Binary operators are introduced in "[Representing
Code](representing-code.html)".

^code expr-binary

### Call expression

Function call expressions are introduced in
"[Functions](functions.html#function-calls)".

^code expr-call

### Get expression

Property access, or "get" expressions are introduced in
"[Classes](classes.html#properties-on-instances)".

^code expr-get

### Grouping expression

Using parentheses to group expressions is introduced in "[Representing
Code](representing-code.html)".

^code expr-grouping

### Literal expression

Literal value expressions are introduced in "[Representing
Code](representing-code.html)".

^code expr-literal

### Logical expression

The logical `and` and `or` operators are introduced in "[Control
Flow](control-flow.html#logical-operators)".

^code expr-logical

### Set expression

Property assignment, or "set" expressions are introduced in
"[Classes](classes.html#properties-on-instances)".

^code expr-set

### Super expression

The `super` expression is introduced in
"[Inheritance](inheritance.html#calling-superclass-methods)".

^code expr-super

### This expression

The `this` expression is introduced in "[Classes](classes.html#this)".

^code expr-this

### Unary expression

Unary operators are introduced in "[Representing Code](representing-code.html)".

^code expr-unary

### Variable expression

Variable access expressions are introduced in "[Statements and
State](statements-and-state.html#variable-syntax)".

^code expr-variable

## Statements

Statements form a second hierarchy of syntax tree nodes independent of
expressions. We add the first couple of them in "[Statements and
State](statements-and-state.html)".

^code stmt

### Block statement

The curly-braced block statement that defines a local scope is introduced in
"[Statements and State](statements-and-state.html#block-syntax-and-semantics)".

^code stmt-block

### Class statement

Class declarations are introduced in, unsurprisingly,
"[Classes](classes.html#class-declarations)".

^code stmt-class

### Expression statement

The expression statement is introduced in "[Statements and
State](statements-and-state.html#statements)".

^code stmt-expression

### Function statement

Function declarations are introduced in, you guessed it,
"[Functions](functions.html#function-declarations)".

^code stmt-function

### If statement

The `if` statement is introduced in "[Control
Flow](control-flow.html#conditional-execution)".

^code stmt-if

### Print statement

The `print` statement is introduced in "[Statements and
State](statements-and-state.html#statements)".

^code stmt-print

### Return statement

You need a function to return from, so `return` statements are introduced in
"[Functions](functions.html#return-statements)".

^code stmt-return

### Variable statement

Variable declarations are introduced in "[Statements and
State](statements-and-state.html#variable-syntax)".

^code stmt-var

### While statement

The `while` statement is introduced in "[Control
Flow](control-flow.html#while-loops)".

^code stmt-while
