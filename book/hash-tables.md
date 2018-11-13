^title Hash Tables
^part A Bytecode Virtual Machine

We're getting close to adding variables to our burgeoning virtual machine. That
means we'll soon need a way to look a value given a variable's name. Later, when
we add classes, we'll need a way to store fields on instances.

The perfect data structure for that is a *hash table*. You probably already know
what a hash table is, even if you don't know it by that name. If you're a Java
programmer, you call them "HashMaps". C# and Python call them "dictionaries". In
C++, it's just "map" while Lisp calls them "associative arrays". Objects in
JavaScript and Lua are hash tables under the hood, which is what gives them
their flexibility.

Note that C isn't in that list.  In jlox, we could rely on Java's rich standard
library to spare us the trouble of building one from scratch. Now that we're in
C, it's up to us. So in this chapter, we're going to cover every inch of
implementing a hash table.

A hash table, whatever your language calls it, lets you associate a set of
*keys* with a set of *values*. Each key/value pair is an *entry* in the table.
Given a key, you can find its corresponding value. You can add new key/value
pairs and remove entries by key. If you add a new value for an existing key, it
replaces the previous entry.

Hash tables appear in so many languages because they are incredibly powerful.
Much of this power comes from one key metric: given a key, a hash table returns
corresponding value in <span name="constant">constant time</span>, *regardless
of how many keys are in the hash table.*

<aside name="constant">

More specifically, the *average-case* lookup time is constant. Worst-case
performance can be, well, worse. But, in practice, it's easy to avoid degenerate
behavior and stay on the fast path.

</aside>

That's pretty remarkable when you think about it. Imagine you've got a big stack
of business cards and I ask you to find a certain person. The bigger the pile
is, the longer it will take. Even if the pile is nicely sorted and you can do a
binary search, you're still talking `O(log(n))`. But with a hash table it takes
the same time to find that busines card if the stack has ten cards or a million.

## An Array of Buckets

A complete, fast hash table has a couple of moving parts. I'll introduce them
one at a time by going through a couple of smaller problems and solutions. Eventually, we'll build up to a data structure that can associate any set of names with their values.

For now, imagine if Lox was a *lot* more restricted in variable names. What if a
variable's name could only be a <span name="basic">single</span> lowercase
letter. How could we very efficiently represent a set of variable names and
their values?

<aside name="basic">

This limitation isn't *too* far-fetched. The initial versions of BASIC out of
Dartmouth only allowed variable names to be a single letter followed by one
optional digit.

</aside>

With only 26 possible variables (27 if you consider underscore a letter, I
guess), the answer is easy. Declare a fixed-size array with 26 elements. Each
element, we'll follow tradition and call them "buckets", represents a variable
with `a` starting at zero. If there's a value in the array at some letter's
index, then that key is present with that value. Otherwise, the bucket is empty
and that key/value pair isn't in the data structure.

Memory usage is great -- just a single reasonably-sized array. There's some
waste from the empty buckets, but it's not huge. There's no overhead for node
pointers, padding, or other stuff you'd get in something like a linked list or
tree.

Performance is even better. Given a variable name -- its character -- you can
subtract the ASCII value of `a` and use the result to index directly into the
array. Then you can either lookup the existing value or store a new value
directly into that slot. It doesn't get much faster than that!

This is sort of our Platonic ideal data structure. Lightning fast, dead simple,
and compact in memory. As we add support for more complex keys, we'll have to
make some concessions, but this is what we're aiming for. Even once we add in
hash functions, dynamic resizing, and collision resolution, this is still the
core of every hash table out there -- a contiguous array you can use the key to
index directly into.

## Load Factor and Wrapped Keys

Confining Lox to single-letter variables makes our job as implementers easier,
but it's probably on fun programming in a language that only gives you 26
storage locations. What if we loosened a little to allow variables up to <span
name="six">eight</span> characters long?

<aside name="six">

Again, this restriction isn't so crazy. Early linkers for C only treated the
first six characters of external identifiers as meaningful. Everything after
that was ignored. If you've ever wondered why the C standard library is so
enamored of abbreviation, it turns out it wasn't entirely because of the small
screens (or teletypes!) of the day.

</aside>

That's small enough that we can pack all eight characters into a 64-bit integer
and still treat the identifier as an array index. Or, at least, we could if we
could somehow allocate a 295,148 *petabyte* array. Memory's gotten cheaper over
time, but not quite *that* cheap. Even if we could make an array that big, it
would be heinously wasteful. Almost every bucket will be empty unless users
start writing way bigger Lox programs than we've anticipated.

Even though out indexes are 64 bits, we clearly don't need an array that large.
Instead, we can allocate array bigger than the number of entries we need, but
still a reasonable size. In order to keep the indexes in bounds, we'll wrap them
around using the modulo operator.

