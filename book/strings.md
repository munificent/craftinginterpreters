^title Strings
^part A Bytecode Virtual Machine

- second chapter on value rep
- so far have three: num, bool nil
- have in common: small and immutable
- worst case, num is two words
- small enough can pack everything in union and pay worst case cost every val

- want to add strings
- no max length
- even if clamped to, say, 256 chars, can't burn that on each value
- need to support objects of different sizes

- exactly what heap and dynamic alloc designed for
- can alloc as many bytes as needed, on demand
- get back pointer

...

## values on the heap

- have two-level represent for lox values
- every value is "Value"
- for small fixed-size, has type tag and stores payload inside value itself
- if value larger, needs heap alloc
- will eventually have multiple heap alloc val types: string, class, instance,
  fn
- lot in common around mem mgmt, so treat uniformly
- call "obj"
- one new val type for all heap alloc types:

^code val-obj (1 before, 1 after)

- stores pointer to Obj
- Obj itself on heap

^code union-object (1 before, 1 after)

- also have same type macros had for other val types
- tell if val is obj

^code is-obj (1 before, 2 after)

- extract obj from value

^code as-obj (2 before, 1 after)

- promote raw obj to val

^code obj-val (1 before, 2 after)

### polymorphic struct

- every heap alloc type is Obj, but not all same
- string needs char array
- fn will need chunk for body
- instance will need array of fields
- how handle different payloads and sizes?
- can't use union like did for value since sizes wildly divergent

