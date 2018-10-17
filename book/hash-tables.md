^title Hash Tables
^part A Bytecode Virtual Machine

- really excited
- get to dig deep on one of most-used but least-understood data structures
  in programming
- no matter what language use, probably use hash tables every day
- "map", "dictionary", "associative array"

- might know roughly how they work, stuff around "hashing" the key
- in this chapter, cover every inch of impl from scratch
- also learn that hash tables have huge amount of variation

- aside from how cool they are, vital for clox
- adding global variables soon, and vm stores them in hash table
- later when add instances of classes, use hash to store fields

## addressing people

- probably already know what they are
- let you associate set of keys with set of values
- given a key, can find its value
- can add new key/value pairs and remove
- most important, can find key really fast
- in fact, constant time, regardless of how many keys in there
- remarkable when think about it

- imagine have big pile of cards
- i ask you to go find one
- bigger the pile, the longer it will take
- even if the pile is sorted
- [log(n) for binary search, interval search]

- but with hash table, still takes as much time to find things, regardless of
  how many keys

- explain how with analogy
- imagine work at post office
- want to verify mail sent to right person
- mail has street address and name on it
- you know who lives at each address
- [assume one person]
- to simplify, only worry about one street
- so, given address number on street, look up name of person
- what's fastest way?
- can take advantage of two things
  - no two people have same number
  - numbers are in relatively small range
- simply allocate array with as many elements as highest number
- in each element (call "bucket"), store name of person with that street number
- if number doesn't have person, leave bucket empty

- now, given number, can find person in constant time
- just one direct array access

## hashing

- works for our trivial example where our keys are unique small integers in
  narrow range
- real world not so nice
- for global variables, want to make strings (var names) to values
- strings definitely not small integers in narrow range

