^title Hash Tables
^part A Bytecode Virtual Machine

> Hash, x. There is no definition for this word -- nobody knows what hash is.
>
> <cite>Ambrose Bierce, The Unabridged Devil's Dictionary</cite>

Before we can add variables to our burgeoning virtual machine, we need some way
to look up a value given a variable's name. Later, when we add classes, we'll
also need a way to store fields on instances. The perfect data structure for
these problems and others is a hash table.

You probably already know what a hash table is, even if you don't know it by
that name. If you're a Java programmer, you call them "HashMaps". C# and Python
users call them "dictionaries". In C++, it's an "unordered map". "Objects" in
JavaScript and "tables" in Lua are hash tables under the hood, which is what gives
them their flexibility.

A hash table, whatever your language calls it, associates a set of *keys* with a
set of *values*. Each key/value pair is an *entry* in the table. Given a key,
you can look up its corresponding value. You can add new key/value pairs and
remove entries by key. If you add a new value for an existing key, it replaces
the previous entry.

Hash tables appear in so many languages because they are incredibly powerful.
Much of this power comes from one metric: given a key, a hash table returns the
corresponding value in <span name="constant">constant time</span>, *regardless
of how many keys are in the hash table.*

<aside name="constant">

More specifically, the *average-case* lookup time is constant. Worst-case
performance can be, well, worse. In practice, it's easy to avoid degenerate
behavior and stay on the happy path.

</aside>

That's pretty remarkable when you think about it. Imagine you've got a big stack
of business cards and I ask you to find a certain person. The bigger the pile
is, the longer it will take. Even if the pile is nicely sorted and you've got
the manual dexterity to do a binary search by hand, you're still talking
`O(log(n))`. But with a <span name="rolodex">hash table</span>, it takes the
same time to find that business card when the stack has ten cards as when it has
a million.

<aside name="rolodex">

Stuff all those cards in a Rolodex -- does anyone even remember those things
anymore? -- with dividers for each letter and you improve your speed
dramatically. As we'll see, that's not too far from the trick a hash table uses.

</aside>

## An Array of Buckets

A complete, fast hash table has a couple of moving parts. I'll introduce them
one at a time by working through a couple of toy problems and their solutions.
Eventually, we'll build up to a data structure that can associate any set of
names with their values.

For now, imagine if Lox was a *lot* more restricted in variable names. What if a
variable's name could only be a <span name="basic">single</span> lowercase
letter. How could we very efficiently represent a set of variable names and
their values?

<aside name="basic">

This limitation isn't *too* far-fetched. The initial versions of BASIC out of
Dartmouth only allowed variable names to be a single letter followed by one
optional digit.

</aside>

With only 26 possible variables (27 if you consider underscore a "letter", I
guess), the answer is easy. Declare a fixed-size array with 26 elements. We'll
follow tradition and call each element a "bucket". Each represents a variable
with `a` starting at index zero. If there's a value in the array at some
letter's index, then that key is present with that value. Otherwise, the bucket
is empty and that key/value pair isn't in the data structure.

<aside name="bucket">

<img src="image/hash-tables/bucket-array.png" alt="A row of buckets, each
labeled with a letter of the alphabet." />

</aside>

Memory usage is great -- just a single reasonably-sized <span
name="bucket">array</span>. There's some waste from the empty buckets, but it's
not huge. There's no overhead for node pointers, padding, or other stuff you'd
get with something like a linked list or tree.

Performance is even better. Given a variable name -- its character -- you can
subtract the ASCII value of `a` and use the result to index directly into the
array. Then you can either lookup the existing value or store a new value
directly into that slot. It doesn't get much faster than that.

This is sort of our Platonic ideal data structure. Lightning fast, dead simple,
and compact in memory. As we add support for more complex keys, we'll have to
make some concessions, but this is what we're aiming for. Even once you add in
hash functions, dynamic resizing, and collision resolution, this is still the
core of every hash table out there -- a contiguous array of buckets that you
index directly into.

### Load factor and wrapped keys

Confining Lox to single-letter variables makes our job as implementers easier,
but it's probably no fun programming in a language that only gives you 26
storage locations. What if we loosened it a little and allowed variables up to
<span name="six">eight</span> characters long?

<aside name="six">

Again, this restriction isn't so crazy. Early linkers for C only treated the
first six characters of external identifiers as meaningful. Everything after
that was ignored. If you've ever wondered why the C standard library is so
enamored of abbreviation -- looking at you, `creat()` -- it turns out it wasn't
entirely because of the small screens (or teletypes!) of the day.

</aside>

