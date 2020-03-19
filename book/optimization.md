^title Optimization
^part A Bytecode Virtual Machine

### notes

- talk about profiling first
- some optimizations don't pan out
	- end up trying and discarding many
	- tricky because sometimes two optimizations individually not helpful but
	  in combination do help
	- dark art

---

Tried another simple optimization. Stored the ip in a local variable in the
bytecode loop. Like in Wren, it makes a big difference, about 10%. But the
code's a little ugly and I'm not sure if it's worth showing. Discarded for now.

---

See explanation of how `IS_BOOL()` works here:

https://github.com/munificent/craftinginterpreters/issues/565

---

https://nikic.github.io/2012/02/02/Pointer-magic-for-efficient-dynamic-value-representations.html

---

mention inlining small strings

---

...

## modulo

...

^code table-capacity-mask (1 before, 1 after)

...

^code init-capacity-mask (1 before, 1 after)

...

### find entry

...

^code find-entry

...

^code initial-index (1 before, 1 after)

...

^code next-index (4 before, 1 after)

...

### adjust capacity

...

^code adjust-find-entry (2 before, 1 after)

...

^code adjust-capacity (1 after)

...

^code re-hash (1 before, 1 after)

...

^code adjust-free (3 before, 1 after)

...

^code adjust-set-capacity (1 before, 1 after)

...

### table get set delete

...

^code get-find-entry (2 before, 1 after)

...

^code set-find-entry (3 before, 2 after)

...

^code table-set-grow (1 before, 1 after)

...

^code delete-find-entry (1 before, 1 after)

...

### other changes

...

^code add-all-loop (1 before, 1 after)

...

^code find-string-index (4 before, 2 after)

...

^code find-string-next (3 before, 1 after)

...

^code remove-white (1 before, 1 after)

...

^code mark-table (1 before, 1 after)

...

^code free-table (1 before, 1 after)

...

## nan tagging

...

^code define-nan-tagging (2 before, 1 after)

...

^code nan-tagging (2 before, 1 after)

...

^code end-if-nan-tagging (1 before, 2 after)

...

### numbers

**todo: remove comments**

...

^code number-val (1 before, 2 after)

...

^code num-to-value (1 before, 2 after)

...

^code double-union (1 before, 2 after)

// A union to let us reinterpret a double as raw bits and back.

...

^code as-number (2 before, 2 after)

...

^code value-to-num (1 before, 2 after)

...

^code is-number (1 before, 2 after)

// If the NaN bits are set, it's not a number.

...

^code qnan (1 before, 2 after)

// The bits that must be set to indicate a quiet NaN.

...

### nil and singleton values

...

^code nil-val (2 before, 1 after)

...

^code tags (1 before, 2 after)

// Tag values for the different singleton values.

...

^code is-nil (2 before, 1 after)

...

### booleans

...

^code false-true-vals (2 before, 1 after)

...

^code bool-val (2 before, 1 after)

...

^code as-bool (2 before, 1 after)

...

^code is-bool (2 before, 1 after)

...

### objects

...

^code obj-val (1 before, 2 after)

// The triple casting is necessary here to satisfy some compilers:
// 1. (uintptr_t) Convert the pointer to a number of the right size.
// 2. (uint64_t)  Pad it up to 64 bits in 32-bit builds.
// 3. Or in the bits to make a tagged Nan.
// 4. Cast to a typedef'd value.

...

^code sign-bit (1 before, 2 after)

// A mask that selects the sign bit.

...

^code as-obj (1 before, 2 after)

...

^code is-obj (1 before, 2 after)

### value functions

...

^code print-value (1 before, 1 after)

...

^code end-print-value (1 before, 1 after)

...

^code values-equal (1 before, 1 after)

...

^code end-values-equal (1 before, 1 after)

...
