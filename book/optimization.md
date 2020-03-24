^title Optimization
^part A Bytecode Virtual Machine

> The evening's the best part of the day. You've done your day's work. Now you
> can put your feet up and enjoy it.
>
> <cite>Kazuo Ishiguro, <em>The Remains of the Day</em></cite>

Think of this chapter as a bonus. If I still lived in New Orleans, I'd call it
*lagniappe* -- a little something extra given for free to a customer. You've got
a whole book and a complete already, but I wanted you to have some fun tweaking
clox to squeeze some more performance out it.

In this chapter, we'll apply two very different optimizations to the our virtual
machine. In the process, you'll get a feel for measuring and improving the
performance of a language implementation, or any program, really.

## Measuring Performance

*Optimization* means taking a working program and improving its performance
along any number of axes. In contrast with other engineering, it doesn't aim to
change *what* the program does, instead, it reduces the number of resources it
takes to do so. We generally think of optimization as meaning runtime speed, and
that tends to be the main axis we optimize on. But it's often important to
reduce memory usage, startup time, persistant storage utilization, or network
bandwidth. All physical resources have some cost -- even if the cost is mostly
in wasted human time -- so optimization often pays its way.

There was a time in the early days of computer technology that a skilled
programmer could hold the entire hardware architecture and compiler pipeline in
their head and reason about the performance of a program just by thinking about
it. Those days are long long gone, separated from us in the present by
microcode, cache lines, branch prediction, deep compiler pipelines, and
sprawling instructions. We like to pretend C is a "low level" language, but the
stack of human engineering between `printf("Hello, world!");` and a greeting
appearing on screen is now centuries tall.

Optimization is now firmly in the realm of empirical science. Our program is a
border collie sprinting through the obstactle course of the hardware. If we want
to make her reach the end faster, we can't just sit and ruminate on canine
physiology until enlightnment strikes. Instead, we need to *observe* her
performance, see where she stumbles, and *then* find faster paths for her to
take.

Note how much of that is particular to one dog on one obstactle course. When we
write optimizations for our virtual machine, we also can't assume that we will
make *all* Lox programs run faster on *all* hardware. Different Log programs
stress different areas of the VM, and different architectures have their own
strengths and weaknesses.

### Benchmarks

When we add new functionality, we validate correctness by writing tests -- Lox
programs that use a feature and pass if the VM behaves property. This lets us
pin down behavior and ensure we don't break things as we make other changes.

We have the same needs when it comes to performance:

1.  How do we validate that an optimization *does* improve performance and by
    how much?

2.  How do ensure that other unrelated changes don't *regress* performance?

The Lox programs we write for that are *benchmarks*. These are carefully crafted
programs that stress some part of the language implementation. They measure not
*what* the program does, but how <span name="much">*long*</span> it takes to do
it.

<aside name="much">

Most benchmarks measure running time. But, of course, you'll also often end up
writing benchmarks that measure use of other resources too -- memory allocation,
how much time is spent in the garbage collector, start up time, etc.

</aside>

By measuring the performance of a benchmark before and after a change, you can
see what your change does. When you land an optimization, all of the tests
should behave the exact same as they did before, but hopefully the benchmarks
run faster.

Once you have an entire *suite* of benchmarks, you can measure not just *that*
an optimization changes performance, but on which *kinds* of code. Sometimes,
you'll find that some benchmarks get faster while others get slower. Then you
have to make hard decisions about what kinds of code your language
implementation optimizes for.

The <span name="js">suite</span> of benchmarks you choose to write is a key part
of that decision. In the same way that your tests encode your choices around
what correct behavior looks like, your benchmarks are the embodiment of your
priorities when it comes to performance. They will guide which optimizations you
implement, so choose your benchmarks carefully and don't forget to periodically
reflect on whether they are helping you reach your larger goals.

<aside name="js">

In the early proliferation of JavaScript VMs, the first widely-used benchmark
suite was Mozilla's SunSpider programs. Browser competition was fierce, and
SunSpider results were used as marketing fodder by each browser to claim theirs
was fastest. That highly incentivized VM hackers to optimize to those benchmarks.

But, unfortunately those benchmarks often didn't reflect real-world performance.
They were mostly microbenchmarks -- tiny toy programs that completed quickly.
Those benchmarks penalize complex just-in-time compilers that start off slower
but get much *much* faster once a program has "warmed up" and had enough time
for the compiler to optimize and re-compile hot code paths. This put VM hackers
in the unfortunate position of having to choose between making the SunSpider
numbers get better, or actually optimizing the kinds of programs real users
were running.

Google's V8 team responded by sharing their Octane benchmark suite, which were
closer to real-world code at the time. Years later, as JavaScript use patterns
continued to evolve, even that eventually outlived its usefulness. Expect that
your benchmarks will need to evolve as your language's ecosystem does.

