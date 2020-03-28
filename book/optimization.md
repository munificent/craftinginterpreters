^title Optimization
^part A Bytecode Virtual Machine

> The evening's the best part of the day. You've done your day's work. Now you
> can put your feet up and enjoy it.
>
> <cite>Kazuo Ishiguro, <em>The Remains of the Day</em></cite>

Think of this chapter as a bonus. If I still lived in New Orleans, I'd call it a
*lagniappe* -- a little something extra given for free to a customer. You've got
a whole book and a complete virtual machine already, but I wanted you to have
some fun tweaking clox to squeeze more performance out it.

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
the bit mask. We'll keep the <span name="rename">same</span> `capacity` field
but its meaning and value is now different. It now represents the *mask* and
its value is always one *less* than the size of the entry array.

<aside name="rename">

Honestly, the code is clearer if we rename the field to `capacityMask`. But
doing so means a slew of very uninteresting code changes to drag you through, so
in the interest of expedience, I left the name alone.

</aside>

With that interpretation, to wrap a key, we simply apply the mask:

^code initial-index (1 before, 1 after)

CPUs love bitwise operators, so it's hard to improve on that. Before we can try
that and see how the perf looks, we need to go through the VM and fix every
piece of code that uses the `capacity` field and update it to work with the
new value it represents. This is going to be kind of a chore. Sorry.

**todo: figure out what subsections to have**

### findEntry()

There are a couple of other changes to make in `findEntry()`. Our linear
probing search may need to wrap around the end of the array, so there is another
modulo to update:

^code next-index (4 before, 1 after)

This one is just as slow as the other, but it didn't show up in the profile
because probing past the first bucket is rare and probing at just the right
spot to wrap around even rarer.

### adjustCapacity()

There are a few changes to make in `adjustCapacity()`. First, when we allocate
the new array, we need to calculate the size from the mask:

^code adjust-alloc (1 before, 1 after)

Then we iterate over the array to initialize the empty entries:

^code adjust-init (1 before, 1 after)

The difference here is that the `<` checking to exit the loop has become a `<=`
since the value of the mask is now the index of the last element, not the size
of the array.

^code re-hash (1 before, 1 after)

When we free the old array, the garbage collector wants to correctly track how
many bytes are released:

^code adjust-free (3 before, 1 after)

There's going to be a lot of `+ 1` in and `<=` in this rest of this section.
This is fertile ground for subtle off-by-one bugs, so we should tread carefully
to ensure we don't get bitten.

### Table operations

Moving up the abstraction stack, we get to the main hash table operations that
call `adjustCapacity()` and `findEntry()`. When we grow the array before
inserting a new entry into it, we need to calculate the capacity:

^code table-set-grow (1 before, 1 after)

### other changes

All the way at the beginning, when we first initialize the field:

^code init-capacity-mask (1 before, 1 after)

As you've seen, when we need to calculate the actual array size from the
`capacity` field we get that by adding one. When the hash table is empty, the
size is zero, so initializing the `capacity` *mask* to -1 will correctly yield
that size when we increment it.

When copying all of the entries from one hash table to another, we iterate over
the source table's array:

^code add-all-loop (1 before, 1 after)

That's another `<` to `<=` change.

The `findEntry()` function has a sister function, `tableFindStrings()` that does
a hash table lookup for interning strings. We need to make the same changes
there -- actual optimizations -- that we made in `findEntry()`. We use the mask
as a mask when wrapping the string's hash key:

^code find-string-index (4 before, 2 after)

And also when the linear probing wraps around:

^code find-string-next (3 before, 1 after)

### Memory manager changes

There are a few places in the memory management code where we access a hash
table's capacity. First, using a `<=` in the loop when we mark all of the
entries in a table:

^code mark-table (1 before, 1 after)

And another one in the loop to remove unmarked entries from the string table:

^code remove-white (1 before, 1 after)

Finally, the garbage collector tracks the amount of memory it frees so needs an
accurate count of the entry array size:

^code free-table (1 before, 1 after)