That's small enough that we can pack all eight characters into a 64-bit integer
and easily turn the string into a number. We can then use it as an array index.
Or, at least, we could if we could somehow allocate a 295,148 *petabyte* array.
Memory's gotten cheaper over time, but not quite *that* cheap. Even if we could
make an array that big, it would be heinously wasteful. Almost every bucket will
be empty unless users start writing way bigger Lox programs than we've
anticipated.

Even though our variable keys cover the full 64-bit numeric range, we clearly don't
need an array that large. Instead, we'll allocate an array with enough
capacity for the entries we need, but still some reasonable size. We map the
full 64-bit keys down to that smaller range by taking the value modulo the size
of the array.

For example, say we want to store "bagel". We allocate an array with eight
elements, plenty enough to store it and more later. We treat the key string as a
64-bit integer. On a little-endian machine like Intel, packing those characters
into a 64-bit word puts the first letter, "b", ASCII value 98 in the
least-significant byte. We take that integer modulo the array size (<span
name="power-of-two">8</span>) to fit it in the bounds and get a bucket index, 2.
Then we store the value there as usual.

<aside name="power-of-two">

I'm using powers of two for the array sizes here, but they don't need to be.
Some styles of hash table work best with powers of two, including the one we'll
build in this book. Others prefer prime number array sizes or have other rules.

</aside>

Using the array size as a modulus lets us map the key's numeric range down to
fit an array of any size. We can thus control the number of buckets
independently of the key range. That solves our waste problem, but introduces a
new one. Any two variables whose key number has the same remainder when divided
by the array size will end up in the same bucket. Keys can **collide**. For
example, if we try to add "jam", it also ends up in bucket 2:

<img src="image/hash-tables/collision.png" alt="'Bagel' and 'jam' both end up in bucket index 2." />

We do have some control of this by tuning the array size. The bigger the array,
the fewer indexes that get mapped to the same bucket and the fewer collisions
that are likely to occur. Hash table implementers track this collision
likelihood by measuring the table's **load factor**. It's defined as the number
of entries divided by the number of buckets. So a hash table with five entries
and an array of 16 elements has a load factor of 0.3125. The higher the load
factor, the greater the chance of collisions.

One way we mitigate collisions is by resizing the array. Just like the dynamic
arrays we implemented earlier, we reallocate and grow the hash table's array as
it fills up. Unlike a regular dynamic array, though, we won't wait until the
array is *full*. Instead, we pick a desired load factor and grow the array when
it goes over that.

## Collision Resolution

Even with a very low load factor, collisions can still occur. The [*birthday
paradox*][birthday] tells us that as the number of entries in the hash table
increases, the chance of collision increases very quickly. We can pick a large
array size to reduce that, but it's a losing game. Say we wanted to store a
hundred items in a hash table. To keep the chance of collision below a
still-pretty-high 10%, we need an array with at least 47,015 elements. To get
the chance below 1% requires an array with 492,555 elements, over 4,000 thousand
empty buckets for each one in use.

[birthday]: https://en.wikipedia.org/wiki/Birthday_problem

A low load factor can make collisions <span name="pigeon">rarer</span>, but the
[*pigeonhole principle*][pigeon] tells us we can never eliminate them entirely.
If you've got five pet pigeons and four holes to put them in, at least one hole
is going to end up with more than one pigeon. With 18,446,744,073,709,551,616
different variable names, any reasonably-sized array can potentially end up with
multiple keys in the same bucket.

[pigeon]: https://en.wikipedia.org/wiki/Pigeonhole_principle

Thus we still have to handle collisions gracefully when they occur. Users don't
like it when their programming language can look up variables correctly only
*most* of the time.

<aside name="pigeon">

Put these two funny-named mathematical rules together and you get this
observation: Take a birdhouse containing 365 pigeonholes, and use each pigeon's
birthday to assign it to a pigeonhole. You'll need only about 26 randomly-chosen
pigeons before you get a greater than 50% chance of two pigeons in the same box.

<img src="image/hash-tables/pigeons.png" alt="Two pigeons in the same hole." />

</aside>

### Separate chaining

Techniques for resolving collisions fall into two broad categories. The first is
**separate chaining**. Instead of each bucket containing a single entry, we let
it contain a collection of them. In the classic implementation, each bucket
points to a linked list of entries. To look up an entry, you find its bucket and
then walk the list until you find an entry with the matching key.

<img src="image/hash-tables/chaining.png" alt="An array with eight buckets. Bucket 2 links to a chain of two nodes. Bucket 5 links to a single node." />