- use related technique
- [no canonical name, "type punning"
  https://en.wikipedia.org/wiki/Type_punning]
- "polymorphic" struct

- have type tag to tell diff obj apart
- then payload fields
- instead of union, separate structs for each type
- tricky part is how to treat different structs uniformly
- no inheritance in c

- explain soon, first get prelim

- Obj itself contains data used by all heap-alloc types
- sort of like "base class"

- because some tricky cycles, need to fwd declare in value

^code forward-declare-obj (2 before, 1 after)

- actual decl in new module

^code object-h

- right now, only contains type tag

^code obj-type (1 before, 2 after)

- add helper macro to extract type given value
- extracts obj then type

^code obj-type-macro (1 before, 2 after)

- only one type for now, strings
- string payload is in separate struct
- again need, fwd decl
- [annoying part of c, won't do often but need for strings because strings very
  core to system because of hash tables and string interning]

^code forward-declare-obj-string (1 before, 2 after)

- defn

^code obj-string (1 before, 2 after)

- string stores array of chars in string
- also number of bytes
- (that way don't have to walk chars looking for \0 to get length)

- because ObjString is Obj, also needs shared state
- get by having first field be Obj
- thus ObjString contains all fields of Obj
- C says struct lays out first in order of decl
- for nested structs, fields stored right inline
- so memory for Obj and ObjString look like

**todo: illustrate**

- note that first bytes of ObjString exactly line up with Obj
- not coincidence
- c spec requires

- [spec:]

    ```
    6.7.2.1 13

    Within a structure object, the non-bit-field members and the units in which
    bit-fields reside have addresses that increase in the order in which they
    are declared. A pointer to a structure object, suitably converted, points to
    its initial member (or if that member is a bit-field, then to the unit in
    which it resides), and vice versa. There may be unnamed padding within a
    structure object, but not at its beginning.
    ```

- designed to enable clever pattern

- can convert pointer to a struct to pointer to its first field and back
- given an `ObjString*`, can safely cast to `Obj*`
- every ObjString "is" Obj in oop sense

- when later add other heap obj types, each struct have Obj as first field

- can cast other direction, too
- need to ensure `Obj*` is actually part of ObjString and not some other obj

- like for val union, macro to test

^code is-string (1 before, 2 after)

- takes value, not raw Obj
- most code works with values, even when using heap-alloc ones

- uses inline fn

^code is-obj-type (1 before, 2 after)

- why not just use macro?
- note references arg twice
- if put whole thing in macro would eval arg expr twice
- bad if side effect

```c
IS_STRING(POP())
```

- would pop two elements!
- fn fixes

- once tested value, can unwrap

^code as-string (1 before, 2 after)

- two macros
- one to treat as clox ObjString
- useful for getting length
- one to go all the way to raw C char array

## strings

- vm can rep strings, now add support to lang
- begin in parser, string literals
- already lex, so handle string token

^code table-string (1 before, 1 after)

- calls

^code parse-string

- gets raw chars from lexeme
- +1 and -2 are to trim quote chars
- call fn to alloc new ObjString on heap with chars
- wrap in Value

- compiler needs access module

^code compiler-include-object (2 before, 1 after)

- declared

^code copy-string-h (2 before, 2 after)

- module now gets impl file

^code object-c

- first thing, allocate new char array on heap
- copies chars from lexeme to that
- [don't forget to terminate string!]
- ensures string owns all of its own memory
- when later free string obj, frees char array
- would be bad if tried to free chunk of memory in middle of orig source string

- uses new macro

^code allocate (2 before, 1 after)

- simple wrapper to avoid redundancy in c alloc idiom

- real work in

^code allocate-string

- create new ObjString on heap, then initialize char pointer and len
- uses

^code allocate-obj (1 before, 2 after)

- lot of little helpers to go through
- [nicely factored code not always super friendly to book format]

^code allocate-object (2 before, 2 after)

- allocates heap mem of right size for full obj
- casts to Obj*
- initializes type tag
- since ObjString starts with Obj, correctly initializes type tag for string

- now have new valid string obj living on heap
- can compile and execute string literals

## operations on strings

- have string literals, but can't do anything else with strings
- first is making existing print handle new value type

^code call-print-object (1 before, 1 after)

- if val is heap obj, call out to helper
- declared over in obj

^code print-object-h

- impl

^code print-object (1 before, 2 after)

- only have one type for now, so simple
- just print string as-is

- [does not re-escape or quote string. prints string itself, not lox source
  rep of string as literal]

- equality operator also needs to handle strings
- consider

```lox
"string" == "string"
```

- two separate literals, so two calls to copyString and two
- separate strings at runtime
- want strings to have value equality
- want print true
- require special support

^code strings-equal (1 before, 1 after)

- equal if same chars, even if not same obj in memory
- will revise how do this later, but ok for now
- to use memcmp, need

^code value-include-string (1 before, 2 after)

- as snippet hints, last op is `+`
- want to concatenate strings
- means `OP_ADD` more complex

^code add-strings (1 before, 1 after)

- if operands both strings, concate
- if both numbers, add
- else error
- concat is in fn

^code concatenate

- verbose as c is
- calc length of result
- alloc char array
- copy two parts
- terminate
- [don't use strcpy() and strcat() since already know length]
- instead of copyString, use new takeString

^code take-string-h (1 before, 2 after)

- need to include

^code vm-include-string (1 before, 2 after)

- looks like

^code take-string

- where copyString assumes can't own given string, take string is when you
  can transfer ownership
- since concat() uses ALLOCATE() to create char array, ok for ObjString to take
  it
- avoids redundant copy

## freeing objects

- consider

```lox
"st" + "ri" + "ng"
```

- alloc ObjString for each literal and store in chunk const table
- runs, pushes "st" and "ri" on stack
- pops and concats
- allocates new string for "stri" and pushes
- pushes "ng"
- pops both, concats, pushes "string"

- vm no longer has any ref to "stri" string
- memory still allocated on heap
- mem leak
- bad people

- begin period "living in sin"
- vm dynamically allocates memory
- does not free it
- lox language frees user from worrying about managing memory
- doesn't mean we don't have to
- user doesn't have to because we do it for them
- our job to ensure unused memory is reclaimed

- full solution is to impl gc so can reclaim while prog running
- alas, need to get quite a few pieces of vm in place before possible to
  implement gc
- so on borrowed time for a while

- [many hobby langs allocate and don't free and intend to get to gc later
- can sink project
- really hard to add later
- lots of places in code where pointer can get squirreled away where gc can't
  find and end up freeing mem still in use]

- first, need to at least be able to *find* alloc mem so possible to reclaim
  mem
- totally fine for user program to no longer ref some obj
- but vm internally still needs to be able to find
- many sophisticated techniques for this
- do simplest but still ok for clox
- all heap obj stored in linked list
- can traverse list to find every single alloc obj, whether or not program
  or stack still has ref to it

**illustrate**

- instead of creating sep linked list node, use Obj itself as node
- each obj has pointer to next obj in list

^code next-field (1 before, 1 after)

- vm stores pointer to head, first node in list

^code objects-root (1 before, 1 after)

- when init vm, no obj yet, so head null

^code init-objects-root (1 before, 1 after)

- whenever alloc obj, insert in list
- ensures can find it even if nothing else in vm refs

^code add-to-list (1 before, 1 after)

- insert at head of list so don't have to track tail
- obj module referencing global vm var
- need to expose

^code extern-vm (2 before, 1 after)

- lets obj see vm

- eventually, gc can reclaim while vm running
- but when vm done need to free everything still alloc

^code call-free-objects (1 before, 1 after)

- empty fn finally does something
- calls

^code free-objects-h (1 before, 2 after)

- need some includes

^code vm-include-object-memory (1 before, 1 after)

- is

^code free-objects

- cs 101 code to walk linked list
- on each obj, calls

^code free-object

- string obj actually two allocs
- objstring itself and separate char array
- free both
- don't need to null pointers or anything
- uses last helper macro

^code free (1 before, 2 after)

- resizes mem down to zero
- goes through single reallocate() fn
- will be useful later so can keep track of how much mem outstanding
- gc use that to know when to collect

- since mem module using obj stuff need to include
^code memory-include-object (2 before, 2 after)

- put include in header because later ref some obj stuff in memory.h
- also needs access to vm to get head of list

^code memory-include-vm (1 before, 2 after)

- vm no longer leaking mem
- frees every alloc by when done executing
- but doesn't free while exec
- keeps alloc as long as running, building debt
- doesn't pay off until done
- when can write longer progs with loops, possible to consume lot of memory
- won't address until add gc
- but big step

- strings important fundamental type, but also quite complex
- [reason c support for strings so bad]
- have those now
- also have infrastructure to handle other dynamically-sized values
- now have strings, can build data structure fundamental to most dynamic lang
  imples, hash table
- for next chapter

## design note: encoding

- don't shy away form most hard problems in clox, but avoiding one
- string encoding
- two core parts
  - what is a single "character" in a string? how many different chars allowed
    and which values?
    - early on, no standard at all
    - first big one was ascii
      - gave you 128 different char values (only 7 bits)
      - great... if you only care about english
    - next came unicode
    - first 16387 chars ("code points")
    - could fit in two bytes
    - now many more than that
  - how are characters represented in bits and bytes?
    - ascii used byte per char
      - extra values given different incompatible meaning
    - utf-16 uses two bytes for each unicode code point
      - worked great when less than 16387 code points
      - now not enough, need "surrogate pairs"
    - utf-32 next evolution of utf-16 - 4 bytes per code point
    - utf-8 is variable-length encoding
      - low code point values fit in fewer bytes
      - can't directly index into byte array to find char
        - have to walk string

- trade-offs fundamental
- ascii is compact and fast but doesn't support other languages
- utf-32 wastes lots of memory when majority of strings contain english
  letters and digits
  - [even in apps for non-english users, many strings contain ids, map keys,
    sql snippets, etc. that use mostly latin alphabet]
- utf-8 is well optimized for mem usage for common strings
  - variable-length encoding makes it slow to access nth char in string
  - basically simple form of compression and have to "decompress" by walking
    chars to find things
- utf-16 historical mistake
  - no longer fixed-size because of surrogate pairs, so poor perf of utf-8
  - uses more memory

- wait, what is char?
- unicode has "combining chars", code points that modify preceding char
- "..." is to reader single "char" but actually series of unicode code points
- even in utf-32, encoding sort of variable length if want to access nth visible thing ("grapheme cluster") in string

- no perfect solution
- have to choose early in life of lang
- choice tends to be hard to change
- implicitly shows up in apis
- [most of python 3 pain around changing string encoding]

- one option is maximal: do rightest thing
- support all unicode code points
- internally use multiple different encodings based on contents of string:
  ascii if all chars fit in range, utf-16 if no surrogate pairs, etc.
- provide api to lang users to walk code points, extended grapheme clusters,
  etc.
- but now very complex system
  - lot to implement, debug, optimize
  - things like serializing strings to disc, interop with other systems harder
  - api more complex, error-prone, hard to learn for users
- swift, perl 6

- simpler compromise is use utf-8 and only expose code point api
- leave grapheme clusters to external library
- more expressive than ascii but not much more complex
- lose fast direct indexing by code point, but not that common in practice
- memory efficient

- don't do utf-16 unless need to run on platform like browser or jvm where that's native type

- if designing big workhorse lang, would probably do maximal
- for smaller scripting or embedded language, utf-8

## challenges

- challenge to use trailing struct array and store chars inline
  https://en.wikipedia.org/wiki/Flexible_array_member

- challenge to support "const" strings that point to source string and don't
  free

- challenge, what should `+` do when one op is string and other isn't? what
  do other langs do?
