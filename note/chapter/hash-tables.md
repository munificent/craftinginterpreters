--

Discuss symbols versus strings. Symbols are usually based on source code, not
strings created at runtime from things like concatenation. Some languages let
you explicitly "intern" a string that was created dynamically. Lua implicitly
interns all strings.

Some languages GC interned strings (Lua), some do not. To GC them, insert step
between mark and sweep that removes unmarked symbols from symbol table.

Key difference between symbols and strings is that symbols are canonicalized:
same sequence of characters always represented by exact same symbol object. Lets
you rely on reference equality to compare strings, which is much faster.

In source code, same sequence of characters may appear multiple times. So must
deduplicate at some point. (If language supports explicit interning, that API
must dedupe too.) Usually done during parsing or compiling.

Deduplication takes a little time, but only has to be done once and amortizes
cost of comparing strings many times later.

Dedupe using hash table: O(1). Could theoretically use something like red-black
tree or maybe even trie or prefix tree, but not done in practice.

Want to use symbols or interned strings for method and field lookup because
they're much faster than string comparison. Because Lua lets you use any string
object, including dynamically-created ones, as table keys, it interns all
strings. Vox doesn't have syntax or API for that, so we don't need to. Can
treat symbols and strings separately.

--

Most other sources use "symbol table" to refer to the data structure built up
during compilation that tracks types and lexical scopes. Though this source:

http://arantxa.ii.uam.es/~modonnel/Compilers/04_SymbolTablesI.pdf

distinguishes a few different kinds of symbol tables.

Things seem a little different in a dynamically typed language. In a static one,
the symbol table is mainly to refer to declarations of identifiers so things
like types and function calls can be resolved. But you eventually compile down
to lower level code that no longer needs to refer to the symbol. Similar to how
we handle local variables now in the compiler.

But for method and field names, "symbol" means something a little different,
more like what it means in the Lisp sense.

http://www.lispworks.com/documentation/lw70/CLHS/Body/t_symbol.htm
https://en.wikipedia.org/wiki/Symbol_(programming)
https://en.wikipedia.org/wiki/String_interning

--

Note that interning relies on strings being immutable!

--

Benchmarking string equality using interning versus comparing length and bytes:

comparing bytes:

          overhead   elapsed    equals
bytes     1.194243  1.977721  0.783478
interned  1.171333  1.364724  0.193391

Here, "overhead" is the time spent in the benchmark doing other stuff (looping,
loading vars, etc.). Elapsed is total execution time of the benchmark. Equals
is the difference, showing just the time spent doing the "==".

(Oof, just realized I probably forgot to turn off GC stress testing for this.
And ran a debug build.)

--

Mention Robin Hood hashing in the hash table chapter.

--

Talk about tombstones in hash table delete.

http://algs4.cs.princeton.edu/34hash/LinearProbingHashST.java.html