In catastrophically bad cases where every entry collides in the same bucket, the
data structure degrades into a single unsorted linked list with `O(n)` lookup.
In practice, it's easy to avoid that by controlling the load factor and how
entries get scattered across buckets. In typical separate chained hash tables,
it's rare for a bucket to have more than one or two entries.

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

The other technique is <span name="open">called</span> **"open addressing"** or
(confusingly) **"closed hashing"**. With this technique, all entries live
directly in the bucket array, with one entry per bucket. If two entries collide
in the same bucket, we find a different empty bucket to use instead.

<aside name="open">

It's called "open" addressing because the entry may end up at an address
(bucket) outside of its preferred one. It's called "closed" hashing because all
of the entries stay inside the array of buckets.

</aside>

Storing all entries in a single big contiguous array is great for keeping the
memory representation simple and fast. But it makes all of the operations on the
hash table more complex. When inserting an entry, its bucket may be full,
sending us to look at another bucket. That bucket itself may be occupied and so
on. This process of finding an available bucket is called **probing** and the
order that you examine buckets is a **probe sequence**.

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
very next element in the array, and so on. If we reach the end, we wrap back
around to the beginning.

The good thing about linear probing is that it's cache friendly. Since you walk
the array directly in memory order, it keeps the CPU's cache lines full and
happy. The bad thing is that it's prone to **clustering**. If you have a lot of
entries with numerically similar key values, you can end up with a lot of
colliding overflowing buckets right next to each other.

Compared to separate chaining, open addressing can be harder to wrap your head
around. I think of open addressing as similar to separate chaining except that
the "list" of nodes is threaded through the bucket array itself. Instead of
storing the links between them in pointers, the connections are calculated
implicitly by the order that you look through the buckets.

The tricky part is that more than one of these implicit lists may be interleaved
together. Let's walk through an example that covers all the interesting cases.
We'll ignore values for now and just worry about a set of keys. We start with an
empty array of 8 buckets:

<img src="image/hash-tables/insert-1.png" alt="An array with eight empty buckets." class="wide" />

We decide to insert "bagel". The first letter, "b" (ASCII value 98), modulo the
array size (8) puts it in bucket 2:

<img src="image/hash-tables/insert-2.png" alt="Bagel goes into bucket 2." class="wide" />

Next, we insert "jam". That also wants to go in bucket 2 (106 mod 8 = 2), but
that bucket's taken. We keeping probing to the next bucket. It's empty, so we
put it there:

<img src="image/hash-tables/insert-3.png" alt="Jam goes into bucket 3, since 2 is full." class="wide" />

We insert "fruit", which happily lands in bucket 6:

<img src="image/hash-tables/insert-4.png" alt="Fruit goes into bucket 6." class="wide" />

Likewise, "migas" can go in its preferred bucket 5:

<img src="image/hash-tables/insert-5.png" alt="Migas goes into bucket 5." class="wide" />

When we try to insert "eggs", it also wants to be in bucket 5. That's full so we
skip to 6. Bucket 6 is also full. Note that the entry in there is *not* part of
the same probe sequence. "Fruit" is in its preferred bucket, 6. So the 5 and 6
sequences have collided and are interleaved. We skip over that and finally put
"eggs" in bucket 7:

<img src="image/hash-tables/insert-6.png" alt="Eggs goes into bucket 7 because 5 and 6 are full." class="wide" />

We run into a similar problem with "nuts". It can't land in 6 like it wants to.
Nor can it go into 7. So we keep going. But we've reached the end of the array,
so we wrap back around to 0 and put it there:

<img src="image/hash-tables/insert-7.png" alt="Nuts wraps around to bucket 0 because 6 and 7 are full." class="wide" />

In practice, the interleaving turns out to not be much of a problem. Even in
separate chaining, we need to walk the list to check each entry's key because
multiple keys can reduce to the same bucket. With open addressing, we need to do
that same check, and that also covers the case where you are stepping over
entries that "belong" to a different original bucket.

## Hash Functions

We can now build ourselves a reasonably efficient table for storing variable
names up to eight characters long, but that limitation is still annoying. In
order to relax the last constraint, we need a way to take a string of any length
and convert it to a fixed-size integer.

Finally, we get to the "hash" part of "hash table". A **hash function** takes
some larger blob of data and "hashes" it to produce a fixed-size integer whose
value depends on all of the bits of the original data. A <span
name="crypto">good</span> hash function has three main goals:

<aside name="crypto">

Hash functions are also used for cryptography. In that domain, "good" has a
*much* more stringent definition to avoid exposing details about the data being
hashed. We, thankfully, don't need to worry about those concerns for this book.

</aside>

1.  It must be *deterministic*. The same input must always hash to the same
    number. If the same variable ends up in different buckets at different
    points in time, it's gonna get really hard to find it.

