^title Optimization
^part A Bytecode Virtual Machine

**todo: quote**

- bonus chapter

## measuring perf

- opt is empirical
- cannot just "make language faster"
- what programs?
- on what machines?
- how to tell if opt better at all?

### benchmarking

- benchmarks
- carefully crafted programs that stress some part of language
- then measure not what program does but how long takes to do it
- by comparing benchmark time before and after opt, can see how opt affects
  perf
- by tracking whole suite of benchmarks, tell which kinds of code get faster
  and which get slower
- benchmarks are test suite for performance
- just like tests control how impl behave, benchmarks constrain how perf
- choosing benchmarks is how select performance priorities for impl
- very important
- writing benchmark itself subtle art
- cpu throttling, caching, etc. all affect perf
- usually need to run multiple times, do stats
- very much like outdoor empirical science
- chapter not tutorial on benchmarking

### profiling

- have benchmark program
- want to make it go faster
- how?
- assume done all easy work, using right algorithms and data structures
- not using o(2^2) where o(n log n) exists
- hardware too complex to reason about from first principles
- must go out in the field
- profiler tool runs program and tracks where machine spends time
- many out there, learn a few
- [even if never do pl work, skill at profiler set you above many other
  progs]
- many times learned something with five minutes in profiler would have taken
  weeks otherwise

## faster hash table indexing

- first opt
- fundamentally tiny change
- though little tedious to fix everything around
- when first got bytecode vm working, wrote little sample script
- in oop lang, lot of code is field access and method calls, so wrote

```lox
class Zoo {
  init() {
    this.aarvark  = 1;
    this.baboon   = 1;
    this.cat      = 1;
    this.donkey   = 1;
    this.elephant = 1;
    this.fox      = 1;
  }
  ant()    { return this.aarvark; }
  banana() { return this.baboon; }
  tuna()   { return this.cat; }
  hay()    { return this.donkey; }
  grass()  { return this.elephant; }
  mouse()  { return this.fox; }
}

var zoo = Zoo();
var sum = 0;
var start = clock();
while (sum < 100000000) {
  sum = sum + zoo.ant()
            + zoo.banana()
            + zoo.tuna()
            + zoo.hay()
            + zoo.grass()
            + zoo.mouse();
}

print clock() - start;
print sum;
```

- benchmarks not useful or even sane way to write code
- [useful to be idiomatic to ensure reflect realworld use]
- just does bunch of method calls and field lookups
- [why print sum?
  many opt do dead code elim, good habit]

- run in profiler, let's see where time is going
- [again, on my machine, others may have different perf]
- before keep reading, take a guess
- where think clox spend most of its time?
- anything think particularly slow?

- when run, naturally spends almost 100% of time in `run()` which is main
  interp loop
- where time go in there?
- about 3.6% on `switch (instruction = READ_BYTE()) {`
- overhead of bytecode dispatch
- [lot techniques to reduce, threaded code, etc.]

- small pockets spread around various instrs: pop, return, add, etc.
- big hotspots are ~17% in OP_GET_GLOBAL
- 12% in OP_GET_PROPERTY
- OP_INVOKE whopping 42%
- three things to opt?
- no, all three spend almost all of that time in call to tableGet()
- in total, spend 72% in tableGet()
- in dynamic oop expect to be relatively large
- lot of runtime is dynamic dispatch -- looking up by name
- but still, wow
- dig into
- `tableGet()` thin wrapper around `findEntry()`
- almost all time spent in there
- recall

```c
static Entry* findEntry(Entry* entries, int capacity,
                        ObjString* key) {
  uint32_t index = key->hash % capacity;
  Entry* tombstone = NULL;

  for (;;) {
    Entry* entry = &entries[index];

    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // Empty entry.
        return tombstone != NULL ? tombstone : entry;
      } else {
        // We found a tombstone.
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      // We found the key.
      return entry;
    }

    index = (index + 1) % capacity;
  }
}
```

- spends 70% -- of total program exec on one line:

```c
  uint32_t index = key->hash % capacity;
```

- the pointer deref not problem
- turns out modulo really slow
- [modu-slow, amirite]
- entire vm spends 70% of time on that `%`
- can we improve?
- in general, hard to beat cpu's own impl of modulo operation
- if there was faster way to general impl in arith, chip would do that
- but, as always, optimizer's trick is that if we know we have constrained
  problem, can use those to advantage
- in our case, always take mod size of hash entry table
- remember how we size that?
- start at 8
- every time grows, double in size
- 8, 16, 32, 64
- divisor always power of two
- is there fast way to calculate remainder of number divided by power of two?
- yes, mask
- for ex, want to 

