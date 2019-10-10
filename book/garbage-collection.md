^title Garbage Collection
^part A Bytecode Virtual Machine

> I wanna, I wanna,<br>
> I wanna, I wanna,<br>
> I wanna be trash.<br>
>
> <cite>The Whip, <em>Trash</em></cite>

**todo: figure out what illustrations to add**

- general goal of managed lang is user never worry about freeing memory
  - alloc alloc
  - lang give illusion of infinite mem
  - don't actually have infinite mem
  - [not far off with today's machines]
  - reclaim memory won't be needed
  - "wont be" hard, so conservative
  - [explain "conversative gc"]
    - if obj cannot be reached by user prog, not needed

- reachability
  - some objs directly accessed by vm itself
    - ex
  - rest reached by user program
  - don't know what code will exec in future, so what obj *could* be accessed
    - ex where value isn't used by prog but could be
  - first, any objs directly reachable
    - global vars
    - local vars on stack
    - call "roots"
    - todo: ex progs
  - from root, can also get to other objs by traversing objs root refs
    - todo: ex
    - can traverse long chain of refs
  - all of these reachable

- basic idea
  - at suitable time go through and reclaim mem
    - ["suitable time" hard]
  - figure out set of objs can't be reached
  - free them

## Mark-Sweep Garbage Collection

- several different approaches
- [other classic are ref count and copying]
- describe simplest, oldest here
- mark-sweep
- invented by mccarthy
- still good
  - many advanced gcs use algo for some things

- goes in two phases
  - mark
    - traverse graph of all reachable objs
    - literally just classic graph traversal
      - [can be dfs or bfs, don't matter]
    - start with roots
    - walk refs until all reachable objs traversed
    - every time visit obj, mark it so know reachable
  - sweep
    - iterate over all alloc obj
    - if not marked, wasn't reached, so free

## Collecting Garbage

- all starts with

^code collect-garbage-h (1 before, 1 after)

- build impl incrementally

^code collect-garbage

- first question: when get called?
- actually hard
- get to real answer for now

- first useful tool, stress

^code define-stress-gc (1 before, 2 after)

- when enabled gc as often as possible
- terrible for perf
- but helps flush out bugs where gc happen at unexpected time
- [clox has some, will fix soon]

^code call-collect (2 before, 1 after)

- simple gcs generally collect before alloc
- [complex ones may run periodically or all the time on separate thread]
- natural point because already calling into mem mgr
  - only point in prog where actually need gc because need mem

- stress test gcs on every alloc
- test size because realloc also used to free or shrink
- don't need to gc then

### Debug logging

- real challenge with gc is opaque
- been running lots of lox programs just fine with no gc at all so far
- how to tell if gc doing anything useful?
- help ourselves adding other diag

^code define-log-gc (1 before, 2 after)

- when flag enabled, clox print to console whenever does anything interesting
  with mem

^code debug-log-includes (1 before, 2 after)

- need couple of includes
- want to know when gc happens

^code log-before-collect (1 before, 2 after)

- will log during gc, so also want to know when done

^code log-after-collect (2 before, 1 after)

- don't have gc, but do manage mem
- can log in existing alloc

^code debug-log-allocate (1 before, 1 after)

- free

^code log-free-object (1 before, 1 after)

- add more as add other operations

## Marking the Roots

- every ref from one object to another
- ex closure to fun it wraps
- forms big digraph
- traverse graph mark all reachable
- where to start?

- "roots"
- values known to be directly reachable without having to go through other
  obj to get to
- can because vm has direct ref for own use
- for example, call frame on stack has ref to closure for each call
- or can be values directly access from user program
- i.e. vars
- local vars on stack and table of global vars

- all called roots
- start traversal by marking all of those

^code call-mark-roots (3 before, 2 after)

- most roots are local variabls temps and other values on stack

^code mark-roots

- calls new fn

^code mark-value-h (1 before, 1 after)

- impl

^code mark-value

- some lox values not on heap at all
- don't participate in gc
- don't want to have to check every val, so make help fn
- real work is in other fn

^code mark-object-h (1 before, 1 after)

- marks given lox obj

^code mark-object

- first check if null
- some places in vm have pointer to obj, but optional
- then mark obj
- set new field
- lives in obj head all obj share

^code is-marked-field (1 before, 1 after)

- all new objs initially unmarked because don't know if reached or not

^code init-is-marked (1 before, 2 after)

- want to log when obj marked

^code log-mark-object (2 before, 1 after)

- other main source of roots is global vars

^code mark-globals (2 before, 1 after)

- use helper

^code mark-table-h (1 before, 2 after)

- impl

^code mark-table

- when think of objs accessible to program, vars are main
- but lots of "hidden" roots
- places where vm accesses obj even if user prog can't
- must mark those too or have subtle bugs

- most data lives in main stack, but have separate callframe stack
- has closure pointers

^code mark-closures (1 before, 1 after)

- vm open upvalue list other place where vm has direct ref to values

^code mark-open-upvalues (3 before, 1 after)

- gc can even happen in middle of *compiling* code
- compiler does do dyn alloc for things like constants and any alloc can
  potentially trigger
- need to make sure objs compiler has ref to also get reached

^code call-mark-compiler-roots (1 before, 1 after)

- let compiler define that
- need include

^code memory-include-compiler (1 before, 1 after)

- decl

^code mark-compiler-roots-h (1 before, 2 after)

- impl

^code mark-compiler-roots

- only obj need to worry about is objfn currently being called
- recall fns nest so compiler has linked list of fns
- need to keep all

- to call mark, compiler needs

^code compiler-include-memory (1 before, 1 after)

## Tracing Object References

- every obj user or vm directly reach now has mark flag set
- each of those objs can potential ref other obj
- for ex, fns contain constant table with other objs
- closure has ref to fn
- now trace through those refs to reach indirect obj

- classic graph traversal
- could do bfs or dfs, doesn't really matter
- [for gcs that move objects in memory, order does matter because affects which
  objs end up next to each other. affects locality ... perf. even then, not
  clear which order better]

- important part is don't lose track while in middle of traversal

### Tri-color abstraction

- tri-color abs
  - [important especially for gcs that can be interrupted, ours stop the world]
  - work through objects
  - tri-color abs
  - [more advanced add more colors: light gray, dark gray, purple]
  - initially all objs white
  - when reach obj, turns gray
  - take obj, after processing all refs, turn black
  - white region unvisited
  - black region all done
  - gray fringe between two for reachable objs left to visit
  - begin marking all roots gray
  - work through gray processing refs and turn black
  - once no more gray, all done
  - any remaining white objs unreachable and can be freed
  - then turn all black objs back to white for next gc

- marked roots, but can't easily find them
- don't want to have to walk entire obj list
- keep separate worklist of gray
- when obj marked, add to list

^code add-to-gray-stack (1 before, 1 after)

- could be stack or queue
- if stack, dfs, if queue, bfs
- pick stack because simpler to impl

- work just like other dyn arrays in clox
- note that using sys `realloc()` not our reallocate!
- memory for gray stack itself *not* managed by gc
- wouldn't want growing the gray stack to recursively trigger a gc
- could tear hole in space time continuum

- vm owns gray stack

^code vm-gray-stack (1 before, 1 after)

- initially empty

^code init-gray-stack (1 before, 2 after)

- since gray stack manually managed, need to free

^code free-gray-stack (2 before, 1 after)

- when vm shuts down

- now after marking roots, all sitting in gray stack
- call next phase

^code call-trace-references (1 before, 1 after)

- impl

^code trace-references

- simple dfs traversal
- as long as still gray to process, pop one off and traverse refs

- that turns gray obj black
- at same time, traverse refs may in turn mark other objs
- turns white objs gray and pushes onto stack new objs onto stack
- iterative back and forth process where first turn gray object black
  - turns other white objs gray
  - reach those and traverse to turn black
  - then their refs become gray
  - in that way, gray wavefront percolates through field of white objs,
    incrementally darkening reachable ones

- no separate mark bit for black
- implicit by fact that obj no longer on gray stack

^code blacken-object

- each obj kind has fields that store refs to other objs
- start with easiest
- native and string have no outgoing refs so no work to do
- [easy opt in mark obj would be to skip putting these on gray stack entirely]

- start working through other obj types

^code blacken-upvalue (2 before, 1 after)

- when upvalue is closed, closed over obj lives right inside upvalue
- [when open still on stack, so tracked as root]
- trace that

^code blacken-function (1 before, 1 after)

- function has ref to obj string for its name
- also has constant table which may have many objs

^code mark-array

- walks value array marks everything in it

- last obj type

^code blacken-closure (1 before, 1 after)

- has ref to raw fn wraps
- also array of upvalues for variables closes over

- add logging too

^code log-blacken-object (1 before, 1 after)

- now have working code to traverse graph of obj refs
- graph is directed but may not be acyclic!
- can have cycles
- right now, if that happens, get stuck in infinite loop
- keep remarking object and retraversing
- easy fix

^code check-is-marked (1 before, 1 after)

- if obj already marked, don't mark again, don't add to gray

## Sweeping Unused Objects

- at this point, empty gray stack
- all objs either black or white
- black objs reached, want to keep
- anything still white cannot possibly be used
- all left to free them

^code call-sweep (1 before, 1 after)

- just one fn

^code sweep

- simpler than marking
- walk singly linked list of all alloced objs
- if encounter marked obj, keep it and keep going
- otherwise, found unreachable obj
- remove from list
- little extra code to handle special case where removing first node in list
- the free object itself
- uses same code to free obj and memory it owns that been using when shutting
  down vm
- just call it earlier now

- one little change

^code unmark (1 before, 1 after)

- after calling sweep, only black objects remain
- mark bits still all set
- when time for next gc, everything starts white
- go ahead and clear mark bits now to prep

- almost done
- other corner of vm little interesting around memory
- vm interns all strings
- means vm maintains own table of every single string in memory so can
  deduplicate
- *didn't* traverse that table during mark phase
- if did, every string would be considered reachable
- would never free a string ever

- string table is special
- needs to keep reference to all string that *are* used
- but does not *cause* them to be considered used
- should still be able delete string from string table if nothing else
  references it
- but if string stays around, table wants to find it
- called weak ref
  https://en.wikipedia.org/wiki/Weak_reference
- needs handling in gc
- [weak refs generally need specially handling in gc, which is why some langs
  do not offer, can be difficult in complex gcs]

^code sweep-strings (1 before, 1 after)

- calls new fn over in hash table

^code table-remove-white-h (2 before, 2 after)

- since traverse phase does not mark, table can contain refs to unmarked white
  objs
- sweep phase will delete, leaving dangling pointers in table
- would be very bad
- right between marking and sweeping, know which objs *will* be freed but all
  still alive
- right then, tell table to discard any refs to strings that about to be freed

^code table-remove-white

- walks entry array in hash table
- any string key obj not marked about to be swept
- remove from table

- now safe to sweep without dangling refs

## Garbage Collection Strategies

- have fully working gc
- with debug stress flag, gets called all the time
- with logging can see does stuff
- but without those flags, never does anything
- how should work in normal exec?
- when should gc run?

- question poorly answered by literature
- back in early software days when prog was given small fixed amount of ram
  easier: run when running out of mem
- modern os with virtual mem and overcommit "running out" not well defined
- progs can easily have heaps gigs
- would wait until huge amount of mem alloc and run gc late as possible
- but when do, gc would have ton of work to do to walk all objs and free
- [especially because gc is stop the world]

### Latency and throughput

- time to talk few important concepts
- gc manages memory on user's behalf
- don't have to explicitly free
- but comes at perform cost
- gc does some amount of work at runtime to figure out what to collect
- [research on compile time mem mgmt]
- time spent on gc time not spent running user code
- goal to minimize impact gc has on prog perf
- two main aspects

- throughput - total amount of time spent running user code versus doing gc
  work

- latency - largest continuous chunk of time where no user code exec because
  entirely paused while gc works

- say user program is bakery selling fresh-baked bread to customers
- throughput is total number of customers can serve in a day
- latency is how long takes to serve single customer loaf of bread

- gc is like temporarily shutting down bakery to sort dishes between being used
  from dirty ones and washing dirty ones
- since no baking happens, lowers throughput and increases latency
- both bad

- in practice, different gc designs trade off one for other
- when run gc, spends amount of time for each live obj and some amount for each
  dead one

- in analogy, we as baker have to sort dishes into in use and done piles
- then in use ones get put back where being used
- have to wash done ones

- gc author control trade-off partially by style of gc
- copying spend almost no time on dead obj
- our gc is "stop the world" which means does entire collection while user
  program paused
- high latency
- incremental gc can do little bit of gc, resume program for a bit, switch back
  to gc
- much better latency, but some overhead when pausing and resuming gc, so at
  expense of throughput
- concurrent gc does various amounts of gc work on separate thread
  - like hiring separate dishwasher to do dishes while bakers bake
- assuming cpu has multiple core, can be big improvement
- but lot of complexity and some overhead to coordinate and make sure dishwasher
  and baker don't run into each other when grabbing same dishes

- even with simple gc like ours, still have knobs can turn to tweak
- biggest is how often we run gc
- more frequently run gc, more time wasted re-sorting same live obj over and
  over again
- imagine sorting into piles every five minutes only to put most of them back
  where were
- in worst case, doing that so often never have time to actually finish anything
  and never end up with any dishes can wash
- [with stress flag enabled, any time need a new dish, do this whole process]

- doing dishes less frequently reduces wasted time visiting dishes still in use
- increases total throughput
- but number of dirty dishes piles up
- when finally do it, spend a lot of time washing them all
- if happen to do that when some customer waiting for bread, going to wait a
  long time
- latency goes way up
- while on average will serve more customers in given amount of time, some
  customers will get much worse service

- no perfect answer here
- some kinds of user progs care more about throughput
- if prog is doing big batch of data processing, just want to get as much done as fast as
  possible
- max throughput
- if prog is responding to user interaction, need immediate response
- lowest latency
- why many vms let prog tune gc params

### Self-adjusting heap

- strategy here simple but works ok
- [think, not sure]
- keep track of total number of bytes alloc
- if goes above threshold, trigger gc
- keep track of how many bytes remain
- adjust threshold based on that
- if had fixed threshold, might gc too often for progs that use lot of mem
- too infreq for progs don't keep much around
- tries to auto balance

- add two fields to vm

^code vm-fields (1 before, 2 after)

- first tracks how many outstanding managed bytes vm has on heap
- second is threshold
- when alloc goes above, triggers gc

- init on startup

^code init-gc-fields (1 before, 2 after)

- initial threshold pretty arbitrary
- like init cap on dynamic array
- if had real-world lox prog to use as benchmark, could profile and adjust
- don't, so just pick
- [challenge with gcs is really hard to learn best practices in lab env
  really need big messy real-world prog chewing on lot of data to actually see
  how gc strat affects perf]

- every time alloc mem, adjust running counter of how many bytes on heap

^code updated-bytes-allocated (1 before, 1 after)

- if cross threshold, run gc

^code collect-on-next (4 before, 1 after)

- finally vm has live gc that will kick in during normal op without stress
  flag

- after gc, know how many bytes still remain
- scale threshold to match

^code update-next-gc (1 before, 2 after)

- new thresh is multiple of remaining live bytes
- as working set grows, threshold moves farther out to avoid thrashing gc

^code heap-grow-factor (1 before, 2 after)

- also pretty arb
- would want to tune in real world

- can now log more useful stats

^code log-before-size (1 before, 1 after)

- capture heap size before gc
- then log at end

^code log-collected-amount (1 before, 1 after)

- show how many bytes collected how many remain

## Garbage Collection Bugs

- in theory, all done now
- have working gc
- kicks in automatically as needed during alloc

- alas, now enter dark dungeon of gc bugs
- gc free every single obj it can get hands on if think obj isn't used
- may happen at almost any point in time
- any time do dynamic alloc, could trigger gc
- we alloc all over place
- during compilation, while running code, etc.

- like musical chairs
- at any point when we alloc, gc might stop music
- every single heap alloc obj we're using needs to find a chair
- either be marked as root, or be reffed by some other obj
- if miss one, if have obj that vm is using by gc doesn't mark, it will delete
  right under us
- no fun

- how can vm still be using obj that gc doesn't see as root?
- typical answer is that it's in local variable on c stack
- [conversative gcs like boehm will walk c stack to look for these]
- when marking roots, walk lox stack, but not c stack
- if obj not marked, gc deletes
- local var in c ends up with dangling ptr to garbage memory

- [part of challenge with gc bug is that when have dangling ptr to freed mem,
    can still look like live obj if bits there still have prev value
    easy fix is to have option to fill memory with special value like `0xcc`
    before clearing. that way easier to tell if looking at pointer to freed
    mem]

- in previous chapters, seen lot of code that pushes obj onto lox stack before
  doing some alloc
- then pops right back off
- seemed pointless
- now finally see why
- if that alloc ran gc, wouldn't see obj and would get freed
- since i wrote vm, (and wrote entire impl and tests with gc stress enabled
  before writing book), i caught and fixed most of these bugs

- but only *most*
- want you to get flavor of what like to actually impl lang on your own and
  once finish last chapter of book, no longer on guided path
- out in jungle on your own
- beasts out there!
- left a few for us to practice hunting down

- if turn on stress and start running some toy lox progs, can probably
  encounter few
- walk through now

- when compiler adds new constant to fn const table, const may be heap alloc
  obj like string of fn
- new obj is passed to `addConstant()` as fn parameter
- no other refs

- that fn adds it to constant table
- but constant table is dynamic array
- if array doesn't have enough capacity, allows new larger array
- can trigger gc

- right before add, push const onto stack

^code add-constant-push (1 before, 1 after)

- after added to table, safe
- compiler has ref to fn we're compiling
- it marks fn as root
- fn then traces into const table
- so can pop back off now

^code add-constant-pop (1 before, 1 after)

- need include for this

^code chunk-include-vm (1 before, 2 after)

- another one
- all strings interned in clox
- when create new string, add to table
- again underlying storage for table is dynamic array
- could realloc
- since string is brand new when intern, no other refs
- go ahead and put on stack

^code push-string (2 before, 1 after)

- and then pop once in string table

^code pop-string (1 before, 2 after)

- table itself does not require string to stay
- remember weak ref
- but need to still keep string around so can return from allocateString()
- after this, caller responsible for ensuring ref

- one last example
- in interpreter `OP_ADD` can be used to concatenate two strings
- pops string operands from stack
- allocs result string big enough
- fills with chars
- pushes result
- can see problem now, right?
- if alloc result string triggers gc, operands get collected because already
  popped

- instead, leave on stack

^code concatenate-peek (1 before, 2 after)

- do peek instead of pop
- that way still findable as root
- once done allocating result, no longer need to keep around so pop before
  pushing result

^code concatenate-pop (1 before, 1 after)

- these all pretty easy, especially because i showed you fix
- in real world, can be some of most difficult bugs to track down
- reason is that gc happens only periodically
- have bug somewhere where code fails to preserve ref to obj
- but that erroneous code may execute way before point in time when gc runs
- very hard to track backwards in time
- [concurrent gc, where dealing with both difficulty of low-level memory
  corruption, detecting bug much later than when source of issue, and adding
  in dealing with threading is notoriously difficult to debug.]

- thankfully, clox simple enough don't have many of these
- so now really all done
- not so bad right?

## design note: generational gcs

- few classic gc algorithms

- mark-sweep as seen here
- ref counting as known from python and some other languges
  - keep running tally of how many references are to any object
  - as soon as hits zero, immediately delete
  - avoids needing to run period stop-world collection phase
  - simple to implement
  - but can be slow to maintain ref counts and can't easily handle cyclic
    refs
- copying collection
  - keep two equal-sized big blocks of memory called "spaces"
  - all new allocs go in one "from" space
  - when run out of room, trigger collection
  - like m-s, start by walking roots
  - instead of marking, immediately copy reached object to other "to" space
  - once roots all copied over, walk all objs in "to" space
  - trace refs
  - when ref points back to obj in old "from" space, copy that obj
  - by time finished walking "to" space, all live objs have been copied
  - everything left garbage
  - can now instantly wipe "from" space instead of having to iterate over and
    free individual objs
  - compared to ms, much faster when many objs are garbage
  - only spends time on live objs, no time spend on garbage
  - but has mem overhead to maintain two spaces
  - and time for live obj proportional to size since have to copy all bytes

- what do mature gc do?
- most do mixture
- "generational" gcs

- early gc researches observed memory use of programs over time and formed
  "generational hypothesis"
- most objects in program are either very short lived or very long lived
- objs with "middle" lifespan relatively rare
- long-lived objs things like config data, static state, or other things that
  used for most of duration of prog
- short lived stuff temporary objs used during some computation and discarded
  once have result
- over many years, hypothesis seems to be mostly true

- **todo: draw bathtub curve**

- developed "generational gc" to optimize for both ends of curve
- each time gc runs considered "generation"
- every time obj survives gc -- still used -- gets "older"
- objs of different generations managed using different gc algo

- typically use copying collector for young objs
- "nursery"
- copying gc can alloc very quickly and free unused instantly
- but gets slow when have lot of large objects that are still alive through
  each gc
- have to copy them in mem over and over
- so when obj survives couple of copy collector runs, gets "tenured" to
  different gc
- often some variant of mark-sweep
- so what we learned here still relevant to some of most advanced gcs out there

## challenges

In Garbage Collection, mention isDark could be stuffed into same byte as type.
Could possibly even stuff type and isDark in low bits of next pointer.

1.  because of struct padding wasting entire byte in obj header for mark bit
    come up with something better

1.  clearing mark bit for each live object can be slow
    come up with something better

1.  instead of a mark-sweep gc, implement a copying collector using cheney's
    alg
    - note, unlike mark-sweep, means also have to implement own version
      of reallocate()
    - will only call into os to allocate two big slabs mem up front for spaces
    - after that, manage all dynamic mem yourself
