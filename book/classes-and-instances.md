^title Classes and Instances
^part A Bytecode Virtual Machine

- last major feature is oop
- two chapters in jlox, three here
- c more verbose
- little real radical new concepts
- after last chapter, earned reprieve
- rest of book smooth sailing

- can't do anything in oop without obj
- in class-based lang, can't have obs withotu classes
- start there
- this chapter add classes
- then allow instances of classes to be created
- support fields on instances

- covers stateful side of oop
- next two chapters behavioral

## class objects

- start with class
- kind of bottom up, implement obj rep and then front end and runtime support
- classes first-class, so new obj type in clox vm
- familiar with routine
- new struct

^code obj-class (1 before, 2 after)

- obj header
- for now, class just has name
- new obj type

^code obj-type-class (1 before, 1 after)

- macros for type testing

^code is-class (2 before, 1 after)

- and casting

^code as-class (2 before, 1 after)

- new function to create

^code new-class-h (2 before, 1 after)

- impl

^code new-class

- takes name, stores
- [use "klass" because clox code can also be compiled as c++ where "class" is
  reserved word. gives zany preschool "kidz korner" feel.]

- when done with class, free

^code free-class (1 before, 1 after)

- now that have mem mgr also handle that

^code blacken-class (1 before, 1 after)

- when gc traces class, mark string to keep class's name alive

- last op
- can print

^code print-class (1 before, 1 after)

- comes into play if user does

```lox
class Toast {}
print Toast; // "Toast".
```

- class just prints own name

## class declarations

- have rep so now hook up to land
- start in parser

^code match-class (1 before, 1 after)

- when hit class keyword, hand off to

^code class-declaration

- after class follows name
- parse that and then move name into chunk's const table as string literal
- need to put there so string is available at runtime
- classes first class and know their name, so unlike local vars, name is not
  purely compile-time entity