2.  It must be *uniform*. Given a typical set of inputs, it should produce a
    wide and evenly-distributed range of output numbers, with as few clumps or
    patterns as possible. We want it to <span name="scatter">scatter</span>
    values across the whole numeric range to minimize collisions and clustering.

3.  It must be *fast*. Every operation on the hash table requires us to hash the
    key first. If hashing is slow, it can potentially cancel out the speed of
    the underlying array storage.

<aside name="scatter">

One of the original names for hash tables was "scatter table" because it takes
the entries and scatters them throughout the array. The word "hash" came from
the idea that a hash function takes the input data, chops it up, and tosses it
all together into a pile to come up with a single number from all of those bits.

</aside>

There is a veritable pile of hash functions out there. Some are old and
optimized for architectures no one uses anymore. Some are designed to be fast,
others cryptographically secure. Some take advantage of vector instructions and
cache sizes for specific chips, others aim to maximize portability.

There are people out there for whom designing and evaluating hash functions is,
like, their *jam*. I admire them, but I'm not mathematically astute enough to
*be* one. So for clox, I picked a simple, well-worn hash function called
[FNV-1a][] that's served me fine over the years. Consider <span
name="thing">trying</span> out different ones in your code and see if they make
a difference.

[fnv-1a]: http://www.isthe.com/chongo/tech/comp/fnv/

<aside name="thing">

Who knows, maybe hash functions could turn out to be your thing too?

</aside>

OK, that's a quick run through of buckets, load factors, open addressing,
collision resolution and hash functions. That's an awful lot of text and not a
lot of real code. Don't worry if it still seems vague. Once we're done coding it
up, it will all click into place.

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
name="string">string</span>, we store the ObjString pointer directly instead of
wrapping it in a Value. It's a little faster and smaller this way.

<aside name="string">

In clox, we only need to support keys that are strings. Handling other types of
keys doesn't add much complexity. As long as you can reduce an object to a
sequence of bits, and compare two objects for equality, it's easy to use as a
hash key.

</aside>

To create a new empty hash table, we declare a constructor-like function:

^code init-table-h (2 before, 2 after)

We need a new implementation file to define that. While we're at it, let's get
all of the pesky includes out of the way:

^code table-c

As in our dynamic value array type, a hash table initially starts with zero
capacity and a `NULL` array. We don't allocate anything until needed. Assuming
we do eventually allocate something, we need to be able to free it too:

^code free-table-h (1 before, 2 after)

And its glorious implementation:

^code free-table

Again, it looks just like a dynamic array. In fact, you can think of a hash
table as basically a dynamic array with a really strange policy for inserting
items. We don't need to check for `NULL` here since `FREE_ARRAY()` already
handles that gracefully.

### Hashing strings

Before we can start putting entries in the table, we need to, well, hash them.
To ensure that the entries get distributed as uniformly throughout the array, we
want a good hash function that looks at all of the bits of the key string. If it
only, say, looked at the first few characters, then a series of strings that all
shared the same prefix would end up colliding in the same bucket.

On the other hand, walking the entire string to calculate the hash is kind of
slow. We'd lose some of the performance benefit of the hash table if we had to
walk the string every time we looked for a key in the table. So we'll do the
obvious thing: cache it.

Over in the "object" module in ObjString, we add:

^code obj-string-hash (1 before, 1 after)

Each ObjString stores the hash code for that string. Since strings are immutable
in Lox, we can calculate it once up front and be certain that it will never get
invalidated. Caching it eagerly makes a kind of sense: Allocating the string and
copying its characters over is already an `O(n)` operation, so it's a good time
to also do the `O(n)` calculation of the string's hash.

Whenever we call the internal function to allocate a string, we pass in its
hash code:

^code allocate-string (1 after)

That function simply stores the hash in the struct:

^code allocate-store-hash (1 before, 2 after)

The fun happens over at the callers. `allocateString()` is called from two
places: the function that copies a string and the one that takes ownership of an
existing dynamically-allocated string. We'll start with the first:

^code copy-string-hash (1 before, 1 after)

No magic here. We calculate the hash code and then pass it along:

^code copy-string-allocate (2 before, 1 after)

The other string function is similar:

^code take-string-hash (1 before, 1 after)

The interesting code is over here:

^code hash-string

This is the actual bonafide "hash function" in clox. The algorithm is called
"FNV-1a", and is the shortest decent hash function I know. Brevity is certainly
a virtue in a book that aims to show you every line of code.