That's our last `+ 1`! That was a lot of plumbing just to turn a single `%` into
a `&`. Let's see if it was worth it by running that zoological benchmark. On my
machine, before the optimization the benchmark runs in about 5.22 sectons. With
this optimization, the total runtime drops to 2.60s. That's almost exactly half
the time, meaning our optimization got the benchmark running about twice as
fast. That is a <span name="fault">massive</span> win when it comes to
optimization. Usually you feel good if you can claw a few percent points here or
there.

**todo: chart**

And, because methods, fields, and global variables are so prevalent in Lox
programs, this tiny optimization will improve performance across the board.
Almost every Lox program will benefit.

<aside name="fault">

We probably shouldn't congratulate ourselves *too* strongly here. Yes, it's a
big improvement, but only *relative to our own previous implementation*. We get
the credit for the optimization, but we also deserve the blame for its original
speed too.

</aside>

Now, the point of this section is *not* that the modulo operator is profoundly
evil and you should stamp it out of every program you ever write. Nor is that
tiny micro-optimization is a vital engineering skill. It's very rare that a
performance problem has such a narrow, effective solution. We just got lucky.

The point is that we didn't *know* that the modulo operator was a performance
drain until our profiler told us so. If we wandered around our VM's codebase
blind guessing at hotspots, we likely wouldn't have noticed it. What I want you
to take away from this is how important it is to have a profiler in your
programmer's toolbox.

To reinforce that point, let's go ahead and run the same benchmark in our
now-optimized VM and see what the profiler shows us. On my machine, `tableGet()`
is still a fairly large chunk of execution time. That's to be expected for a
dynamically-typed language. But it has dropped from 72% of the total execution
time down to 35%. That's much more inline with what we'd like to see and shows
that our optimization didn't just make the program faster, but made it faster
*in the way we expected*. Profilers are just as useful for verifying solutions
as they are for discovering problems.

## NaN Boxing

This next optimization has a very different feel. Thankfully, despite the odd
name, it does not involve pugilistics with your grandmother. It's different, but
not, like, *that* different. With our previous optimization the profiler
basically told us where the problem was and we just had to use a little
ingenuity to come up with a solution.

**todo: illustrate?**

This optimization is more subtle, and its performance effects more diffuse
across the virtual machine. The profiler won't help us come up with this.
Instead, it was invented by <span name="someone">someone</span> thinking deeply
about the lowest levels of machine architecture.

<aside name="someone">

I'm not sure who first invented the optimization we're about to implement. The
earliest source I can find is David Gudeman's 1993 paper "Representing Type
Information in Dynamically Typed Languages". Everyone else cites it. But Guteman
himself says the paper isn't novel work but instead "gathers together a body of
folklore".

Maybe the inventor has been lost to the mists of time, or maybe it's been
reinvented a number of times. Anyone who ruminates on IEEE 754 long enough
probably starts thinking about trying to stuff something useful into all those
unused NaN bits.

</aside>

Like the heading says, this optimization is called "NaN boxing" or sometimes
"NaN tagging". Personally I like the latter name because "boxing" tends to imply
some kind of heap-allocated representation, but it seems to be the more
widely-used term. It has to do with how we represent values in the VM.

On a 64-bit machine, our Value type takes up 16 bytes. The struct has two
fields, a type tag and a union for the payload. The largest fields in the union
are an Obj pointer and a double, which are both 8 bytes. The compiler aligns
those to an 8-byte boundary, which means the tag takes up a full 8 bytes too.

**todo: illustrate**

That's pretty big. If we could cut that down, then the VM could pack more values
into the same amount of memory. Most computers have plenty of RAM these days, so
the direct memory savings aren't a huge deal. But smaller values mean more data
fits in a cache line. That means fewer cache misses which affects *speed*.

But if Values need to be aligned to their largest payload size and a Lox number
or Obj pointer needs a full 8 bytes, how can we get any smaller? In a
dynamically typed language like Lox, each value needs to not just carry its
payload, but enough additional information to be able to identify the value's
*type* at runtime. If a Lox number is already using the full 8 bytes, where
could we squirrel away a couple of extra bits to tell the runtime "this is a
number"?

This is one of the perennial problems for dynamic language hackers. It
particularly bugs them because statically-typed languages don't generally have
this problem. The type of each value is known at compile time, so no extra
memory is needed at runtime to track it. When your C compiler compiles 32-bit
int, the resulting variable gets *exactly* 32 bits of storage.