For example, say we need to store six entries. We could allocate an array of
maybe 16 elements, plenty enough to store them all with room leftover. We take
each key string as a 64 bit integer and modulo the array size (<span
name="power-of-two">16</span>) to fit it in bounds and get a bucket index. Then
we store the value there as usual.

<aside name="power-of-two">

I'm using powers of two for the array sizes here, but they don't need to be.
Some styles of hash table work best with powers of two, including the one we'll
build in this book. Others prefer prime number array sizes or have other rules.

</aside>

We solved our waste problem, but introduced a new one. Any two variables whose
key number has the same remainder when divided by the array size will end up in
the same bucket. Variables can **collide**. We do have some control of this by
tuning the array size. The bigger the array, the fewer collisions are likely to
occur.

Hash table implementers track this by measuring the table's **load factor**.
This is the number of entries divided by the number of buckets. So a hash table
with five entries and an array of 16 elements has a load factor of 0.3125. The
higher the load factor, the greater the number of collisions.

Part of the way we'll address that is by dynamically resizing the array. Just
like the dynamic arrays we implemented earlier, we'll reallocate and grow the
hash table's array as it fills up. Unlike a regular dynamic array, though, we
won't wait until the array is *full*. Instead, we pick a desired load factor and
grow the array when it goes over that.

## Collision Resolution

Even with a very low load factor, collisions can still occur. The [birthday
paradox][] tells us that as the number of entries in the hash table increases,
the change of colliion increases very quickly. We can pick a large array size to
reduce that, but it's a losing game. Say we wanted to store a hundred items in a
hash table. To keep the chance of collision below a still-pretty-high 10%, we
need an array with at least 47,015 elements. To get the chance below 1% requires
an array with 492,555 elements, over 4,000 thousand empty buckets for each one
in use.

**todo: aside or illustration about birthday paradox**

[birthday paradox]: https://en.wikipedia.org/wiki/Birthday_problem

Even then, making collisions rare doesn't mean they are impossible. We still
have to handle them gracefully when they occur. Users don't like it when their
programming language can look up variables correctly only *most* of the time.

### Separate chaining

Techniques for resolving collisions fall into two broad categories. The first is
**separate chaining**. Instead of each bucket containing a single entry, we let
it contain a collection of them. In the classic implementation, a bucket is a
pointer to the head of a linked list of entries. To look up an entry, you find
its bucket, then walk the list of entries until you find one with the matching
key.

In catastrophically bad cases where every entry collides in the same bucket, the
data structure degrades into a single unsorted linked list with O(n) lookup.
That's bad. In practice, it's easy to avoid that. As long as you keep the load
factor relatively low, most buckets will have on average one entry or fewer.

Separate chaining is conceptually simple -- it's literally an array of linked
lists. Most operations are straightforward to implement, even deletion which, as
we'll see, can be a pain. But it's not a great fit for modern CPUs. It has a lot
of overhead from pointers and tends to scatter little linked list <span
name="node">nodes</span> around in memory which isn't great for cache usage.

<aside name="node">

There are a few tricks to optimize this. Many implementations store the first
entry right in the bucket so that in the common case where there's only one, no
extra pointer indirection is needed. You can also make each linked list node
store a few entries to reduce the pointer overhead.

</aside>

### Open addressing

The other technique is <span name="open">called</span> **open addressing** or
(confusingly) **closed hashing**. With this technique, all entries live directly
in the bucket array, with one entry per bucket. If two entries collide in the
same bucket, we find a different bucket to overflow one of the entries into.
*That* bucket could be full too, so you sometimes need to search an entire
sequence of buckets to find an empty one.

<aside name="open">

It's called "open" addressing because the entry may end up at an address outside
of its prefered one. It's called "closed" hashing because all of the entries
stay inside the array of buckets.

</aside>

Storing all entries directly in a single big contiguous array is great for
keeping the memory representation simple and fast. But it makes all of the
operations on the hash table more complex. When inserting an entry, its bucket
may be full, sending us to look at another bucket. That bucket itself may be
occupied and so on. This process of finding an available bucket is called
**probing** and the order that you look in the buckets is a **probe sequence**.

There are a <span name="probe">number</span> of algorithms for determining
which buckets to probe and how to decide which entry goes in which bucket.
There's been a ton of research here because even slight tweaks can have a large
performance impact. And, on a data structure as heavily used as hash tables,
that performance impact touches a very large number of real-world programs
across a range of hardware capabilities.

<aside name="probe">

If you'd like to learn more (and you should, because some of these are really
cool), look into "double hashing", "cuckoo hashing", "Robin Hood hashing", and
anything those lead you to.

</aside>

