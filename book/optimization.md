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

### unsorted

...

^code define-nan-tagging (1 before, 1 after)

...

^code table-capacity-mask (1 before, 1 after)

...

^code init-capacity-mask (1 before, 1 after)

...

^code free-table (1 before, 1 after)

...

^code find-entry (1 before, 1 after)

...

^code initial-index (1 before, 1 after)

...

^code next-index (1 before, 1 after)

...

^code get-find-entry (1 before, 1 after)

...

^code adjust-capacity (1 before, 1 after)

...

^code re-hash (1 before, 1 after)

...

^code adjust-find-entry (1 before, 1 after)

...

^code adjust-free (1 before, 1 after)

...

^code adjust-set-capacity (1 before, 1 after)

...

^code table-set-grow (1 before, 1 after)

...

^code set-find-entry (1 before, 1 after)

...

^code delete-find-entry (1 before, 1 after)

...

^code add-all-loop (1 before, 1 after)

...

^code find-string-index (1 before, 1 after)

...

^code find-string-next (1 before, 1 after)

...

^code remove-white (1 before, 1 after)

...

^code mark-table (1 before, 1 after)

...

^code nan-tagging (1 before, 1 after)

**todo: split into smaller**

...

^code end-if-nan-tagging (1 before, 1 after)

...

^code print-value (1 before, 1 after)

...

^code end-print-value (1 before, 1 after)

...

^code values-equal (1 before, 1 after)

...

^code end-values-equal (1 before, 1 after)

...