</aside>

Writing good benchmarks is a subtle art. Like tests, you need to balance not
overfitting to your implementation while ensuring that the benchmark does
actually tickle the code paths that you care about. When you measure
performance, you need to compensate from variance caused by CPU throttling,
caching, and other weird hardware and operating system quirks. I won't give you
a whole treatise here, but treat benchmarking as its own skill that you will get
better as you practice.

### Profiling

OK, so you've got a few benchmarks now. You want to make them go faster. Now
what? First of all, let's assume you've done all the obvious easy work. You are
using the right algorithms and data structures -- or, at least, you aren't using
ones that are aggressively wrong. I don't consider using a hash table instead of
a linear search through a huge unsorted array "optimization" so much as "good
software engineering".

Since the hardware is too complex to reason about our program's performance from
first principles, we have to go out into the field. That means *profiling*. A
profiler, if you've never used one, is a tool that runs your <span
name="program">program</span> and tracks hardware resource use as it executes.
Simple ones show you how much time was spent in each function in your program.
Sophisticated ones track each instruction and data cache miss, memory
allocations, and all sorts of other metrics.

<aside name="program">

"Your program" here means the Lox VM itself running some *other* Lox program. We
are trying to optimize clox, not the user's Lox script. Of course, the choice of
which Lox program to load into our VM will highly affect which parts of clox get
stressed the most, which is why benchmarks are so important.

A profiler *won't* show us how much time is spent in each *Lox* function in the
script being run. We'd have to write our own "Lox profiler" to do that, which is
slightly out of scope for this book.

</aside>

There are many profilers out there for various operating systems and languages.
For whatever platform you program in, it is worth getting familiar with a decent
profiler on it. You don't need to be a master. I have learned things within
minutes of throwing a program at a profiler that would have taken me *days* to
discover on my own through trial and error. Seriously -- profilers wonderful,
magical tools.

## Faster Hash Table Probing

Enough pontificating, let's make some performance charts go up and to the right.
The first optimization we'll do, it turns out, is about the *tiniest* possible
change we could make to our VM. There's a lot of scaffolding *around* the change
we'll have to deal with, but the main optimization itself is pint-sized.

When I first got the bytecode virtual machine that clox is descended from
working, I did what any self-respecting VM hacker would do. I cobbled together a
couple of benchmarks, fired up a profiler, and ran those scripts through my
interpreter. In a dynamically-typed language like Lox, a large fraction of user
code is field accesses and method calls, so one of my benchmarks looked
something like this:

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
  sum = sum + zoo.ant() // [sum]
            + zoo.banana()
            + zoo.tuna()
            + zoo.hay()
            + zoo.grass()
            + zoo.mouse();
}