Dynamic language folks hate losing ground to the static camp, so they've come up
with a number of very clever ways to pack some type information and a payload
into a small number bits. NaN boxing is one of those. It's a particularly good
fit for languages like JavaScript and Lua where all numbers are double-precision
floating point. That describes Lox too.

### What is (and is not) a number?

Before we can start optimizing, we need to really understand how your friend the
CPU represents floating point numbers. Almost all machines today use the same
mechanism for converting a real number into a series of bits, and that scheme is
encoded in the venerable scroll [IEEE 754][754], known to mortals as "IEEE
Standard for Floating-Point Arithmetic".

[754]: https://en.wikipedia.org/wiki/IEEE_754

A 64-bit double-precision IEEE floating point number looks something like this:

**todo: illustrate**

*   The first bit is the <span name="sign">*sign bit*</span> and indicates
    whether the number is positive or negative.

*   After that is 11 *exponent* bits. These tell you how far the number is
    shifted away from the decimal (well, binary) point.

*   The remaining 52 bits are the *fraction* or *mantissa* bits. They represent
    the significant digits of the number, as a binary integer.

I know that's a little vague, but this chapter ain't a deep dive on floating
point representation. If you want to know how about how the exponent and
mantissa play together, there are better explanations out there already than I
could write.

<aside name="sign">

Since the sign bit is always present even if the number is zero, that implies
that "positive zero" and "negative zero" have different bit representations and,
indeed, IEE 754 does have both of those.

</aside>

The important part for our purposes is that the spec carves out a special case
exponent value. When all of the exponent bits are set, then instead of just
being a really big number, the value gets a special meaning. These values are
"not a number" (hence, "NaN") values. These are used to represent special edge
cases like infinity or the result of trying to divide a number by zero.

*Any* double who's exponent bits are all set, regardless of the mantissa bits.
That means there's lots and lots of *different* NaN bit patterns. IEEE 754
divides those into two categories. Values where the highest mantissa bit is zero
are called *signalling* NaNs and the others are *quiet* NaNs. Signalling NaNs
are intended to be the result of erroneous computations, like dividing by zero.
A chip <span name="abort">may</span> detect when one of these values is produced
and abort a program completely. They may self-destruct if you try to read one.

<aside name="abort">

I don't know if any CPUs actually *do* trap signalling NaNs and abort. The spec
just says they *could*.

</aside>

Quiet NaNs are generally supposed to be safer to use. They also don't represent
useful numbers, but they should at least not set your hand on fire if you touch
them.

Every double with all of its exponent bits set and its highest mantissa bit set
is a quiet NaN. That leaves 52 bits unaccounted for. We'll sidestep one of those
so that we don't step on Intel's "QNan Floating-Point Indefinite" value, leaving
us 51 bits. Those bits can be anything. We're talking 2,251,799,813,685,248
unique quiet NaN bit patterns.

**todo: illustrate**

The way to think about it is that a 64-bit double has enough room to store all
of the various different numeric values and *also* room for another 51 bits of
data that we can use for whatever we want. That's obviously plenty of room to
set aside a couple of bit patterns to represent Lox's `nil`, `true`, and `false`
values. But what about Obj pointers? Don't pointers need a full 64 bits too?

Fortunately, we have another trick up our other sleeve. Yes, technically
pointers on a 64-bit architecture are 64 bits. But, no architecture I know of
actually uses that entire address space. Instead, most widely-used chips today
only ever use the low <span name="48">48</span> bits. The remaining 16 bits are
either unspecified or always zero.

<aside name="48">

48 bits is still enough room to address 262,144 gigabytes of memory so that
should be sufficient for a while.

</aside>

If we've got 51 bits, we can stuff a 48-bit pointer in there with three bits
to spare. Just enough to store a tiny type tag to distinguish between `nil`,
Booleans, and Obj pointers.

That's what NaN boxing is. Within a single 64-bit double, you can store all of
the different floating pointer numeric values, as well as a pointer and a couple
of other special sentinel values. Half the memory usage as our current Value
struct, while retaining all of the fidelity.

What's particularly nice about this representation is that there is no need to
*convert* a raw double value into a "boxed" form. Lox numbers *are* just normal
64-bit doubles. We'll still need to *check* their type before we use them,
since Lox is dynamically typed, but we don't need to do any bit shifting or
pointer indirection to go from "value" to "number".