As usual in this book, we'll pick the simplest one that gets the job done
efficiently. That's good old **linear probing**. When looking for an entry, we
look in the first bucket its key maps to. If it's not in there, we look in the
very next element in the array, and so on. If we reached the end, we wrap back
around to the beginning.

**TODO: walk through example inserting a few items in buckets**

The bad thing about linear probing is that it's prone to **clustering**. If you
have a lot of entries with numerically similar key values, you can end up with a
lot of colliding overflowing buckets right next to each other. The good thing is
it's cache friendly. Since you walk the array directly in memory order, it keeps
the CPU's cache lines full and happy.

Compared to separate chaining, open addressing can be harder to wrap your head
around. You can think of open addressing as similar to separate chaining except
that the list of nodes is threaded through the bucket array itself. Instead of
storing the links between them in pointers, the connections are calculated
implicitly by the order that you look through the buckets to find a place for
the entry.

The tricky part is that more than one of these implicit lists may be interleaved
together. Here's an example:

**TODO: walk through example**

In practice, the interleaving turns out to not be much of a problem. Even in
separate chaining, we need to walk the list to check each entry's key because
multiple keys can reduce to the same bucket. With open addressing, we need to do
that same check, and that also covers the case where you are stepping over
entries that "belong" to a different original bucket.

### Hash functions

We could now build ourselves a reasonably efficient table for storing variable
names up to eight characters long, but that limitation is still annoying. We'll
relax the last constraint and allow arbitrary strings as keys. In order to do
that, we need a way to take a string of any length and convert it to a
fixed-size integer.

Finally, we get to the *hash* part of hash tables. A **hash function** takes
some larger blob data and "hashes" it to produce a fixed-size integer. A <span
name="crypto">good</span> hash function has three main goals:

<aside name="crypto">

Hash functions are also used for cryptography. In that domain, "good" has a
*much* more stringent definition to avoid exposing details about the data being
hashed. We, thankfully, don't need to worry about those concerns for this book.

</aside>

1.  It must be *deterministic*. The same input must always hash to the same
    number. If the same variable name ends up in different buckets at different
    points in time, it's gonna get really hard to find them.

2.  It must be *uniform*. Given a typical set of inputs, it should produce a
    wide and evenly-distributed range of output numbers, with as few clumps or
    patterns as possible. You want it <span name="scatter">scatter</span> values
    across the whole numeric range. That minimizes collisions and clustering,
    both of which can tank a hash table's performance.

3.  It must be *fast*. Every operation on the hash table requires us to hash the
    key first. If hashing is slow, it can potentially cancel out the speed of
    the underlying array storage.

<aside name="scatter">

One of the original names for hash tables was "scatter table" because it takes
the entries and scatters them throughout the array. The word "hash" came from
the idea that a hash function takes the input data, chops it up, and tosses it
all together into a pile to come up with a single number that touches all of
those bits.

</aside>

If there are a sprinkling of probing techniques for open addressing, then there
is a whole storm of hash functions out there. Some are old and optimized for
architectures no one uses any more. Some are designed to be fast, others
cryptographically secure. Some take advantage of vector instructions on specific
chips, others aim to maximize portability.

There are people out there where designing and evaluating hash functions is,
like, their *thing*. I admire them, but I'm not mathematically astute enough to
*be* one. So for clox, I picked a simple, well-work hash function called FNV-1
that's served me fine over the years. Consider <span name="thing">trying</span>
out different ones in your code and see if they make a difference.

<aside name="thing">

Who knows, maybe it could turn out to be your thing too?

</aside>

OK, that's probably about all I can expect you to internalize without seeing
some real code. I wanted to go through the high level concepts first before we
start pushing bits around. But now I think it's time to make things concrete.
Now we're going to build ourselves a hash table that supports string keys, open
addressing, and linear probing. Don't worry if that still seems vague and scary.
It will come into focus shortly.

## Building a Hash Table

The great thing about hash tables compared to other classic techniques like
balanced search trees is that the actual data structure is so simple. Ours goes
into a new module:

^code table-h

A hash table is an array of entries. As in our dynamic array earlier, we keep
track of both the allocated size of the array (`capacity`) and the number of
key/value pairs currently stored in it (`count`). The ratio of count to capacity
is exactly the load factor of the hash table.

Each entry is one of these:

^code entry (1 before, 2 after)

It's a simple key/value pair. Since the key is always a <span
name="string">string</span>, we store the ObjString point directly instead of
wrapping it in a Value. It's a little faster and smaller this way.

<aside name="string">

In clox, we only need to support keys that are strings. Handling other types of
keys doesn't add much complexity. As long as you can reduce an object to a
sequence of bits, and compare two objects for equality, it's easy to use it as a
hash key.

</aside>

The create a new empty hash table, we declare a constructor-like function:

^code init-table-h (2 before, 2 after)