print clock() - start;
print sum;
```

<aside name="sum">

Another thing this benchmark is careful to do is *use* the result of the code
it executes. By calculating a rolling sum and printing the result, we ensure
the VM *must* execute all that code. This is an important habit to be in. Our
Lox VM is too simple, but many compilers do aggressive dead code elimination
and are smart enough to discard computations whose result is never used.

Many a programming language hacker has been impressed by the blazing performance
of a VM on some benchmark, only to realize that it's because the compiler
optimized the entire benchmark program away to nothing.

</aside>

If you've never seen a benchmark before, this might seem ludicrous. *What* is
going on here? The program itself isn't designed to do anything useful. What it
does do is call a bunch of methods and access a bunch of fields. It does that
and basically nothing else to ensure that our profiling results focus on the
parts of the language we're interested in. Since fields and methods live in hash
tables, it takes care to populate at least a <span name="more">*few*</span>
interesting strings in the instance and method tables. Finally, it does that in
a big loop to ensure we give our profiler enough execution time to really dig
in and see where the cycles are going.

<aside name="more">

If you really want to benchmark hash table performance, you'll want to try
tables of lots of different sizes. The six keys we add to each table here aren't
even enough to get over our hash table's eight-element minimum threshold. But I
didn't want to throw an enormous benchmark script at you. Feel free to add more
animals and treats if you like.

</aside>

Before I tell you what my profiler showed me, spend a minute taking a few
guesses. Where in clox's codebase do you think the VM spent most of its time? Is
there any code we've written in previous chapters that you suspect is
particularly slow?

Here's what I found: Naturally, the VM spends almost all of its time in `run()`
or in functions called by it. We'll use "inclusive time" to mean the time
spent in some function or other functions it calls. The `run()` function is the
main bytecode execution loop. It drives *everything*. Inside there, we spent
about 3.6% of the runtime on this line:

```c
  switch (instruction = READ_BYTE()) {
```

That's the main bytecode dispatch loop, so that fraction represents some of the
overhead that a bytecode interpreter's abstraction layer adds over native code.
There are other small pockets of time sprinkled around inside various cases in
the bytecode switch for common instructions like `OP_POP`, `OP_RETURN`, and
`OP_ADD`. The big heavy instructions are `OP_GET_GLOBAL` with 17% of the
execution time, `OP_GET_PROPERTY` at 12%, and `OP_INVOKE` which takes a whopping
42% of the total running time.

So we've got three hotspots to optimize? Actually no. Because it turns out all
of them spend almost all of their time calling the same function: `tableGet()`.
That function claims a whole 72% of the execution time (inclusive). Now, in a
dynamically-typed language, we expect to spend a fair bit of time looking stuff
up in hash tables -- it's sort of the price of dynamism. But, still, *wow.*

If you take a look at `tableGet()`, you'll see it's mostly a wrapper around a
call to `findEntry()` where the actual hash table lookup happens. To refresh
your memory, here it is in full:

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

When running that previous benchmark -- on my machine, at least -- the VM spends
70% of the total execution time on one line. Any guesses as to which one? No?
It's this:

```c
  uint32_t index = key->hash % capacity;
```

That pointer reference isn't the problem. It's the little `%`. It turns out the
modulo operator is *really* slow. Much slower than other <span
name="division">arithmetic</span> operators. Can we do something better?

<aside name="division">

Except for its sister operation division, which is equally slow.

</aside>

In the general case, it's really hard to re-implement a fundamental arithmetic
operator in user code in a way that's faster than what the CPU itself can do.
After all, all of our C code must ultimately compile down to the CPU's own
arithmetic operations. If there were tricks we could use to go faster, it would
already be using them.

However, we *can* take advantage of the fact that we know more about our problem
than the CPU does. The reason we're using modulo here is to take a key string's
hash code and wrap it to fit within the bounds of the table's entry array. That
array starts out at eight elements and grows by a factor of two each time. We
know -- and the CPU and C compiler do not -- that out our table's size will
always be a power of two.

And because we're clever bit twiddlers, we know a faster way to calculate the
remainder of a number modulo a power of two: *masking*. For example, let's say
we want to calculate:

**todo: illustrate these**

```
228 % 64 = 37
```

Decimal doesn't illuminate, but binary does:

```
11100101 229
       %
01000000  64
       =
00100101  37
```

Note how every 1 bit in the dividend at or to the left of the divisor's only 1
bit gets discarded. All of the remaining 1 bits in the dividend -- all the bits
above the rightmost 0 bits pass through and form the result. In other words, it's
exactly like taking the dividend and doing a bitwise and with one less than the
divisor:

```
11100101 229
       &
00111111  63
       =
00100101  37
```

I'm not enough of a mathematician to *prove* to you that this works, but if you
think it through, it should make sense. This means we can replace that slow
modulo operator with a fast subtraction followed by a very fast bitwise and.

We can simply change that offending line of code to:

```c
  uint32_t index = key->hash & (capacity - 1);
```

This code is still very performance critical, though, and any extra work we can
eliminate, we should. We are performing that subtraction every time, but the
result of `(capacity - 1)` only changes infrequently, when the table grows or
shrinks. We can cache that result and reuse it. In fact, we can go one further.
If we cache the *mask* used to wrap a hash key into the table size, we don't
need to also store the *capacity*. It's trivial to calculate the capacity in
the few places we need it by just adding one to the mask.

So, instead of storing the entry array capacity in Table, we'll directly store
the bit mask:

^code table-capacity-mask (1 before, 1 after)

I went ahead and changed the name to make it clearer to our future selves
reading the code that this number no longer means the size of the array.

With that field, to wrap a key, we simply apply the mask:

^code initial-index (1 before, 1 after)

CPUs love bitwise operators, so it's hard to improve on that. Before we can try
that and see how the perf looks, we need to go through the VM and fix every
piece of code that was using the old `capacity` field to work with the new
`capacityMask` one. This is going to be kind of a chore. Sorry.

---

First, we initialize the field:

^code init-capacity-mask (1 before, 1 after)

The initial value doesn't matter much since it only has this value when the
array is `NULL` and the hash table checks for that before using the capacity or
mask to index into it.

### findEntry()

There are a couple of other changes to make in `findEntry()`. Our linear
probing search may need to wrap around the end of the array, so there is another
modulo to update:

^code next-index (4 before, 1 after)

This one is just as slow as the other, but it didn't show up in the profile
because probing past the first bucket is rare and probing at just the right
spot to wrap around even rarer.

We want to use the new name for this field consistently across the VM, so the
declaration of `findEntry()` is different too:

^code find-entry

We pass in the mask, not the actual capacity. Of course, this implies that we
need to also fix everything that calls `findEntry()`.

### adjustCapacity()

First up is `adjustCapacity()`. Here we switch to the new name when we call
`findEntry()`:

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