For the other value types, there is a conversion step, of course. But,
fortunately, our VM hides all of the mechanism to go from values to raw types
behind a handful of macros. Rewrite those to implement NaN boxing and the rest
of the VM should just work.

### Conditional support

I know the details of the new value representation aren't very clear in your
head yet. Don't worry, they will crystallize as we work through the
implementation. Before we get to that, we're going to put some compile-time
scaffolding in place.

For our previous optimization, we just fixed the code in place and called it
done. This one is a little different. NaN boxing relies on some very low-level
details of how a chip represents floating point numbers and pointers. It
*probably* works on most CPUs you're likely to encounter, but you can never be
totally sure.

It would suck if our VM completely lost support for an architecture just because
of its value representation. To avoid that, we'll maintain support for *both*
the old tagged union implementation of Value and the new NaN-boxed form. A
simple compile-time flag selects the new representation:

^code define-nan-boxing (2 before, 1 after)

If that's defined, the VM uses the new form. Otherwise it defaults to the old
style. The few pieces of code that are care about the details of the value
representation -- mainly the handful of macros for wrapping and unwrapping
Values -- will check this. The rest of the VM can continue along its merry way.

Most of the work happens in the "value" module where we add the section for the
new type:

^code nan-boxing (2 before, 1 after)

When Nan boxing is enabled, the actual type of a Value is a flat unsigned 64-bit
integer. We could use double instead which would make the macros for dealing
with Lox numbers a little simpler. But all of the other macros will need to do
some bitwise operations and uint64_t is a much friendlier type for that. Outside
of this module, the rest of the VM doesn't really care one way or the other.

Before we start re-implementing those macros, we need to close the `#else`
branch of the `#ifdef`:

^code end-if-nan-boxing (1 before, 2 after)

All that remains for us now is filling in that first `#ifdef` section with new
implementations of all the stuff already in the `#else` side. We'll work through
it one value type at a time, from easiest to hardest.

### Numbers

We'll start with numbers since they have the most direct representation under
NaN boxing. To "convert" a C double to a NaN boxed Lox Value, we don't need to
touch a single bit -- the representation is exactly the same. But we do need to
convince our C compiler of that fact, which is made harder by defining Value to
be a uint16_t.

We need to get the compiler to take a set of bits that it thinks are a double
and use those same bits as a uint64_t, or vice versa, called *type punning*. The
<span name="close">closest</span> thing to an officially supported way of doing
that in C is through a union:

<aside name="close" class="bottom">

I say "closest" because it's not *entirely* clear to me that the standard
officially supports this even though the technique has been in wide use since
the early days of C. I've read a number of interpretations of the spec on this
point and arguments for and against seem to read more like Biblical hermeneutics
or literary criticism than any sort of clear answer.

Regardless of what the Good Spec says, just about every compiler under the sun
permits it, so relying on it is, at worse, a minor sin.

</aside>

^code double-union (1 before, 2 after)

The idea is that you store a value into one of the union's fields and then read
out the other field. Voilà, the bits have been transmuted from the first field's
type to the second's. Alas, I don't know any elegant way to pack that operation
into a single expression, so the conversion macros will have to call out to
helper functions. Here's the first:

^code number-val (1 before, 2 after)

To convert a C double to a Lox Value -- in other words a uint64_t -- we rely on:

^code num-to-value (1 before, 2 after)

This creates a temporary union, stores the double in it, and then yanks out the
uint64_4 whose bits overlap that in memory. This looks *really* slow: a function
call, a local variable, an assignment, and a read. Fortunately, compilers are
smart enough to optimize that all down to absolutely nothing.

"Unwrapping" a Lox number is the mirror image:

^code as-number (1 before, 2 after)

That macro calls this function:

^code value-to-num (1 before, 2 after)

It works exactly the same except it flips the fields that it writes to and reads
from. Again, the compiler will eliminate all of it.

That was a lot of code to ultimately do nothing but silence the C type checker.
Doing a runtime type *test* on a Lox number is a little more interesting. If all
we have are exactly the bits for a double, how do we tell that it *is* a double?

^code is-number (1 before, 2 after)