```
01000000  64
       %
11100101 229
       =
00100101  37
```

- looks like result keeps all 1 bits of original number to right of `1` in
 divisor
- if subtract 1 from divisor and use and to mask

```
00111111  63
       &
11100101 229
       =
00100101  37
```

- bit op very fast
- subtract pretty fast too
- so could change to:

```c
  uint32_t index = key->hash & (capacity - 1);
```

- but still very hot piece of code
- don't want to do sub every time
- only changes when capacity changes
- so can cache that result
- but go one farther
- if caching capacity - 1, no need to start capacity
- always cache + 1
- so instead of storing capacity as size, store directly as mask
- then to wrap hash to fit in entry array, just

^code initial-index (1 before, 1 after)

- one line change, big perf imp
- just have to go through and fix all other code used capacity
- kind of chore, but sometimes opt is grunt work

- first change decl in table

^code table-capacity-mask (1 before, 1 after)

- give new name since represents different value
- no longer size of array, but 1 minus
- which means when array is zero, size -1

^code init-capacity-mask (1 before, 1 after)

- would not work right if actually used as mask
- but can't use zero as divisor for % either
- when capacity is zero and table null, hash table doesn't check array at all

### find entry

- couple of other changes in find entry
- linear probing wraps around, so replace that mod too

^code next-index (4 before, 1 after)

- much rarer to hit this because probe sequences tend to be short, so less
  of a perf problem
- also declaration of function different

^code find-entry

- pass in mask not capacity now
- now need to fix everything calls findentry

### adjust capacity

- first adjust capacity

^code adjust-find-entry (2 before, 1 after)

- just switching name
- new capacity passed in, so fix signature too

^code adjust-capacity (1 after)

- adj uses both table's current and new capacity
- fix all
- when alloc new array, actually do need capacity, not mask

^code adjust-alloc (1 before, 1 after)

- add one
- code path rarer, so find doing the arith here
- init new array

^code adjust-init (1 before, 1 after)

- since capacity mask one less size, use `<=` to include full range indexes
- similar when walking old array

^code re-hash (1 before, 1 after)

- and when free old array

^code adjust-free (3 before, 1 after)

- and finally store mask instead of capacity

^code adjust-set-capacity (1 before, 1 after)

- just updating to new names
- [good names matter]

### table get set delete

- moving along, couple more fns that call find or adju
- when doing lookup, pass capacity mask

^code get-find-entry (2 before, 1 after)

- this is hot code path really care about
- setting important too

^code set-find-entry (3 before, 2 after)

- also need to handle growing array in tableset

^code table-set-grow (1 before, 1 after)

- lot of +1 and -1 here
- fertile soil for off-by-one bugs to crawl around in
- double check, but think got it right

- last place call finde is delete

^code delete-find-entry (1 before, 1 after)

- just fixing name again

### other changes

- that's find and adj
- handful other places use capacity
- when copying entries from one table to another

^code add-all-loop (1 before, 1 after)

- again just `<=` and var name change

- code for interning strings mirrors find entry
- own separate impl of hash table lookup
- need to switch to mask there too

^code find-string-index (4 before, 2 after)

- same as find entry
- also when linear probe wraps around

^code find-string-next (3 before, 1 after)

- couple bits over in mem mg
- fix for loop to mark all entries in table

^code mark-table (1 before, 1 after)

- and fix loop to remove all unmarked entries from string table

^code remove-white (1 before, 1 after)

- finally gc tracks size of table when free and needs correct count

^code free-table (1 before, 1 after)

- phew
- lot of plumbing to replace one `%` with a `&`
- see how we did
- on my machine 5.22452s before 2.60772s after
- almost exactly half time, twice as fast
- huge win
- because methods fields so prevalent, improve perf of almost all lox progras
  signif

- point not that modulo is evil or micro-optimizations super important
- very rare find such narrow hotspot in vm with such simple fix
- point is that didn't *know* mod was hotspot until profiler told us
- [granted, probably good to remember that mod is slow]

- run profiler again
- tableget still pretty hot
- as expected, big part of exec
- down to 35% total exec

## nan boxing

- very different kind of optimization
- above simply tiny change
- profiler basically told us what to do

- this much more subtle
- different flavor of opt
- invented by someone thinking deeply about lowest levels of machine arch
  and general perf goals
- [not sure who invented. used in vms for many years, lua jit jsc]
- called nan boxing or nan tagging

- on 64 bit machine, our value rep is 16 bytes

**todo: illustrate**

- value struct has two fields, tag and union for payload
- largest union cases are double and Obj* which both 8 bytes
- machine those aligned to 8 byte boundary too, so compiler pads tag to give
  8 bytes