We need a new implementation file to define that. While we're at it, let's get
all of the pesky includes out of the way:

^code table-c

As in our dynamic value array type, a hash table initially starts with zero
capacity and a null array. We don't allocate anything until needed. Assuming we
do eventually allocate something, we need to be able to free it too:

^code free-table-h (1 before, 2 after)

And its glorious implementation:

^code free-table

Again, it looks just like a dynamic array. In fact, you can think of a hash
table as basically a dynamic array with a really unusual policy for inserting
items. We don't need to check for NULL here since `FREE_ARRAY()` alread handles
that gracefully.

### Hashing strings

Before we can start putting entries in the table, we need to, well, hash them.
To ensure that the entries get distributed as widely and uniformly as possible
throughout the array, we want a good hash function that looks at all of the bits
of the key string. If it only, say, looked at the first few characters, then a
series of strings that all shared the same prefix would end up colliding in the
same bucket.

On the other hand, walking the entire string to calculate the hash is kind of
slow. We'd lose some of the performance benefit of the hash table if we had to
walk the string every time we looked for a key in the table. So we'll do the
obvious thing: cache it.

Over in the "object" module, we add:

^code obj-string-hash (1 before, 1 after)

Each StringObj stores the hash code for that string. Since strings are immutable
in Lox, we can calculate once up front and be certain that it will never need to
be recalculated. Doing it then kind of makes sense. Allocating the string and
copying its characters over is already an O(n) operation, so it's a good time to
also do the O(n) calculation of the string's hash.

Whenever we call the internal function to allocate a string, we pass in its
hash code:

^code allocate-string (1 after)

That function simply stores the hash in the struct:

^code allocate-store-hash (1 before, 2 after)

The fun happens over at the callers. `allocateFunction()` is called from two
places: the function that copies a string and the one that takes ownership of
an existing dynamically-allocated string. We'll start with the first:

^code copy-string-hash (1 before, 1 after)

No magic here. We calculate the hash code and then pass it along.

^code copy-string-allocate (2 before, 1 after)

The other string function is similar:

^code take-string-hash (1 before, 1 after)

The actual interesting code is over here:

^code hash-string

This is the actual bonafide "hash function" in clox. Designing a good hash
function is a subtle art. Very well-educated smart people pour months into
coming up with new ones or doing statistical analyses of them. I personally
have *not* sunk that much time into it, so consider any hash function
recommendations from me to be suspect at best.

I'm using [FNV-1a][] here, which has been around forever. My understanding is
that it's not great, but it's not terrible either, and it's the *shortest*
decent hash function I know. Brevity is certainly a virtue in a book that aims
to show you every line of code.

[fnv-1a]: http://www.isthe.com/chongo/tech/comp/fnv/

**todo: aside on name**

The basic idea is pretty simple and many hash functions follow the same pattern.
You start with some initial hash value, usually a constant with certain
carefully chosen mathematical properties. Then you walk the data to be hashed.
For each byte (or sometimes word), you munge the bits into the hash value
somehow and then mix the resulting bits around some.

What it means to "munge" and "mix" can get pretty sophisticated. Ultimately,
though, the basic goal is *unformity* -- we want the resulting hash values to be
as widely scattered around the numeric range as possible to avoid collisions and
clustering.

### Inserting entries

Now that string objects know their hash code, we can start putting them into
hash tables. You can't do anything useful with an empty hash table, so let's
start with inserting:

^code table-set-h (1 before, 2 after)

This adds the given key/value pair to the given hash table. If an entry for that
key is already present, the new value overwrites the old one. Otherwise, it
inserts a new entry. It returns true if a new entry was added.

Here's the implementation:

^code table-set

Most of the interesting logic is in `findEntry()` which we'll get to soon. That
function's job is to take a key and figure out which bucket in the array it
should go in. It returns a pointer to that bucket -- the Entry in the array.

Once we have that, inserting is straightforward. We copy the key and value into
the corresponding fields in the entry. Then we update the hash table's size,
taking care to not increase the count if we overwrote an already-present key.

We're missing a little something here, though. We haven't actually allocated the
entry array yet. Oops! Before we can insert anything, we need to make sure we
have an array, and that it's big enough:

^code table-set-grow (1 before, 1 after)

This is similar to the code we wrote a while back for growing a dynamic array.
If we don't have enough capacity to insert an item, we reallocate and grow the
array. The `GROW_CAPACITY()` macro takes an existing capacity and grows it by
a multiple to ensure that we get amortized constant performance over a series
of inserts.

The interesting difference here is that `TABLE_MAX_LOAD` constant:

^code max-load (2 before, 1 after)

This is how we manage the table's load factor. We don't grow when the capacity
is completely full. Instead, we grow the array before then, when the array
becomes at least <span name="75">75%</span> full.