The basic idea is pretty simple and many hash functions follow the same pattern.
You start with some initial hash value, usually a constant with certain
carefully chosen mathematical properties. Then you walk the data to be hashed.
For each byte (or sometimes word), you munge the bits into the hash value
somehow and then mix the resulting bits around some.

What it means to "munge" and "mix" can get pretty sophisticated. Ultimately,
though, the basic goal is *uniformity* -- we want the resulting hash values to
be as widely scattered around the numeric range as possible to avoid collisions
and clustering.

### Inserting entries

Now that string objects know their hash code, we can start putting them into
hash tables:

^code table-set-h (1 before, 2 after)

This adds the given key/value pair to the given hash table. If an entry for that
key is already present, the new value overwrites the old one. It returns `true`
if a new entry was added. Here's the implementation:

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

This is how we manage the table's <span name="75">load</span> factor. We don't
grow when the capacity is completely full. Instead, we grow the array before
then, when the array becomes at least 75% full.

<aside name="75">

Ideal max load factor varies based on the hash function, collision-handling
strategy, and typical keysets you'll see. Since a toy language like Lox doesn't
have "real world" data sets, it's hard to optimize this, and I picked 75%
somewhat arbitrarily. When you build your own hash tables, benchmark and tune
this.

</aside>

We'll get to the implementation of `adjustCapacity()` soon. First, let's look
at that `findEntry()` function you've been wondering about:

^code find-entry

This function is the real core of the hash table. It's responsible for taking a
key and an array of buckets and figuring out which bucket the entry belongs in.
This function is also where linear probing and collision handling come into
play. We'll use it both to find existing entries in the hash table, and to
decide where to insert new ones.

For all that, there isn't much to it. First, we take the key's hash code and map
it to the array by taking it modulo the array size. That gives us an index into
the array where, ideally, we'll be able to find or place the entry.

There are a few cases to check for:

*   If the key for that entry in the array is `NULL`, then the bucket is empty.
    If we're using `findEntry()` to look up something in the hash table, this
    means it isn't there. If we're using it to insert, it means we've found a
    place to add it.

*   If the key in the bucket is <span name="equal">equal</span> to the key we're
    looking for, then that key is already present in the table. If we're doing a
    look-up, that's good -- we found the key we seek. If we're doing an insert,
    this means we'll be replacing the value for that key instead of adding a new
    entry.

<aside name="equal">

It looks like we're using `==` to see if two strings are equal. That doesn't
work, does it? There could be two copies of the same string at different places
in memory. Fear not, astute reader. We'll solve this further on. And, strangely
enough, it's a hash table that provides the tool we need.

</aside>

*   Otherwise, the bucket has an entry in it, but with a different key. This is
    a collision. In that case, we start probing. That's what that for loop does.
    We start at the bucket where the entry would ideally go. If that bucket is
    empty or has the same key, we're done. Otherwise, we advance to the next
    element -- this is the *linear* part of "linear probing" -- and check there.
    If we go past the end of the array, that second modulo operator wraps us
    back around to the beginning.

We exit the loop when we find either an empty bucket or a bucket with the same
key as the one we're looking for. You might be wondering about an infinite loop.
What if we collide with *every* bucket? Fortunately, that can't happen thanks to
our load factor. Because we grow the array as soon as it gets close to being
full, we know there will always be empty buckets.

Once we exit the loop, we return a pointer to the found entry so the caller can
either insert something into it or read from it. Way back in `tableSet()`, the
function that first kicked this off, it stores the new entry in that bucket and
we're done.

### Allocating and resizing

Before we can put entries in the hash table, we do need a place to actually
store them. We need to allocate an array of buckets. That happens in this
function:

^code table-adjust-capacity

It creates a bucket array with `capacity` entries. After it allocates the array,
it initializes every element to be an empty bucket and then stores the array
(and its capacity) in the hash table's main struct. This code is fine for when
we insert the very first entry into the table, and we require the first
allocation of the array. But what about when we already one and we need to grow
it?

Back when we were doing a dynamic array, we could just use `realloc()` and let
the C standard library copy everything over. That doesn't work for a hash table.
Remember that to choose the bucket for each entry, we take its hash key *modulo
the array size*. That means that when the array size changes, entries may end up
in different buckets.

We need to recalculate the buckets for each of the existing entries in the hash
table. That in turn could cause new collisions which we need to resolve. So the
simplest way to handle that is to rebuild the table from scratch by re-inserting
every entry into the new empty array:

^code re-hash (2 before, 2 after)

We walk through the old array front to back. Any time we find a non-empty
bucket, we insert that entry into the new array. We use `findEntry()`, passing
in the *new* array instead of the one currently stored in the Table. (This is
why `findEntry()` takes a pointer directly to an entry array and not the whole
`Table` struct. That way, we can pass the new array and capacity before we've
stored those in the struct.)