- [if put tag after payload, would still do same because array of values needs
  to keep aligned. basically struct size always multiple of largest element.]

- if could cut that down then vm use less memory to store values
- machines have enough ram don't usually worry as much about optimizing for
  mem usage as for runtime speed
- but cpu caching means mem also have large impact on speed
- if make values smaller, fit more into cache line, fewer cache misses

- in dynamically typed lang, every val has to carry some runtime rep of its
  type
- in statically typed lang, if store 32-bit int in var, only need exactly 32
  bits for var
- in dyn, also need some kind of type tag or type rep
- one of big memory cost diff between dyn and static
- dyn vm hackers over years spent tons time invented clever ways to store type
  in as few bits as possible
- [pointer tag other one]

- optimization do here is example of that
- good fit for lang like js and lua where all numbers floating point
- lox too

### what is and is not a number?

- before get to opt, need to really understand how double rep in mem
- way almost all machines encode floating point numbers in binary defined in
  IEEE 754 [link]
- look like

**todo: illustrate**

- first bit is sign - whether num is positive or negative
- [since sigh bit always present, implies both negative and positive zero, and
  indeed has both]
- 11 bits for exponent
- 52 for fraction or "mantissa"
- not treatise on floating point, so won't go into too much detail about how
  exp and mantissa work
- basically mantissa is set of significant digits in number (in binary)
- exponent is how far to shift them away from decimal (well, binary) point

- ieee carves out special case exponent

**todo: illustrate**

- all ieee doubles that rep actual floating number have at least one zero in
  exponent bits
- if all exponent bits set, not just really large number
- instead, means value is "not a number" (nan)
- used to store special values like infinity or result of divide by zero
- for ex nan with all mantissa zero used for infinity
- [sign bit can be either so positive and negative infinity]