It's time to get bit twiddling. We know that every Value that is *not* a number
will use a special quiet NaN representation. And we're taking for granted that
we have correctly avoided any of the meaningful NaN representations that may
actually be produced by doing arithmetic on numbers.

If the double has all of its NaN bits set, and the quiet NaN bit set, and
maybe one more for good measure, we can be pretty certain it is one of the bit
patterns we ourselves have set aside for other types. To check that, we mask
out all of the bits except for are set of quiet NaN bits. If *all* of those bits
are set, it must be a value of some other Lox type.

The set of quiet NaN bits are declared like this:

^code qnan (1 before, 2 after)

It would be nice if C supported binary literals. But if you do the conversion,
you'll see that value is the same as:

**todo: illustrate**

Which is exactly all of the exponent bits, plus the quiet NaN bit, plus one
extra to dodge that Intel value. That's numbers.

### Nil, true, and false

The next type to handle is `nil`. That's pretty simple since there's only one
`nil` value and thus we only need a single bit pattern to represent all values
of that type. It's a "singleton" value. There are two other singleton values,
the two Boolean values `true` and `false`. We need three unique bit patterns.

Two bits gives us four different combinations, which is plenty. So we will use
the two lowest bits of our unused bit space as a "type tag" to determine which
of these three singleton values we're looking at.

**todo: illustrate tag bit location**

The three type tags are defined like so:

^code tags (1 before, 2 after)

Our representation of `nil` is thus all of the bits required to define our
quiet NaN representation along with the `nil` type tag bits:

**todo: illustrate**

In code, that's:

^code nil-val (2 before, 1 after)

We simply bitwise or the quiet NaN bits and the type tag, and then do a little
cast dance to teach the C compiler what we want those bits to mean.

Since `nil` has only a single bit representation, we can use equality on
uint64_t to see if a Value is `nil`:

<span name="equal"></span>

^code is-nil (2 before, 1 after)

<span name="equal"></span>

<aside name="equal" class="bottom">

Note that using `==` would not do the right thing if our typedef for Value was
double instead of uint64_t. Using C's equality operator on two doubles steps
into the realm of IEEE 754 equality which has lots of weird special rules. Some
doubles with distinct bit patterns are considered equivalent, like positive and
negative zero.

</aside>

You can guess what defining the `true` and `false` values looks like:

^code false-true-vals (2 before, 1 after)

To convert a C bool into one a Lox Boolean, we rely on these two singleton
values and the good old conditional operator:

^code bool-val (2 before, 1 after)

There's probably a more clever bitwise way to do this, but my hunch is that the
compiler can figure that out faster than I can.

Going the other direction is simpler:

^code as-bool (2 before, 1 after)

Since we know there are exactly two bit Boolean representations in Lox -- unlike
in C where any non-zero value can be considered "true" -- if it ain't one, it
must be the other. This macro does assume you only call it on a Value that you
know *is* a Lox Boolean. To do that, there's one more macro:

^code is-bool (2 before, 1 after)

That looks a little weird. It *kind* of looks like it only checks for `false`
but what about `true`? A more obvious macro would look like:

```c
#define IS_BOOL(v) ((v) == TRUE_VAL || (v) == FALSE_VAL)
```

Unfortunately, that's not safe. The expansion mentions `v` twice which means if
that expression has any side effects, they will be executed twice. We could call
out to a separate function, but, ugh, what a chore.

Instead, we can take advantage of the bits we chose for the type tags. Note that
both `true` and `false` have the second bit set bit `nil` does not. If our Value
has all of its quiet NaN bits set, we know it's not a number. And if the second
bit is set, we know it's not `nil`. It must be `true` or `false`. Conveniently,
`FALSE_VAL` is just the right bit pattern to check the quiet NaN bits and the
second tag bit, so we just test against that.

**todo: illustrate**

That's a little more clever than I like to be with my bit hackery, but here we
are.

### Objects

The last value type is the hardest. Unlike the singleton values, there are
billions of different pointer values we need to box inside a NaN. This means we
need both some kind of tag to indicate that these particular NaNs *are* Obj
pointers and we need room for the pointers themselves.