After that's done, we can release the memory for the old array:

^code free-old-array (3 before, 3 after)

With that, we have a hash table that we can stuff as many entries into as we
like. It handles overwriting existing keys and growing itself as needed to
maintain the desired load capacity.

While we're at it, let's also define a helper function for copying all of the
entries of one hash table into another:

^code table-add-all-h (1 before, 2 after)

We won't need this until much later when we support method inheritance, but we
may as well implement it now while we've got all the hash table stuff fresh in
our minds.

^code table-add-all

There's not much to say about this. It walks the bucket array of the source hash
table. Whenever it finds a non-empty bucket, it adds the entry to the
destination hash table using the `tableSet()` function we recently defined.

### Retrieving values

Now that our hash table contains some stuff, let's start pulling things back
out. Given a key, we can look up the corresponding value, if there is one, with
this function:

^code table-get-h (1 before, 1 after)

You pass in a table and a key. If it finds an entry with that key, it returns
`true`, otherwise it returns `false`. If the entry exists, it stores the
resulting value in the `value` output parameter.

Since `findEntry()` already does the hard work, the implementation isn't too
bad:

^code table-get

Obviously, if the table doesn't even have a bucket array, we definitely won't
find the entry, so we check for that first. Otherwise, we let `findEntry()`
work its magic. That returns a pointer to a bucket. If the bucket is empty --
which we detect by seeing if the key is `NULL` -- then it didn't find an entry
with our key. If it does return a non-empty entry, then that's our match. We
take its value and copy it to the output parameter so the caller can get it.
Piece of cake.

### Deleting entries

There is one more fundamental operation a full-featured hash table needs to
support: removing an entry. This seems pretty obvious, if you can add things,
you should be able to *un*-add them, right? But you'd be surprised how many
tutorials on hash tables omit this.

I could have taken that route too. In fact, we never actually *use* deletion in
clox. But if you want to actually understand how to completely implement a hash
table, this feels important. I can sympathize with their desire to overlook it.
As we'll see, deleting from a hash table that uses <span
name="delete">open</span> addressing is tricky.

<aside name="delete">

With separate chaining, deleting is as simple as remove a node from a linked
list.

</aside>

At least the declaration is simple:

^code table-delete-h (1 before, 2 after)

The obvious approach is to mirror insertion. Use `findEntry()` to look up the
entry's bucket. Then clear out the bucket. Done!

In cases where there are no collisions, that works fine. But if a collision has
occurred, then the bucket where the entry lives may be part of one or more
implicit probe sequences. For example, here's a hash table containing three keys
all with the same preferred bucket, 2:

<img src="image/hash-tables/delete-1.png" alt="A hash table containing 'bagel' in bucket 2, 'biscuit' in bucket 3, and 'jam' in bucket 4." />

Remember that when we're walking a probe sequence to find an entry, we know
we've reached the end of a sequence and the entry isn't present when we hit an
empty bucket. It's like the probe sequence is a list of entries and an empty
entry terminates that list.

If we delete "biscuit" by simply clearing the entry, then we break that probe
sequence in the middle, leaving the trailing entries orphaned and unreachable.
Sort of like removing a node from a linked list without relinking the pointer
from the previous node to the next one.

If we later try to look for "jam", we'll start at "bagel", stop at the next
empty entry, and never find it:

<img src="image/hash-tables/delete-2.png" alt="The 'biscuit' entry has been deleted from the hash table, breaking the chain." />

To solve this, most implementations use a trick called <span
name="tombstone">**"tombstones"**</span>. Instead of clearing the entry on
deletion, we replace it with a special sentinel entry called a "tombstone". When
we are following a probe sequence during a lookup, and we hit a tombstone, we
*don't* treat it like an empty slot and stop iterating. Instead, we keep going
so that deleting an entry doesn't break any implicit collision chains and we can
still find entries after it:

<img src="image/hash-tables/delete-3.png" alt="Instead of deleting 'biscuit', it's replaced with a tombstone." />

The code looks like this:

^code table-delete

