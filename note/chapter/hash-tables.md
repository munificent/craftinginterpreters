
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