<aside name="75">

75% is a fairly common load factor. Some hash tables go higher, some lower,
depending on the specific hash function, collision handling strategy and other
constraints.

I didn't do a *ton* of profiling for this specific implementation because it's
hard to define what a "real world" data set even *is* for a toy language like
Lox. Don't put too much stock in this number. When you build your own hash
tables, spend a little time tweaking the load factor and see what it does to
your performance.

**todo: overlap**

</aside>

We'll get to the implementation of `adjustCapacity()` soon. First, let's look
at that `findEntry()` function you've been wondering about:

^code find-entry

This function is the real core of the hash table. It's responsible for taking a
key and an array of buckets and figuring out which bucket the entry belongs in.
We'll use this both to find existing entries in the hash table, and to find a
place to insert new ones. This function is also where linear probing and
collision handling come into play.

For all that, there isn't much to it. First, we take the key's hash code and
map it to the array by taking it modulo the array size. This way we do look past
the end of the array. That gives us an index into the array where, ideally,
we'll be able to find or place the entry.

There are a few cases to check for:

*   If the key in that entry in the array is `NULL`, then the bucket is empty.
    If we're using `findEntry()` to look for something in the hash table, this
    means it isn't there. If we're using it to insert, it means we've found a
    place to add it.

*   If the key in the entry is <span name="equal">equal</span> to the key we're
    looking for, then that key is already present in the table. If we're doing a
    look-up, that's good -- we found what we're looking for. If we're doing an
    insert, this means we'll be replacing the value for that key instead of
    adding a new entry.

<aside name="equal">

It looks like we're using `==` to see if two strings are equal. That doesn't
work, does it? There could be two copies of the same string at different places
in memory. Fear not, astute reader. I have just the solution. And, strangely
enough, it's a hash table that provide the tool we need.

</aside>

*   Otherwise, the bucket has something in it, but it's a different key. This is
    a collision. In that case, we need to start probing.

That's what that for loop does. We start at the bucket where the entry will
ideally go. If that bucket is empty or has the same key, we're done. Otherwise,
we advance to the next element -- this is the *linear* part of "linear probing"
-- and check there.

If we go past the end of the array, we wrap back around to the beginning. That's
what the second modulo operator is for. If we loop around, you might be
wondering about an infinite loop. What if we collide with *every* bucket?
Fortunately, that can't happen thanks to our load factor. Because we grow the
array as soon as it gets close to being full, we know there will always be empty
buckets in the array. Even in pathologically bad cases with lots of collisions,
we'll eventually find an empty bucket.

We exit the loop when we find either an empty bucket or a bucket with the same
key as the one we're looking for. Then we return a pointer to that entry so the
caller can either insert something into it or read from it. Way back in
`tableSet()`, the function that first kicked this off, it stores the new entry
in that bucket and we're done.

### Allocating and resizing

Before we can put anything in the hash table, we do need an actual table. We
need to allocate an array of entries. That happens in this function:

^code table-adjust-capacity

Its job is to allocate grow the hash table's bucket array to `capacity` entries.
After it allocates the array, it initializes every element to be an empty bucket
and then stores the array (and its capacity) in the hash table's main struct.

When we insert the very first entry into the table, we'll end up calling this
with a non-zero capacity and it will do the first allocation of the array. The
code here is fine for that job, but what about when we already have an array and
we need to grow it?

Back when we were doing a dynamic array, we could just use `realloc()` and let
the C standard library copy everything over. That doesn't work for a hash table.
Remember that to choose the bucket for each entry, we take its hash key modulo
the array size. That means that when the array size changes, so do all the
bucket indexes.

**todo: illustrate example**

We need to recalculate the buckets for each of the existing entries in the hash
table. That in turn could cause new collisions which we need to resolve. So the
simplest way to handle that is to rebuild the table from scratch by re-inserting
every entry into the new empty array:

^code re-hash (2 before, 2 after)

We walk through the old array front to back. Any time we find a non-empty
bucket, we insert that entry into the new array. This is why `findEntry()` takes
a pointer to an entry array and not the whole `Table` struct. We can call it
with the new array and capacity before we store those in the struct.

After that's done, we can release the memory for the old array:

^code free-old-array (3 before, 3 after)

With that, we have a hash table that we can stuff as many entries into as we
like. It handles overwriting existing keys and growing itself as needed to
maintain the desired load capacity.

While we're at it, let's also define a helper function for copying all of the
entries of one hash table into another:

^code table-add-all-h (1 before, 2 after)

We'll use this later when we implement method inheritance but we may as well
implement it here while we've got all of the hash table stuff fresh in our
minds.

^code table-add-all

There's not much to say about this. It walks the bucket array of the source hash
table. Whenever it finds a non-empty bucket, it adds the entry to the
destination hash table using the `tableSet()` function we recently defined.