First, we find the bucket containing the entry we want to delete. (If we don't
find it, there's nothing to delete, so we bail out.) We replace the entry with a
tombstone. In clox, we use a `NULL` key and a `true` value to represent that,
but any representation that can't be confused with an empty bucket or a valid
entry works.

<aside name="tombstone">

<img src="image/hash-tables/tombstone.png" alt="A tombstone enscribed 'Here lies entry bagel -> 3.75, gone but not deleted." />

</aside>

That's all we need to do to delete an entry. Simple and fast. But all of the
other operations need to correctly handle tombstones too. A tombstone is a sort
of "half" entry. It has some of the characteristics of a present entry, and some
of the characteristics of an empty one.

When we are following a probe sequence during a lookup, and we hit a tombstone,
we *don't* treat it like an empty slot and stop iterating. Instead, we note it
and keep going:

^code find-tombstone (2 before, 2 after)

The first time we pass a tombstone, we store it in this local variable:

^code find-entry-tombstone (1 before, 1 after)

If we reach a truly empty entry then then the key isn't present. In that case,
if we have passed a tombstone, we return its bucket instead of the later empty
one. If we're calling `findEntry()` in order to insert a node, that lets us
treat the tombstone bucket as empty and reuse it for the new entry.

Reusing tombstone slots automatically like this helps reduce the number of
tombstones wasting space in the bucket array. In typical use cases where there
is a mixture of insertions and deletions, the number of tombstones grows for a
while and then tends to stabilize.

Even so, there's no guarantee that a large number of deletes won't cause the
array to be full of tombstones. In the very worst case, we could end up with
*no* empty buckets. That would be bad because, remember, the only thing
preventing an infinite loop in `findEntry()` is the assumption that we'll
eventually hit an empty bucket.

So we need to be thoughtful about how tombstones interact with the table's load
factor and resizing. The key question is, when calculating the load factor,
should we treat tombstones like full buckets or empty ones?

If we treat them like full buckets, then we may end up with a bigger array than
we probably need because it artificially inflates the load factor. There are
tombstones we could reuse, but they aren't treated as unused so we end up
growing the array prematurely.

But if we treat tombstones like empty buckets and *don't* include them in the
load factor, then we run the risk of ending up with *no* actual empty buckets to
terminate a lookup. An infinite loop is a much worse problem than a few extra
array slots, so for load factor, we consider tombstones to be full buckets.

That's why we don't reduce the count when deleting an entry in the previous
code. The count is no longer the number of entries in the hash table, it's the
number of entries plus tombstones.

When we resize the array, we allocate a new array and re-insert all of the
existing entries into it. During that process, we *don't* copy the tombstones
over. They don't add any value since we're rebuilding the probe sequences
anyway, and would just slow down lookups. That means we need to recalculate the
count since it may change during a resize. So we clear it out:

^code resize-init-count (2 before, 1 after)

Then each time we find a non-tombstone entry, we increment it:

^code resize-increment-count (1 before, 1 after)

This means that when we grow the capacity, we may end up with *fewer* entries in
the resulting larger array because all of the tombstones get discarded. That's a
little wasteful, but not a huge practical problem.

I find it interesting that much of the work to support deleting entries was in
`findEntry()` and `adjustCapacity()`. The actual delete logic is quite simple
and fast. In practice, deletions tend to be rare, so you'd expect a hash table
to do as much work as it can in the delete function and leave the other
functions alone to keep them faster. With our tombstone approach, deletes are
fast, but lookups get penalized.

I did a little benchmarking to test this out in a few different deletion
scenarios. I was surprised to discover that tombstones did end up being faster
overall compared to doing all the work during deletion to reinsert the affected
entries.

But if you think about it, it's not that the tombstone approach pushes the work
of fully deleting an entry to other operations, it's more that it makes deleting
*lazy*. At first, it does the minimal work to turn the entry into a tombstone.
That can cause a penalty when later lookups have to skip over it. But it also
allows that tombstone bucket to be reused by a later insert too. That reuse is a
very efficient way to avoid the cost of rearranging all of the following
affected entries. You basically recycle a node in the chain of probed entries.
It's a neat trick.

## String Interning

We've got ourselves a hash table that mostly works, though it has a critical
flaw in its center. Also, we aren't using it for anything yet. It's time to
address both of those and in the process learn a classic technique used by
interpreters.

The reason the hash table doesn't totally work is that when `findEntry()` checks
to see if an existing key matches the one it's looking for, it uses `==` to
compare two strings for equality. That only returns true if the two keys are the
exact same string in memory. Two separate strings with the same characters
should be considered equal, but aren't.

Remember, back when we added strings in the last chapter, we added [explicit
support to compare the strings character-by-character][equals] in order to get
true value equality. We could do that in `findEntry()`, but that's <span
name="hash-collision">slow</span>.

[equals]: strings.html#operations-on-strings

<aside name="hash-collision">

In practice, we would first compare the hash codes of the two strings. That
quickly detects almost all different strings -- it wouldn't be a very good hash
function if it didn't. But when the two hashes are the same, we still have to
compare characters to make sure we didn't have a hash collision on different
strings.

</aside>

Instead, we'll use a technique called **"string interning"**. The core problem
is that it's possible to have different strings in memory with the same
characters. Those need to behave like equivalent values even though they are
distinct objects. They're essentially duplicates, and we have to compare all of
their bytes to detect that.

<span name="intern">String interning</span> is a process of deduplication. We
create a collection of "interned" strings. Any string in that collection is
guaranteed to be textually distinct from all others. When you intern a string,
you look for a matching string in the collection. If found, you use that
original one. Otherwise, the string you have is unique, so you add it to the
collection.

<aside name="intern">

I'm guessing "intern" is short for "internal". I think the idea is that the
language's runtime keeps its own "internal" collection of these strings, whereas
other strings could be user created and floating around in memory. When you
intern a string, you ask it to add the string to that internal collection and
return a pointer to it.

Languages vary in how much string interning they do and how it's exposed to the
user. Lua interns *all* strings, which is what clox will do too. Lisp, Scheme,
Smalltalk, Ruby and others have a separate string-like type called "symbol" that
is implicitly interned. (This is why they say symbols are "faster" in Ruby.)
Java interns constant strings by default, and provides an API to let you
explicitly intern any string you give it.

</aside>

In this way, you know that each sequence of characters is represented by only
one string in memory. This makes value equality trivial. If two strings point
to the same address in memory, they are obviously the same string and must be
equal. And, because we know strings are unique, if two strings point to
different addresses, they must be distinct strings.

Thus, pointer equality exactly matches value equality. Which in turn means that
our existing `==` in `findEntry()` does the right thing. Or, at least, it will
once we intern all the strings. In order to reliably deduplicate all strings,
the VM needs to be able to find every string that's created. We do that by
giving it a hash table to store them all:

^code vm-strings (1 before, 2 after)

As usual, we need an include:

^code vm-include-table (1 before, 1 after)

When we spin up a new VM, the string table is empty:

^code init-strings (1 before, 1 after)

And when we shut down the VM, we clean up any resources used by the table:

^code free-strings (1 before, 1 after)

Some languages have a separate type or an explicit step to intern a string. For
clox, we'll automatically intern every one. That means whenever we create a new
unique string, we add it to the table:

^code allocate-store-string (2 before, 1 after)

We're using the table more like a hash *set* than a hash *table*. The keys are
the strings and those are all we care about, so we just use `nil` for the
values.

This gets a string into the table assuming that it's unique, but we need to
actually check for duplication before we get here. We do that in two of the
higher-level functions that call `allocateString()`. Here's one:

^code copy-string-intern (1 before, 2 after)

When copying a string into a new LoxString, we look it up in the string table
first. If we find it, instead of "copying", we just return a reference to that
string. Otherwise, we fall through, allocate a new string, and store it in the
string table.

Taking ownership of a string is a little different:

^code take-string-intern (1 before, 1 after)

Again, we look up the string in the string table first. If we find it, before we
return it, we free the memory for the string that was passed in. Since ownership
is being passed to this function and we no longer need the duplicate string,
it's up to us to free it.

Before we get to the new function we need to write, there's one more include:

^code object-include-table (1 before, 1 after)

To look for a string in the table, we can't use the normal `tableGet()` function
because that calls `findEntry()`, which has the exact problem with duplicate
strings that we're trying to fix right now. Instead, we use this new function:

^code table-find-string-h (1 before, 2 after)

The implementation looks like this:

^code table-find-string

It looks like we copy-pasted `findEntry()`. There is a lot of redundancy, but
there's a couple of key differences. First, we pass in the raw character array
of the key we're looking for instead of an ObjString. At the point that we call
this, we haven't created an ObjString yet.

Second, when checking to see if we found the key, we do an actual
character-by-character string comparison. This is the one place in the VM where
we actually test strings for textual equality. We do it here to deduplicate
strings and then the rest of the VM can take for granted that any two strings at
different addresses in memory must have different contents.

In fact, now that we've interned all the strings, we can take advantage of it in
the bytecode interpreter. When a user does `==` on two objects that happen to be
strings, we don't need to test the characters any more:

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
    as a first-class collection, it would useful to support different kinds of
    keys.

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
    something to <span name="publish">publish</span> theses on: each has its
    uses in the many varied domains and hardware scenarios where hashing comes
    into play. Look up a few hash table implementations in different open source
    systems, research the choices they made, and try to figure out why they did
    things that way.

    <aside name="publish">

    Well, at least that wasn't the *only* reason they were created. Whether that
    was the *main* reason is up for debate.

    </aside>

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
