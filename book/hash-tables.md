^title Hash Tables
^part A Bytecode Virtual Machine

---

...

^code table-c

...

^code init-table (2 before)

...

^code max-load (2 before, 1 after)

...

^code free-table

...

// Finds the entry where [key] should be. If the key is not already
// present in the table, this will be an unused entry. Otherwise, it
// will be the existing entry for that key.

^code find-entry

...

^code table-get

...

^code table-resize

...

^code table-set

...

^code table-delete

...

^code table-add-all

...

^code table-find-string

...

^code object-include-table (1 before, 1 after)

...

^code allocate-string (1 after)

...

^code allocate-store-hash (1 before, 2 after)

...

^code allocate-store-string (2 before, 1 after)

...

^code hash-string

...

^code take-string (1 before, 1 after)

...

^code copy-string (1 before, 1 after)

...

^code copy-string-allocate (2 before, 1 after)

...

^code init-strings (1 before, 1 after)

...

^code free-strings (1 before, 1 after)

...

^code obj-string-hash (1 before, 1 after)

...

^code table-h

...

^code table-find-string-h (1 before, 2 after)

...

^code entry (1 before, 2 after)

...

^code table (1 before, 2 after)

...

^code init-table-h (2 before, 2 after)

...

^code free-table-h (1 before, 2 after)

...

^code table-get-h (1 before, 2 after)

...

^code table-set-h (1 before, 2 after)

...

^code table-delete-h (1 before, 2 after)

...

^code table-add-all-h (1 before, 2 after)

...

^code vm-include-table (1 before, 1 after)

...

^code vm-strings (1 before, 2 after)

...

^code equal (1 before, 1 after)

...