That goes through the full process of finding a bucket for the entry, resolving
collisions, etc. That might seem unnecessarily complicated, but since we don't
know what the destination hash table already contains, there's little we can do
to optimize it. This function isn't on any critical performance path anyway, so
simple is best.

### Retrieving values

Now that our hash table contains some stuff, let's start pulling things back
out. Given a key, we can look up the corresponding value, if there is one, with
this function:

^code table-get-h (1 before, 2 after)

You pass in a table and a key. If it finds an entry with that key, it returns
true, otherwise it returns false. If the entry exists, we store the resulting
value in the `value` output parameter.

Since `findEntries()` already does the hard work, the implementation isn't too
bad:

^code table-get

Obviously, if the table doesn't even have a bucket array, we definitely won't
find the entry, so we check for that first. Otherwise, we let `findEntries()`
work its magic. That returns a pointer to a bucket. If the bucket is empty --
which we detect by seeing if the key is `NULL` -- then it didn't find an entry
with our key.

If it does return a non-empty entry, then that's our match. We take its value
and copy it to the output parameter so the caller can get it. Piece of cake.

### Deleting entries

There is one more fundamental operation a full-featured hash table needs to
support: removing an entry. This seems pretty obvious, if you can add things,
you should be able to *un*-add them, right? But you'd be surprised how many
tutorials and books on hash tables omit this completely.

I could have taken that route too. In fact, we never actually *use* deletion in
clox. But if you want to actually understand how to completely implement a hash
table, this feels important. I can sympathize with their desire to overlook it.
As we'll see, deleting from a hash table that uses <span
name="delete">open</span> addressing is tricky.

<aside name="delete">

With separate chaining, deleting is as simple as deleting from a linked list.

</aside>

The declaration belies its complexity:

^code table-delete-h (1 before, 2 after)

The obvious approach is to mirror insertion. Use `findEntry()` look up the
entry's bucket. Then clear out the bucket. Done!

In cases where there are no collisions, that works fine. But if a collision has
occurred, then the bucket where the entry lives may be part of one or more
implicit probe sequences. Remember that when we're walking a probe sequence to
find an entry, we know we've reached the end of a sequence and the entry isn't
present when we hit an empty bucket. It's like the probe sequence is a list of
entries and an empty entry terminates that list.

**todo: illustrate**

If the entry we're deleting is in the middle of a sequence and we clear it out,
we break that list in the middle, leaving the trailing entries abandoned and
unreachable. Sort of like removing a node from a linked list without relinking
the pointer from the previous node to the next one.

**todo: illustrate**

OK, so maybe we treat it like removing an element from an array. We delete the
entry and then shift up any entries that follow it. Does that work? Sometimes,
yes. But our deleted entry could be in the middle of a couple of interleaved
probe sequences. A bucket after it may actually contain an entry in its
preferred slot. If we move that entry forward, it will shift to *before* where
it's supposed to appear. That's also bad.

**todo: illustrate**

Some of the trailing entries need to move forward and some don't. We could
handle that by deleting the entry and then removing and re-inserting all of the
entries that follow it so that we re-resolve any collisions and put them where
they want to go taking the new empty bucket into account.

That *does* actually work, thank Zeus. But it's pretty slow. So most
implementations use a different trick called "tombstones". It looks like this:

^code table-delete