The tag bits we used for the singleton values are in the region where I decided
to store the pointer itself, so we can't easily use a different <span
name="ptr">bit</span> there to indicate that the value is an object reference.
However, there is another bit we aren't using. Since all our NaN values are not
numbers -- it's right there in the name -- the sign bit isn't used for anything.
We'll go ahead and use that as the type tag for objects. If one of our quiet
NaNs has its sign bit set, then it's an Obj pointer, otherwise it must be one of
the previous singleton values.

<aside name="ptr">

We actually *could* use the lowest bits to store the type tag even when the
value is an Obj pointer. That's because Obj pointers are always aligned to a
8-byte boundaries since Obj contains a 64-bit field. That in turn implies that
the three lowest bits of an Obj pointer will always be zero. We could store
whatever we wanted in there and just mask it off before derefering the pointer.

This is another value representation optimization called "pointer tagging".

</aside>

If the sign bit is set, then the remaining low bits store the pointer to the
Obj.

**todo: illustrate**

To convert a raw Object pointer to a Value, we simply take the pointer and set
all of the quiet NaN bits and the sign bit:

^code obj-val (1 before, 2 after)

The pointer itself is a full 64 bits and, in <span name="safe">principle</span>,
it could thus overlap with some of those quiet NaN and sign bits. But in
practice, at least on the architectures I've tested, everything above the 48-th
bit in a pointer is always zero. There's a lot of casting going on here, which
I've found is necessary to satisfy some of the pickiest C compilers.

<aside name="safe">

In this book, we mostly try to follow the letter of the law when it comes to
our Java and C code, so this paragraph is pretty dubious. There comes a point
when doing optimization where really start to push the boundary of not just
what the spec *says* you can do but what a real compiler and chip actually lets
you get away with.

There are certainly risks when stepping outside of the spec, but there are
rewards in that lawless territory too. It's up to you to decide if your
optimizations are worth it.

</aside>

That macro needs access to the sign bit:

^code sign-bit (2 before, 2 after)

An IEEE 754 double stores the sign at the very highest bit position.

**todo: illustrate**

To get the Obj pointer back out, we simply mask off all of those extra bits:

^code as-obj (1 before, 2 after)

The `~`, if you haven't done enough bit munging to encounter before, it bitwise
not. It flips all ones and zeroes in its operand. By masking the value's bits
with the bitwise negation of the quiet NaN and sign bits, we clear those bits
and let the pointer bits remain.

**todo: illustrate**

One last macro. A Value storing an Obj pointer will have its sign bit set, but
so will any negative number. To tell if a Value is an Obj pointer, we need to
check that both the sign bit and all of the quiet NaN bits are set.

^code is-obj (1 before, 2 after)

This is similar to how we detect the type of the singleton values except this
time we use the sign bit as the tag.

### Value functions

That's it for macros so we are almost done implementing this optimization. We
have the representation itself fully supported. However, there are a couple of
functions in the "value" module that peek inside the otherwise black box of
Value and work with it's encoding directly. We need to fix those too.

The first is `printValue()`. It has separate code for each value type. We no
longer have an explicit type enum we can switch on so instead we use a series
of type tests to handle each kind of value:

^code print-value (1 before, 1 after)

This is technically a tiny bit slower than a switch, but compared to the
overhead of actually writing to a stream, it's negligible.

We keep the old code to continue to support the original tagged union
representation so we close the conditional section after it:

^code end-print-value (1 before, 1 after)

The other operation is testing two values for equality:

^code values-equal (1 before, 1 after)

It doesn't get much simpler than that! If the two bit representations are
identical, the values are equal. That does the right thing for the singleton
values since each has a unique bit representation and they are only equal to
themselves. It also does the right thing for Obj pointers, since objects use
identity for equality -- two Obj references are only equal if they point to the
exact same object.

It's *mostly* correct for numbers too. Most floating-point numbers with
different bit representations are distinct numeric values. Alas, IEEE 754
contains a little pothole to trip us up. For reasons that aren't entirely clear
to me, the spec mandates that NaN values are *not* equal to *themselves.* This
isn't a problem for the special quiet NaNs that we are using for our own
purposes. But it's possible to produce a "real" arithmetic NaN in Lox and if we
want to correctly implement IEEE 754 numbers then the resulting value is not
supposed to be equal to itself. More concretely:

```lox
var nan = 0/0;
print nan == nan;
```

IEEE 754 says this program is supposed to print "false". It does the right thing
with our old tagged union representation because the `VAL_NUMBER` case applies
`==` to two values that the C compiler knows are doubles. Thus the compiler
generates the right CPU instruction to perform an IEEE floating-point equality.

Our new representation breaks that by defining Value to be a uint64_t. If we
want to be *fully* compliant with IEEE 754, we need to handle this case:

^code nan-equality (1 before, 1 after)

I know, it's weird. And there is a performance cost to doing this type test
every time we check two Lox values for equality. If we are willing to sacrifice
a little compatibility -- who *really* cares if NaN is not equal to itself in
real code? -- we could leave this off. I'll leave it up to you to decide how
pedantic you want to be.

Finally, we close the conditional compilation section around the old implementation:

^code end-values-equal (1 before, 1 after)

And that's it. This optimization is complete, as is our clox virtual machine.
That's the last line of new code in the book.

### Evaluating performance

The code is done, but we still need to figure out if we actually made anything
better with these changes. Evaluating an optimization like this is very
different from the previous one. There, we had a clear hotspot visible in the
profiler. We fixed that part of the code and could instantly see the hotspot
get faster.

The effects of changing the value representation are much more diffuse. The macros
are expanded in place wherever they are used, so the performance changes are
spread across the codebase in a way that's hard for many profilers to track
well, especially in an <span name="opt">optimized</span> build.

<aside name="opt">

When doing profiling work, you almost always want to profile an optimized,
"release" build of your program since that reflects the performance story your
end users experience. Compiler optimizations like inlining can dramatically
affect which parts of the code are performance hotspots. Optimizing a debug
build risks sending you off "fixing" problems that the compiler will already
solve for you.

Make sure you don't accidentally benchmark and optimize your debug build.

</aside>

We also can't easily reason about the effects of our change. We've made values
smaller, which reduces cache misses all across the VM. But the actual real-world
performance effect of that change is highly dependent on the memory use of the
Lox program being run. A tiny Lox microbenchmark may not have enough values
scattered around in memory for the effect to be noticeable, and even things
like the addresses handed out to us by the C memory allocator can affect things.

If we did our job right, basically everything gets a little faster, especially
on larger, more complex Lox programs. Doing performance work like this is
unnerving because you can't easily *prove* that you've made the VM better. You
can't point to a single surgically targeted microbenchmark and say, "There, see?"

Instead, what we really need is a *suite* of larger benchmarks. Ideally, they
would be distilled from real-world Lox applications -- not that such a thing
really exists for a toy language like Lox. Then we can measure the aggregate
performance changes across all of those. I did my best to cobble together a
handful of larger Lox programs. On my machine, the new value representation
seems to make everything roughly 10% across the board.

That's not a huge improvement, especially compared to profound affect of making
hash table lookups faster. I added this optimization in large part because it's
a good example of a certain *kind* of performance work you're likely to
experience, and, honestly, because I think it's technically really cool. It
might not be the first thing I reached for if I were seriously trying to make
clox faster. There is probably other lower-hanging fruit.

But, if you find yourself working on a virtual machine where all of the easy
wins have been taken, then at some point you may want to think about tuning your
value representation. This chapter has hopefully shined a light on some of the
options you have in that area.

## Where to Next

We'll stop here with the Lox language and our two interpreters. We could tinker
on it forever, adding new language features and clever speed improvements. But,
for this book, I think we've reached a natural place to call our work complete.
I won't rehash everything we've learned in the past many pages. Instead, I'd
like to take a minute to talk about where you might go from here. What is the
next step in your programming language journey?

Most of you probably won't spend a significant part of your career working in
compilers or interpreters. It's a pretty small slice of the computer science
pie, and an even smaller segment of software engineering in industry. That's OK.
Even if you never work on a compiler again in your life, you will certainly
*use* one, and I hope this book has equipped you with a better understanding of
how the programming languages you use are designed and implemented.

You have also learned a handful of important, fundamental data structures, and
gotten some practice doing low-level profiling and optimization work. That kind
of expertise is helpful no matter what kind of code you write.