- remaining nan divided into two categories -- signalling and quiet
- determine by highest bit in mantissa
- if zero, signalling, otherwise quiet
- signalling nan for erroneous values that should halt computation
- don't want to use because some cpus may trap and abort
- [don't think any do, but good to be careful]
- quiet nans thinks like infinity where not numeric value, but otherwise
  "useful"

- what about other mantissa bits?
- couple combinations reserved by some cpus
- for x86 "QNan Floating-Point Indefinite"
- need to avoid
- all other mantissa bit combinations unused!
- so 64 bits can represent billions of floating point values
- but quiet nan only requires these bits set

**todo: illustrate**

- all unspecified bits can be anything, still considered qnan
- in other words, [todo how many] XXX different qnan values
- double basically has room to store extra 51 bit value in there
- could obvious set aside couple of bit patterns to rep nil, true, and false
- just leaves obj
- pointer is 64 bit, so seems doesn't fit
- but almost all 64 bit arch don't actually use full 64 bits for address
- [would allow you to address XXX bytes of ram, which no machine has]
- instead, only lowest 48 bits used
- can fit 48 bit pointer in there with 3 bits left
- enough bits to store tiny type tag to distinguish between nil, bool, or obj

- nan boxing
- using qnan bit combinations to store values inside ieee double
- cut lox value size in half
- even better, for numeric lox code, no conversion needed to turn raw double
  into lox value
- double has exact same rep
- [because dyn type, do still need to check type when performing op on double]

- for other types, turning raw value into lox value involve some bit magic
- fortunately, value rep already abstracted behind Value typedef and handful
  of macros
- change those basically rest of vm done
- hand wavey now, but get concrete as work through types

### conditional

- instead of changing existing code in place, add compile time switch

^code define-nan-boxing (2 before, 1 after)

- when enable use new nan box
- otherwise use old tagged union
- nan boxing requires 64 bit ieee 734 doubles
- most arch support, not all
- sad if couldn't use clox at all on those machines
- so support both

- declare new section in value header

^code nan-boxing (2 before, 1 after)

- new value rep is unsigned 64 bit int
- could use double
- would make macros for working with numbers easier
- but most other macros need to work on bits and u 64 easier for that
- treat as opaque type to most of vm
- so only macros care

- first end conditional section

^code end-if-nan-boxing (1 before, 2 after)

- now when NAN_BOXING defined you get new rep, otherwise old
- remaining work is creating new definitions of all existing macros to use
  when NAN_BOXING enabled
- work through one value type at a time

### numbers

**todo: remove comments**

- start with numbers since most straightforward

- to convert c double to lox value don't need to touch single bit
- 64 bit double is lox number value
- but do need to convince c compiler to interpret those unsigned int 64 bits
  as double
- classic way is union

^code double-union (1 before, 2 after)

- [part of reason make nan box optiona is because of this
  "type punning" through union been used for decades in c to access raw bit
  rep of value
  but never entirely clear whether officially valid code
  compilers generally do right thing
  not sure if guaranteed
  ]

- alas no way in vanilla c to create union and access field in single
  express, so call fn

^code number-val (1 before, 2 after)

- define like this

^code num-to-value (1 before, 2 after)

- create union, store double and then return uint64
- since union fields all overlap in memory, reinterprets same bits as different
  type
- looks slow -- fn call, create struct, assign
- compiler will eliminate all of it
- compile down to nothing

- going other way is similar

^code as-number (2 before, 2 after)

- macro calls out to fn

^code value-to-num (1 before, 2 after)

- fn works like other one but in reverse
- stores value into uint field
- returns double field

- lot of code to nothing
- type testing little more interesting
- every lox number is a double
- not all doubles rep lox numbers
- how to tell?

^code is-number (1 before, 2 after)

- if all of the quiet nan bits set then double is not representing number but
  some other type
- so mask everything out except quiet nan bits and then check to see if all set
- these are bits care about

^code qnan (1 before, 2 after)

**todo: illustrate**

- these are defined by ieee
- that's it for numbers

### singleton values nil true false

- next type is nil
- only one nil value, so only need single bit representation for it
- nil "singleton value"
- two others: true and false
- need three unique bit patterns
- two bits enough to distinguish four different val
- lowest bits of value as "tag" to define which singleton value

**todo: illustrate tag bit location**

^code tags (1 before, 2 after)

- nil then just bit representation of qnan with its tag set

**todo: illustrate**

^code nil-val (2 before, 1 after)

- to see if lox val is nil must exactly match bit pattern
- simple equality on two uint64 work

^code is-nil (2 before, 1 after)

- [would not work if type of Value was double
  ieee 754 defines equality for doubles and some doubles with different bit
  patterns defined to compare equal
  positive and negative zero (i.e. zero mantissa, zero exp but differen sign
  bit) compare equal
  weirder, two identical nan bit reps compare non equal
  uint64 avoids]

- bools similar

^code false-true-vals (2 before, 1 after)

- qnan bits with respective type tag bits

- to convert c bool to lox bool singleton, look at value and choose which
  one

^code bool-val (2 before, 1 after)

- go other direction pretty simple

^code as-bool (2 before, 1 after)

- macro should only be called after sure value is bool
- so must be either TRUE_VAL or FALSE_VAL
- can tell which one by comparing to either

- last type test macro more subtle

^code is-bool (2 before, 1 after)

- value could be any lox value including num
- want to tell if bool
- means must be either TRUE_VAL or FALSE_VAL
- could do `((v) == TRUE_VAL || (v) == FALSE_VAL)`
- bad -- eval macro arg twice
- [bad because arg can have side effect]
- would have to hoist to separate fn

- take advantage of tag rep
- both TRUE_VAL and FALSE_VAL have second tag bit set
- nil does not
- numbers don't have qnan bits set
- so if mask value with FALSE_VAL bits both TRUE_VAL and FALSE_VAL
  result is FALSE_VAL

**todo: illustrate**

- bit hackery!

### objects

- last type, hardest
- difficult because need to put entire pointer inside double bits
- not just couple of singletons
- so need "tag" to indicate obj type and also need set aside other bits for
  payload

- when qnan bits set, value not a double
- so sign bit unused
- use sign bit as tag to indivate obj versus not obj
- (not obj mean bool or nil)
- if sign bit and qnan bits all set, value must be obj*
- in that case, all remaining lower bits used to store address

**todo: illustrate**

- to convert raw obj* to value, just or all those together

^code obj-val (1 before, 2 after)

- take sign bit, qnan bits, and pointer and combine
- [pointer 64 bit on 64 bit arch, higher bits of that overlap qnan and sign
  bits
  don't mask out here because assume will always be zero and only lower 48 bits
  used
  on all comp and arch tested, is true
  working at level where c lang spec can't promise our code is correct, may
  only be valid on some comp/arch combinations
  not wrong to do so, just mean have to verify]
- lot of casting on obj* in there
- need to satisfy some picky compilers

1. (uintptr_t) Convert the pointer to a number of the right size.
2. (uint64_t)  Pad it up to 64 bits in 32-bit builds.
3. Or in the bits to make a tagged Nan.
4. Cast to a typedef'd value.

- lot of machinery, but mostly compile away
- actual work performed just some bitwise ops

- sign bit is highest bit:

^code sign-bit (1 before, 2 after)

- to get obj pointer back out, simply zero out sign and qnan bits

^code as-obj (1 before, 2 after)

**todo: illustrate**

- to clear bits, use `~` to create inverse mask of those bits then and value
  with result of that
- then cast to pointer

- last macro is type test

^code is-obj (1 before, 2 after)

- sign bit basically third type tag bit to distinguish obj from singleton types
- value is obj if all qnan bits are set (not number) and sign bit is too (not
  singleton)
- so mask down to just those bits and see if they are all set

### value functions

- macros all done
- rest of vm goes through those so almost everything working now
- two fns in value module work with value rep directly
- fix those

- first printing
- since nan box support conditional, don't replace old code
- instead add new code under ifdef

- don't have direct enum type can switch on
- instead use chain of type test macros to handle cases for each value type

^code print-value (1 before, 1 after)

- then end conditional section after old code

^code end-print-value (1 before, 1 after)

- other is equality

^code values-equal (1 before, 1 after)

- dead simple
- no type checks
- just compare raw bits
- singleton values nil and bools only equal to exactly themselves
- obj types like instances and classes only equal if pointing to same obj
- strings already interned, so also only equal if point to same obj
- numbers equal if have same bits

- oops! not true

^code nan-equality (1 before, 1 after)

- ieee 754 says two identical nan *not* equal to each other
- i know, crazy
- c compiler emits correct cpu instr to do that if types compared are double
- not for uint64
- need to handle
- annoying
- [could omit if willing to break weird corner]

- close conditional

^code end-values-equal (1 before, 1 after)

### measuring

- evaluating opt very different from earlier
- there clear hotspot in profiler
- fix and see go down
- macros have different perf impact
- get inlined whenever used so hard to see in profiler

- here much more diffuse
- made objs smaller
- reduce cache misses across all code
- depends heavily on size of lox program and amount of memory used
- tiny microbenchmark may not show much impact since everything so small in mem

- basically everything get a little faster
- opt like this unnerving
- can't rely on single surgical microbench
- instead want big suite of larger benchmarks roughly like real world code
- see how behave across all
- on my machine, try few different programs
- seem to roughly make things 10% faster
- not huge especially compared to prev opt
- probably not first opt would reach for
- other lower hanging fruit
- in sophisticated vm where those fruits all plucked, opt obj rep like this
  become more important
- main goal here to show you different flavor of opt and different kind of
  change can make

## where to next

- stop here with clox
- could tinker on it forever
- add new features, new optimizations
- but for this book, good stopping point

- what next in your pl journey?
- up to you

- most of you probably won't do significant interp or compiler work in career
- ok
- even so, book has equipped you with better understanding of how language you
  use work
- taught you insides of several important data structures
- given you a little practice with profiling and opt

- more important, give new way of looking at and solving problems
- even outside of lang, may be surprising to find problems that are "lang like"
  if look at right
- maybe that report you need to generate can model as little stack-based series
  of "instructions"
- user interface need to build and render is traversal over ast-like tree

- if do want to keep going further in langs, couple of directions to go next

- take "product" angle and learn more about how to not just build lang but get
  users to come to it
  - lang impl only one piece
  - becoming better writer for docs and marketing surprisingly important skill
  - ecosystem
  - need broad set of libraries and packages for lang to be useful
  - fun to build libs on core of stack
  - green field always enjoyable
  - write arg parser, file io, stuff like that
  - tools vital too
  - try building syntax highlighter for lox in your editor of choice
  - how would you add a debugger to clox?
  - very hard, some material out there on it, hugely powerful

- maybe want to go in more academic research angle
  - this book not very rigorous, but hope to give you solid intuitive founda
    to build on
  - lot of other compiler books out there
  - beyond that, papers
  - lot of cs lore, especially pl, never escape academic publishing
  - learning to find and read cs papers is own skill worth developing

- or just really like designing and hacking on lang
  - add more features
  - tweak syntax
  - write more benchmarks and more opt
  - if want to go faster with clox, will hit limitation of single-pass compiler
    soon
  - makes very hard to do many compile-time opt like constant folding or dead
    code elim
  - learn about ir and insert ast or other rep between parser and code gen

- or maybe this satisfied curiosity and content to stop here
- either way, hope you found it worth your time

## challenges

1. fire up profiler and look for other hotspot. see anything can optimize?

2. many strings in user prog very small. often single char. heap allocating tiny
   array and then having value to store pointer wasteful. classic obj rep opt
   is to have separate rep for small strings where chars stored inline.

   impl in clox building on earlier tagged union rep. profile and see if helps.

3. reflect back on experience with book. what worked well for you? what didn't?
   learning your personal learning style make you more effective at learning
   and growing.