- class is also bound to its name as a variable
- otherwise would be hard to ref
- [could have class decl be expression and force user to store in variable
  themselves

  ```lox
  var Pie = class Pie {}
  ```

- like fun decl, reuse existing code for declaring var
- class name is just var
- then tell runtime to create class using new `OP_CLASS` inst
- takes index of class name in constant table as inst op
- after that, class obj exist on stack
- tell compiler to define var, which will emit code to store in global var if
  class declared at top level

- also tells compiler to make class name available for use
- note that do this before compiling class body
- this way class can refer to itself in own methods
- important for things like factory methods that construct instances of own
  class

- finally compile class body
- don't have methods yet so for now just require pair of braces

- that's it for front end for classes
- fields not declared up front, so class decl doesn't care about them
- over in runtime

- added new instr

^code class-op (1 before, 1 after)

- and dis

^code disassemble-class (2 before, 1 after)

- gets passed to runtime

^code interpret-class (3 before, 1 after)

- reads string from constant table
- pass to `newClass()` to create new class obj
- push onto stack

- have classes now
- like showed before can do

```lox
class Brioche {}
print Brioche;
```

- can't do anything else interesting with yet

## instances of classes

- classes have two main functions in oop

1. way to create instances of self
2. define behavior all instances share

- not doing behavior yet, so just former
- before add to language, add runtime rep for instance
- another obj struct

^code obj-instance (1 before, 2 after)

- kind of interesting
- has pointer to class
- instance knows class belong to
- not used now for much but important later for behavior
- also needs way to store fields

- [one big difference between dynamic and static langs.
  static lang know set of fields at compile time, so runtime rep usually flat
  contig array of fields accessed directly by byte offset at runtime. small and
  fast.
  dyn lang don't usually know which fields will be added to instance, so need
  flexible.]

- since can freely add fields to instance, need something flexible and growable
- could do array, but also want to access field by name as quick as possible
- what's growable, and fast to access by name? hash table
- fortunately already have that data struct
- just need to include module

^code object-include-table (1 before, 1 after)

- new struct, new obj type

^code obj-type-instance (1 before, 1 after)

- and new macros

^code is-instance (1 before, 1 after)

- and

^code as-instance (1 before, 1 after)

- to create instance, only need to know what class instance of

^code new-instance-h (1 before, 1 after)

- impl

^code new-instance

- store ref to class
- then initialize field table to empty table
- instance start off with no fields

- when vm does with instance

^code free-instance (4 before, 1 after)

- instance owns field table, so when instance freed, immediately tell table
  to free mem it owns too

- fields are most important source of indirect refs in gc
- most non-rooted live objects found by going through instance fields
- implement now

^code blacken-instance (4 before, 1 after)

- if instance is alive, then need to keep class alive too
- also need to keep every object instance's fields have reference to
- use existing `markTable()` fn

- and can print

^code print-instance (3 before, 1 after)

- just prints name of class followed by instance
- [in real lang would let user override behavior for custom `toString()`]

- now can hook up to runtime
- lox has no special `new` syntax, so no compiler support
- create instance by invoking class obj like fn
- so in existing runtime code that handles fn call, add new case

^code call-class (1 before, 1 after)

- if the object being called -- thing to left of arg list -- is a class object,
  then treat as constructor
- create new instance of called class and store on stack in slot where stack
  was
- [ignore arg list for now, will use in next chapter when add initlz]
- getter farther

```lox
class Cake {}
print Cake();
```

- prints "Cake instance"

## properties

- got classes instances, last is state
- fields
- lox uses classic dot syntax for access and set fields
- [also for methods]
- `.` works sort of like infix expr
- expression that evals to instance on left
- followed by dot then field name on right
- start by hooking into parse table

^code table-dot (1 before, 1 after)

- `.` very high precedence, same as fn call
- hands off to

^code compile-dot

- when get here, already parsed `.`
- expect name token immediately after
- parse that and store as string in constant table
- like class name, field name needs to be accessible at runtime since fields
  dynamically resolved

- two new expression forms, field access (getter) and mutate (setter)
- if see equals sign after field name, then must be setter
- but only check for that if in low-precedence context where allowed
- `canAssign` check ensures that in expr like:

```lox
a + b.c = 3
```

- need to report that as error, not incorrectly parse as if it was

```lox
a + (b.c = 3)
```

- assuming in low prec context where assignment allowed and did find `=`
- compile assigned expr
- then emit new instr for setting field
- inst takes one operand, index of field name in constant table

- otherwise, field access
- emit instr for that
- likewise op for field name
- two new insts

^code property-ops (1 before, 1 after)

- go ahead and dis

^code disassemble-property-ops (1 before, 1 after)

- slide over to runtime
- start with getter because little simpler

^code interpret-get-property (3 before, 2 after)

- when instr exec, already exec expr for lhs
- instance should be on top of stack
- read field name from const tabl
- look up field in instance's field hash table
- if found, pop instance and push field's value as result

- of course, might not be found
- lox, define that as runtime error

^code get-undefined (3 before, 3 after)

- if tableGet returns false, don't reach break so continue to here and exit
  with runtime error

- other failure mode to consider
- currently assume receiver is actually instance
- user could write

```lox
var obj = "not an instance";
print obj.field;
```

- code is wrong
- currently, vm would try to reinterpret ObjString as ObjInstance
- very bad
- need to check that object is instance first
- only instances have fields
- [could allow fields on other object types
  complicates implementation in ways that hurt perf
  also confusing: what happens if attach field to number 3? does every arith
  op that results in number three return object with that field? are there
  different threes? what does mean for equality?]

^code get-not-instance (1 before, 1 after)

- if the value on top of stack not instance, report runtime error and safely
  abort

- of course getters not very useful without actual fields
- need setters

^code interpret-set-property (3 before, 2 after)

- little more complex
- top of stack has instance and then value being stored above it
- again get instance from stack
- look up field name based on instr op
- then get assigned value from stack and store in instance's field table

- don't need to worry about field existing
- will create new field if needed or overwrite existing one
- do need to ensure that think setter is applied to is actually instance

^code set-not-instance (1 before, 1 after)

- stateful side of oop working now

```lox
class Box {}

var box = Box();
box.a = 1;
box.b = 2;
print box.a + box.b; // 3.
```

- doesn't really feel like oop
- more like weird dynamically typed c where objs just dumb bags of data
- next chapter make them come alive

### challenges

1.  accessing non-existent field immediately kills vm with no way to recover
    means no way to programmatically tell if field exists
    how do other dyn lang handle this?
    what would you do?
    impl soln.

2.  fields accessed by string name, but name must be encoded in source as
    identifier
    user program cannot dynamically create string and use as field name
    should they be?
    come up with mechanism to let user do that and implement

3.  no way to remove field from instance
    add mechanism

4.  field access requires hash table look up
    very slow
    how do advanced dyn lang impls optimize?