- to use direct array access trick, need to somehow convert string to integer
- do that with hash function
- takes some piece of data -- string here -- and produces a number
- consistent: same data always hashes to same value
- [wouldn't do if street address for person spontaneously changed]

- so, given string key, calc hash
- then use that as index into array
- value is in array
- right back to constant time
- [time may depend on length of string, but can cache hash
  doesn't depend on number of keys in table]

**todo: illustrate**

- problem
- how do we ensure hash fn generates unique int for strings?
- turns out not possible because of pigeonhole principle

**todo: illustrate**

- states that if have more pigeons than boxes, then some box must have more
  than one pigeon

- in clox, hash to unsigned 32 bit int
- 4294967296 different values (2^32)
- definitely more than that many possible strings
- even if limit to strings of ten-letter uppercase letters, 141167095653376 different possibilities
- thus many strings must share same hash

- good hash fn makes collisions rare
- in practice means that, given likely keys, produces numbers widely distributed
- in range
- "hashing" takes set of values and scatters them around numeric space like
  when chopping potato into hash browns and scattering on grill

**todo: illustrate**

- [original also called "scatter tables"]

- even so, if don't know set of keys up front, always possible to end up with
  pair of keys with same hash
- [perfect hashing]

### load factor and resizing

- if assume hash fn generates numbers uniformly distributed across range, odds
  of collision goes up as number of full buckets increases
- with lots of empty buckets, likely to hit empty one
- as fill up, collision more likely
- once all full, obviously inevitable

- ratio of full buckets to total buckets "load factor"
- lower load factor makes collisions rarer but uses more memory
- [not as good for cpu cache either]
- collisions generally degrade perf, so balancing act

- balance by dynamically resizing array
- when load factor gets too high, grow array to add more empty buckets
  and lower load factor
- get into how to do that later

## collisions

- even with low load factor, collisions still possible
- even if rare, not good enough to blow up if collision happens
  - [bloom filter]

- entry
  - first thing, since multiple key/val pairs may collide in same bucket,
    can't assume that if bucket has value, that it matches our key
  - key could be different one with same hash
  - so when looking up, need to check actual key too
  - thus bucket doesn't just store value, stores key/value pair
  - "entry"

- many techniques, two broad categories
- separate chaining
  - each bucket can contain collection of entries
  - simplest is bucket is pointer to head of linked list of entries
  - to look up, find bucket O(1), then walk list O(n) in size of list
  - in worst case, if all keys have same hash, becomes O(n)
  - in practice, good hash fn makes that very rare and most buckets have on
    average one entry or empty
  - low load factor helps
  - conceptually simple
  - makes deletion easy too
  - but wasted lot of memory - lot of little linked list cells
  - [many impls opt by storing first entry right in bucket, and storing
    multiple entries per linked list node]
  - lots of pointer chasing, not cache friendly
  - don't see used often in practice

- open addressing
  - store everything in single array of buckets, with one entry per bucket
  - to handle collision, multiple entries with same key may overflow into
    different buckets
  - more algorithmically complex to find and manip entries
  - but simpler layout in memory -- one big contiguous array

  - process to look up becomes:
    - hash key
    - look in bucket
    - if entry there is different key, look in other bucket
    - repeat until find bucket with key, or determine no bucket has it
  - "open addressing" - address of entry is "open" in that may not directly
    match hash
  - process is called "probing"
  - different rules for what sequence of buckets to search, what to do when
    find it, how to tell if not found
  - [links to quadratic, robin-hood, double hashing, cuckoo, etc.]
  - lot of research because so key to perf of many programs

- linear probing
  - simplest technique is look in sequential buckets
  - if not in first, look next array element, and so on
  - wrap around if needed
  - very cache friendly

  - good hash fn even more important
  - need to produce not just different values, but widely scattered ones
  - that way, likely to have empty buckets between used ones to have room to
    grow when collisions occur
  - if keys *cluster* to close int values, end up with more collisions

**todo: illustrate cluster**

- simple case is when have couple of entries with same hash and following
  buckets are empty

**todo: illustrate table with chain of entries of same key**

- can think of as doing separate chaining but where "linked list" threads
  through bucket array itself and pointers are calculated implicitly by
  probing sequence

- but trailing buckets can also be in use too, so can have multiple chains of
  interleaved, overlapping buckets

**todo: illustrate table with multiple chains of entries of same key**

- in practice, ok
- when looking up, check to see if key matches
- also handles cases where keys don't even have same hash
- just want to minimize number of buckets to probe by controlling load factor
- deleting gets harder, though
- get into that later

- enough overview to get started
- ok if vague
- plan implement hash table with string keys, open addressing, and linear
  probing

## building a hash table

- great thing about hash table, compared to tree or other data structure is
  actual structure so simple
- new module

^code table-h

- hash table is array of entries
- keep track of both how big array is and how many actual key/value pairs stored
  in it
- ratio is load factor
- entry

^code entry (1 before, 2 after)

- simple key value pair
- since key is always string, store objstring pointer directly
- faster and smaller than having to unwrap value every time
- [supporting other key types]

- to create new empty hash, declare ctor like fn

^code init-table-h (2 before, 2 after)

- new impl file
- get all includes out of the way right now
- much like dynamic ValueArray did earlier (todo: link), start initially
  empty with null pointer for array

^code table-c

- need to release mem too

^code free-table-h (1 before, 2 after)

- also like dynamic array
- [hash basically is dynamic array, with magic policy for where to put items]

^code free-table

- recall FREE_ARRAY gracefully handles null, so ok

### string hashes

- before can put stuff in hash table, need to be able to hash
- to ensure hash values as widely distributed as possible, hash fn should look
  at all string bytes
- if only looked at, say, first few chars, would get tons of collisions if had
  bunch of strings that only differed in suffix
- slow, would lose some perf benefit if had to walk string every time looked it
  up in hash table

- so do obvious thing: cache hash inside string obj
- over in obj module

^code obj-string-hash (1 before, 1 after)

- when first alloc create string obj, also calculate hash and store in struct
- kind of makes sense: allocate has to walk string anyway, already O(n)
- good time to get hash too

- pass hash to internal fn that allocs string obj

^code allocate-string (1 after)

- simply stores in struct

^code allocate-store-hash (1 before, 2 after)

- alloc called two places, in fn that copies string and one that takes ownership
- of existing string buffer
- start with first

^code copy-string-hash (1 before, 1 after)

- when called, calc hash of string
- then pass to alloc

^code copy-string-allocate (2 before, 1 after)

- take similar

^code take-string-hash (1 before, 1 after)

- actual work happens in hash string
- this is literal hash fn

^code hash-string

- hash fn design is a dark art
- touches on algo perf, low level instruction-level perf, statistics, crypto
- [link to austin's evaluation]
- using FNV-1a hash
- (http://www.isthe.com/chongo/tech/comp/fnv/)
- very simple, pretty old, fairly common
- lot more advanced ones do things like use vector instructions
- fun area to explore

- basic idea is simple
- start with initial hash value
- walk string
- every char, munge chars bits into hash and then mix things around
- "munge" and "mix" get very sophisticated
- lot of number theory over my head
- explore on your own

- now strings know hash, can start putting in tables
- first operation is set

^code table-set-h (1 before, 2 after)

- adds key/value pair to hash
- if key already present, overwrites
- returns true if key not already present

^code table-set

- take string key and use hash to figure out which entry in array it should go
  in
- check if there is already existing entry in there
- if so, replacing new value with same key
- otherwise, inserting new key and need to bump count
- store key/value pair in array

- one missing piece
- haven't actually alloc array yet
- before insert

^code table-set-grow (1 before, 1 after)

- similar to code for growing dynamic array
- if capacity not big enough for greater count, realloc and grow array
- [grow using multiplier so that adding new key/val is amortized constant time]
- one diff is max load
- this is how control load factor
- scales capacity down to ensure leave certain amount of empty slots

^code max-load (2 before, 1 after)

- semi arbitrary
- since not using lox for real world prog hard to tune
- in your impl, find realistic dataset care about and tune using that
- note that hash fn and way handle collisions highly affects this
- area of research is coming up with better collision strategies that handle
  higher load factor

- get to ajdust soon, assume table has array of some capacity
- to get bucket

^code find-entry

- know hash value of string, but covers entire numeric range
- first need to clamp to num array elements
- if actually clamped too high values to max, almost all values would collide
  in last bucket
- modulo wraps them in range while preserving scattering
- overlays ranges on top of each other

- also handle coll
- so start loop
- first probe in entry where key should be
- if bucket is empty, or bucket has entry with exact same key, found it
  - have to check key because bucket could be non-empty but belong to different
    key
  - could either be string that hashes to same value, or even different hash
    but part of other collision seq that led to it being here
- [== on strings, get to later]
- non-collide
- otherwise, need to find other bucket
- start probing
- linear probing, so just increment
- mod is to wrap around if reach end

- keep stepping through buckets until either find empty one or one with same
  key
- former is adding new key/value
- latter is replacing value for existing key

- infinite loop safe
- load factor ensures always empty cells
- pretty simple, but key fn for hash table
- spend large fraction of exec here
- [optimize later]
- returns array index

### allocating and resizing

- init table entry
- before can put stuff, need buckets

^code table-adjust-capacity

- allocs entry array of right size
- initialize all to empty
- store in table

- when set first key, call adjust with non-zero and do first alloc of array

- works for first alloc, but not resize
- if table already has entry arrays, need to handle existing entries and arrays

- need to move old entries over to new larger array
- can't just put in same buckets
- remember, took key hash and wrapped mod capacity size
- capacity has changed, so that will produce new value
- todo: ex

- need to recalculate bucket for each entry
- changing buckets means new collisions could happen
- easiest, just re-insert every entry using existing logic

^code re-hash (2 before, 2 after)

- uses find entry but passes in new entry array and capacity
- store old entry in new bucket in new array

- release old arrays

^code free-old-array (3 before, 3 after)

- not as easy as growing existing alloc like with array because need to
- move entries around

### retrieving items

- can insert key/value pairs, next look up

^code table-get-h (1 before, 2 after)

- given pointer to key, stores value at given pointer if present
- returns true if found or false

^code table-get

- this why find entry separate fn
- can reuse here
- almost all ops look up bucket, then differ in what do with it
- find entry returns index of empty bucket if key not found
- so can insert new key on set
- for get, can check that to see if found
- if so, fill output param

- while at it, add couple of other operations
- first is helper to add all entries from one table to another

^code table-add-all-h (1 before, 2 after)

- will use this later when implementing inheritance

^code table-add-all

- simple
- walk bucket array of table
- for every non-empty entry, add to other table
- go through full add process with hashing and collision res
- not really faster way to do it since each table may have diff cap and existing
  entries affect collision

### deleting entries

- one more operation, removing entry

^code table-delete-h (1 before, 2 after)

- won't actually use in book
- [considered adding syntax to lox specifically to let use, but seemed
  gratuitous]
- adding because many hash table tutorials never tell you how to delete
- with separate chaining, easy
  - find bucket, then remove from linked list
- open addressing much more subtle
- hardest operation!

- obv soln is to find entry and clear it out
- in non-collide case, fine

**todo: illustrate**

- but entry may be part of sequence of colliding entries
- remember, use empty bucket to tell when to stop probing
- if clear entry, won't find colliding entries after that slot
- like remove linked list node without relinking prev node to next

**todo: illustrate**

- could shift up subsequence nodes, like deleting from array

**todo: illustrate**

- but may also have interleaved sequence of entries from collisions in multiple
  buckets

**todo: illustrate**

- if shift up, could shift entry to *before* its first bucket
- bad

- could fix by removing and then re-adding stuff in cluster after
- kind of slow
- instead, trick called "tombstones"


^code table-delete

- first, find entry where key is
- if not present, just stop
- silently ignores deleting non-present

- if found, don't actually remove entry
- replace with special value that represents deleted entry
- that way don't have to shift around
- use nil key and non-nil value to rep tombstone
- delete simple, but need to handle tombstones in other op

- when looking for entry for key, use empty entry to know when to stop probing
  when key not found
- need to keep going past tombstone
- that way can find stuff after in probe seq

- if did pass tombstone, remember it
- that way if looking for entry to insert key, can reuse tombstone
- so declare locl to track tomb

^code find-entry-tombstone (1 before, 1 after)

- when probing check for tomb

^code find-tombstone (1 before, 1 after)

- if hit empty entry, done
- need to return entry caller can use to either set or tell if not present
- use tomb if found, otherwise empty entry just hit
- if hit tombstone, just remember and keep going
- if found key, same as before

- deleting adds tombstones
- probe seqs tend to get longer
- but when inserting, can reuse tombstone entry
- over time, stabilizes and number of tombstones doesn't get too large
- in practice many hash tables never delete
- [for ex in lox, no way to undeclare global var or remove instance field]

- note don't reduce count when deleting
- count includes tombstones
- that way, load factor takes tombstones into account and ensure still have
  empty entries

- when resizing and have to rehash anyway, tombstones can be discarded
- don't know how many non-tomb have, so recalc count
- clear

^code resize-init-count (2 before, 1 after)

- then increment each non-tom entry

^code resize-increment-count (1 before, 1 after)

- interesting how simple delete is but how many other parts need to be touched
  to support
- given that delete rare, expect would want to localize code there so other
  ops not slowed
- did some benchmarking
- seems to be faster
- all the textbooks do it this way
- [be suspicious of benchmarks. hash tables many many ways to vary perf.]

## string interning

- have mostly working hash table
- not using it for anything
- also not entirely working
- solve both problems

- reason not working is because use `==` to check for keys
- just does pointer equality
- remember in last chapter added explicit code to compare chars in strings for
  val equality
- needed that so that two strings at different point in mem are still equal
  if chars equal

- hash table doesn't do that
- could do that, but would be slow
- [in practice, would check string hashes first, so only have to compare chars
   in very rare cases where entire 32-bit hashes collide]
- instead, use as excuse to introduce common technique
- string interning

- core problem is can have distinct strings in mem with same chars
- essentially duplicates
- interning is deduplication
- any given seq of chars will only exist in one place in mem

- [exposed by languages as symbols, smalltalk, lisp, ruby, etc.
  lua interns all strings]

- much faster
- in order to make work, need to ensure not possible to have same string twice
  in mem
- do that by keeping table of every string allocated

^code vm-strings (1 before, 2 after)

- vm has table of string
- to use table, need include

^code vm-include-table (1 before, 1 after)

- when vm starts up, empty

^code init-strings (1 before, 1 after)

- free when shut down
- [later get to dealing with gc interaction]

^code free-strings (1 before, 1 after)

- to "intern" string means put in string pool if not already there
- some langs let you intern explicitly
- others like lisp ruby have separate string-like type usually called symbol
- lox, like lua, implicitly interns all strings

- when alloc string, put into table

^code allocate-store-string (2 before, 1 after)

- need to not do that if already in table
- instead, higher level fns to create strings first look for its char seq in
  string table
- if found, use that instead of creating new string
- otherwise, store string as key in table
- value not important so just nil
- [basically using hash table as hash set]

^code copy-string-intern (1 before, 2 after)

- take little different

^code take-string-intern (1 before, 1 after)

- takes ownership of given string
- if already have interned one, don't need, so free now
- need another include

^code object-include-table (1 before, 1 after)

- relies on new fn, similar to get

^code table-find-string-h (1 before, 2 after)

- looks like

^code table-find-string

- kind of redundant, but couple of key differences
- first pass in raw string to search for instead of objstring since don't have
  obj yet
- here is one place in code now where do compare strings char-wise
- this where dedupe is happening

- which means everywhere else, can safely use pointer compare on ObjString
- to test for string equals
- including in vm

^code equal (1 before, 1 after)

- now have key data structure needed for tracking variables and instance fields
- use it to build string intern table
- saves memory for strings
- nice side effect makes string compares much faster
- important for dynamically typed lang like lox
- method calls dispatched by string name
- so need name comparison to be fast
- why see many dyn langs use explicitly interned "symbol" type
- what they use for method names

**todo: move part about symbols up?**

- challenges
    - lots of knobs to tweak: hash fn, load factor, open/vs closed, etc.
      research other langs and see what they do
    - allow other value types as keys
    - write benchmark prog
          - how does choice of strings affect results?
          - number of strings?