First, we find the bucket containing the entry we want to delete. (If we don't
find it, there's nothing to delete, so we bail out.) Then, instead of completely
clearing it, we replace the entry with a special sentinel entry called a
"tombstone". In clox, we use a `NULL` key and a `true` value to indicate a
tombstone, but any representation that can't be confused with an empty entry or
a valid one works.

That's all we need to do to delete an entry. Simple and fast. But all of the
other operations need to correctly handle tombstones too. A tombstone is a sort
of zombie entry. It has some of the characteristics of a living present entry,
and some of the characteristics of a dead empty one.

When we are following a probe sequence during a lookup, and we hit a tombstone,
we *don't* treat it like an empty slot and stop iterating. Instead, we note it
and keep going:

^code find-tombstone (2 before, 2 after)

We store the first tombstone found during the probe sequence in a local
variable:

^code find-entry-tombstone (1 before, 1 after)

By iterating through the tombstone, we ensure that deleting an entry doesn't
break any implicit collision chains so that we can still find entries after it.
In that sense, the tombstone entry is still "there".

But if, after going past the tombstone (or tombstones), we don't find the entry
we're looking for, we return the first tombstone's bucket instead of the empty
entry after the end of the probe sequence. If we're calling `findEntry()` in
order to insert a node, that lets us treat the tombstone bucket as empty and
reuse it for the new entry.

Reusing tombstone slots automatically like this helps reduce the number of
tombstones wasting space in the bucket array. In typical use cases where there
is an even mixture of insertions and deletions, the number of tombstones grows
for a while and then hits a point where it tends to stabilize.

Even so, there's no guarantee that a large number of deletes won't cause the
array to be full of tombstones. In the very worst case, we could end up with
*no* empty buckets. That would be bad because, remember the only thing
preventing an infinite loop in `findEntry()` is the assumption that we'll
eventually hit an empty bucket.

So we need to be thoughtful about how tombstones interact with the table's load
factor and resizing. The key question is, when calculating the load factor,
should we treat tombstones like full buckets or empty ones?

If we treat them like full buckets, then we may end up with a bigger array than
we probably need because it artificially inflates the load factor. There are
tombstones we could reuse, but they aren't counted so we end up growing the
array sometimes.

But if we treat tombstones like empty buckets and *don't* include them in the
load factor, then we run the risk of ending up with *no* actual empty buckets to
terminate a lookup. An infinite loop is a much worse problem than a few extra
array slots, so for load factor, we consider tombstones to be full buckets.

That's why we don't reduce the count when deleting an entry in the previous
code. The count is no longer the number of entries in the hash table, it's the
number of entries and tombstones.

When we resize the array, we allocate a new array and re-insert all of the
existing entries into it. During that process, we *don't* copy the tombstones
over. They don't add any value and would just slow down lookups. That means we
need to recalculate the count since it may change during a resize. So we clear
it out:

^code resize-init-count (2 before, 1 after)

Then each time we find a non-tombstone entry, we increment it:

^code resize-increment-count (1 before, 1 after)

I find it interesting that much of the work to support deleting entries was in
`findEntry()` and `adjustCapacity()` to deal with tombstones. The actual delete
logic is quite simple and fast. In practice, deletions tend to be rare, so you'd
expect a hash table to do as much work in the delete function and leave the
other functions alone to keep them faster. With our tombstone approach, deletes
are fast, but lookups get penalized.

I did a little benchmarking to test this out in a few different deletion
scenarios. I was surprised to discover that tombstones did end up being faster
overall compared to doing all the work during deletion to reinsert the affected
entries.

But if you think about it, it's not that the tombstone approach pushes the work
of fully deleting an entry to other operations, it's more that it makes deleting
*lazy*. At first, it does the minimal work to turn the entry into a tombstone.
That can cause a penalty when later lookups have to skip over it. But it also
allows that tombstone bucked to be reused by a later insert too. That reuse is a
very efficient way to avoid the cost of rearranging all of the following
affected entries. You basically recycle a node in the chain of probed entries.
It's a neat trick.

## String Interning

Alright, we've got ourselves a hash table that's mostly working and supports
the operations it needs. We aren't using it yet. But it's also not *totally* working right. Right now, we're gonna solve both of those problems at the same time, and learn another classic tool used by interpreters.

The reason it's not totally working is that when `findEntry()` checks to see if
an existing key matches the one it's looking for, it uses `==` to compare two
strings for equality. That only returns true if the two keys are the exact same
string in memory. Two separate strings with the same characters should be
considered equal, but aren't.

Remember, back when we added strings in the last chapter, we added explicit
support to compare the strings character-by-character in order to get true value
equality. We could do that in `findEntry()`, but that's <span
name="hash-collision">slow</span>.

<aside name="hash-collision">

In practice, we would first check the full hash codes of the two strings for
equality first. That distinguishes almost all non-identical strings -- it
wouldn't be a very good hash function if it didn't. But when the two keys *are*
the same string, we still have to resort to comparing characters in order to be
safe and handle the rare cases where different strings *do* have the same hash.

</aside>

Instead, we'll use a technique called **string interning**. The core problem is
that it's possible to have different strings in memory with the same characters.
Those need to behave like equivalent values even though they are distinct
objects. They're essentially duplicates, and we have to compare all of their
bytes to detect that.

<span name="intern">String interning</span> is de-duplication. We create a
collection of interned strings. Any string in the collection is guaranteed to
distinct from all others. When you "intern" a string, you look for a matching
string in the collection. If found, you use that original one. Otherwise, the
string you have is unique, so you add it to the collection.

<aside name="intern">

Languages vary in how much string interning they do and how it's exposed to the
user. clox, like Lua will intern *all* strings. Lisp, Scheme, Smalltalk, Ruby
and others have a separate string-like type called "symbol" that is implicitly
interned. (This is why, for example, they suggest you use symbols in Ruby for
performance.) Java interns constant strings by default, and provides an API to
let you explicitly intern any string you give it.

</aside>

In this way, you know that each sequence of characters is represented by only
one string in memory. This makes value equality trivial. If two strings points
to the same address in memory, they are obviously the same string and must be
equal. And, because we know strings are unique, if two strings point to
different addresses, they must be distinct strings.

In other words, pointer equality exactly matches value equality. Which in turn
means that our existing `==` in `findEntry()` does the right thing. Or, at
least, it will once we intern all the strings.

In order to reliably de-duplicate all strings, the VM needs to be able to find
every string that's created. We do that by giving it a hash table to store them
all:

^code vm-strings (1 before, 2 after)

As usual, we need an include:

^code vm-include-table (1 before, 1 after)

When we spin up a new VM, the string table is empty:

^code init-strings (1 before, 1 after)

And when we <span name="gc">shut</span> down the VM, we clean up any resources used by the table:

<aside name="gc">

When we implement the garbage collector, we'll handle removing strings from the
string table if they are no longer needed while the program is running.

todo: link

</aside>

^code free-strings (1 before, 1 after)

Some languages have a separate type or an explicit step to intern a string. For
clox, we'll automatically intern every string. That means whenever we create a
new unique string, we add it to the table:

^code allocate-store-string (2 before, 1 after)

We're using the table more like a hash *set* than a hash *table*. The keys are
the strings and those are all we care about, so we just use `nil` for the
values.

This gets a string into the table assuming that it's unique, but we need to
actually check for duplication before we get here. We do that in the higher
level functions that call `allocateString()`. First:

^code copy-string-intern (1 before, 2 after)

When copying a string into a new LoxString, we look it up in the string table
first. If we find it, instead of "copying", we just return a reference to that
string.

Otherwise, we fallthrough, allocate a new string, and store it in the string
table. Taking ownership of a string is a little different:

^code take-string-intern (1 before, 1 after)

Again, we look up the string in the string table first. If we find it, before we
return it, we free the memory for the string that was passed in. Since ownership
is being passed to this function and we no longer need the duplicate string,
it's up to us to free it.

Before we get to the new function we need to write, there's one more include:

^code object-include-table (1 before, 1 after)

To look up a string in the string table, we use this new function:

^code table-find-string-h (1 before, 2 after)

The implementation looks like this:

^code table-find-string

It looks like we copy-pasted `findEntry()`. There is a lot of redundancy, but
there's a couple of key differences. First, we pass in the raw character array
of the key we're looking for instead of an ObjString. At the point that we call this, we haven't created an ObjString yet.

Second, when checking to see if we found the key, we do an actual
character-by-character string comparison. This is the one place in the VM where
we actually test strings for textual equality. We do it here to deduplicate
strings and then the rest of the VM can take for granted that any two strings at
different addresses in memory must have different contents.

In fact, now that we've done that, we can take advantage of it in the bytecode
interpreter. When a user does `==` on two objects that happen to be strings, we
don't need to test the characters any more:

^code equal (1 before, 1 after)

We've added a little overhead when creating strings to intern them. But in
return, at runtime, the equality operator on strings is much faster. With that,
we have a full-featured hash table ready for us to use for tracking variables,
instances, or any other key-value pairs that might show up.

We also sped up testing strings for equality. This is nice for when the user
does `==` on strings. But it's even more critical in a dynamically-typed
language like Lox where method calls and instance fields are looked up by name
at runtime. If testing a string for equality is slow, then that means looking up
a method by name is slow. And if *that's* slow in your object-oriented language,
then *everything* is slow.

<div class="challenges">

## Challenges

1.  In clox, we happen to only need keys that are strings, so the hash table we
    built is hardcoded for that key type. If we exposed hash tables to Lox users
    as a first-class collection type, it would useful to support keys of
    different types.

    Add support for keys of the other primitive types: numbers, Booleans, and
    `nil`. Later, clox will support user-defined classes. If we want to support
    keys that are instances of those classes, what kind of complexity does that
    add?

1.  Hash tables have a lot of knobs you can tweak that affect their performance.
    You decide whether to use separate chaining or open addressing. Depending on
    which fork in that road you take, you can tune how many entries are stored
    in each node, or the probing strategy you use. You control the hash
    function, load factor, and growth rate.

    All of this variety wasn't created just to give CS doctoral candidates
    something to publish theses on: each has its uses in the many varied domains
    and hardware scenarios where hashing comes into play. Look up a few hash
    table implementations in different open source systems and research the
    details of their hash table. Try to figure out why they made the choices
    they made if you can.

1.  Benchmarking a hash table is notoriously difficult. A hash table
    implementation may perform well with some keysets and poorly with others. It
    may work well at small sizes but degrade as it grows, or vice versa. It may
    choke when deletions are common, but fly when they aren't. Creating
    benchmarks that accurately represent how your users will use the hash table
    is a challenge.

    Write a handful of different benchmark programs to validate our hash table
    implementation. How does the performance vary between them? Why did you
    choose the specific test cases you chose?

</div>