Even more important, I think, is that I hope to have given you a new way of
<span name="domain">looking</span> at and solving problems. Even if you never
work on a language again, you may be surprised to discover how many programming
problems can be seen as "language-like" in some way and then the tools you've
gained in parsing and interpreting can be brought to bear on them. Maybe that
report generator you need to write can be modeled as a little series of
stack-based "instructions" that the generator "executes". That user interface
you need to render looks an awful lot like traversing an AST.

<aside name="domain">

This goes for other domains too. I don't think there's a single topic I've
learned in programming -- or even outside of programming -- that I haven't ended
up finding useful in other areas. One of my favorite aspects of software
engineering is how much it rewards those with eclectic interests.

</aside>

If you do want to go further down the programming language rabbit hole, here
are some suggestions for which branches in the tunnel to explore:

*   Our simple single-pass bytecode compiler pushed us towards mostly runtime
    optimization. In a mature language implementation, compile-time optimization
    is generally more important, and the field of compiler optimizations is
    incredibly rich. Grab a classic <span name="cooper">compilers</span> book,
    and rebuild the front end of clox or jlox to be a sophisticated compilation
    pipeline with some interesting intermediate representations and optimization
    passes.

    Dynamic typing will place some restrictions on how far you can go, but there
    is still a lot you can do.

    <aside name="cooper">

    I like Cooper and Torczon's "Engineering a Compiler" for this. Appel's
    "Modern Compiler Implementation" is also well regarded.

    </aside>

*   In this book, I aim to be correct, but not particularly rigorous. My goal is
    mostly to give you an *intuition* and a feel for doing language work. If you
    like more precision, then the whole world of programming language academia
    is waiting for. Languages and compilers have been studied formally since
    before we even had computers, so there is no shortage of books and papers on
    parser theory, type systems, and formal logic. Going down this path will
    also teach you how to read CS papers, which is a valuable -- and distinct --
    skill in its own right.

*   Or, if you just really enjoy hacking on and making languages, you can take
    Lox and turn it into your own <span name="license">plaything</span>. Change
    the syntax to something that delights your eye. Add missing features or
    remove ones you don't like. Jam new optimizations in there.

    <aside name="license">

    The *text* of this book is copyrighted to me, but the *code* and the
    implementations of jlox and clox use the very permissive [MIT license][].
    You are more than welcome to [take either of those interpreters][source] and
    do whatever you want with them. Go to town.

    If you do choose to release your own language based on these, it would be
    good to change *name*, mostly to avoid confusing people about what "Lox"
    refers to.

    </aside>

    Eventually you may get to a point where you have something you think others
    could use as well. That gets you into the very distinct world of programming
    language *popularity.* Expect to spend a ton of time writing documentation,
    example programs, tools, and useful libraries. The marketplace is crowded
    with languages vying for users and to compete in that space you'll have to
    put on your marketing hat and *sell*. Not everyone enjoys that kind of
    public-facing work, but if you do, it can be incredibly gratifying to see
    people use your language to build their own programs.

Or maybe this book alone has satisfied your craving and you'll stop here.
Whichever way you go, or don't go, I hope you found this book worth your time.

[mit license]: https://en.wikipedia.org/wiki/MIT_License
[source]: https://github.com/munificent/craftinginterpreters

<div class="challenges">

## Challenges

Assigning homework on the last day of school seems cruel but if you really want
something to do during your summer vacation:

1.   Fire up your profiler, run a couple of benchmarks, and look for other
     hotspots in the VM. Do you see anything in the runtime that you can
     improve?

2.   Many strings in real-world user programs are very small, often only a
     character or two. This is less of a concern in clox because we intern
     strings, but most VMs don't. For those that don't, heap-allocating a tiny
     character array for each of those little strings and then representing the
     value as a pointer to that array is quite wasteful. Often the pointer is
     smaller than the string's characters. A classic trick is to have a separate
     value representation for small strings that stores the characters right
     inline in the value.

     Starting from clox's original tagged union representation, implement that
     optimization. Write a couple of relevant benchmarks and see if it helps.

3.   Reflect back on your experience with this book. What parts of worked well
     for you? What didn't? The more you understand about your personal learning
     style, the more effectively you can upload knowledge into your head. You
     can specifically target material that teaches the way you learn best.

</div>
