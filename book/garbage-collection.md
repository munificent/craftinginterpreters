^title Garbage Collection
^part A Bytecode Virtual Machine

## stuff to talk about

- when to gc
- avoiding getting caught in cycles
- lang's ref versus vm's ref
- throughput vs latency
- tri-color abstraction
- collecting cycles
  - avoid cycles during tracing
  - actually being able to collect a unreffed cyclic group

- todo: put in stress testing before adding all code for finding roots and
  stuff so reader can see stuff get deleted

- todo: rename debug trace to something else since collides with tracing

- challenge to reimpl using lisp-1 or copying collector

## snippets

### collect

...

^code collect-garbage-h (1 before, 1 after)

...

^code collect-garbage

...

### marking roots

...

^code call-mark-roots (1 before, 1 after)

...

^code mark-roots

...

^code gray-value-h (1 before, 1 after)

...

^code gray-value

...

^code gray-object-h (1 before, 1 after)

...

^code gray-object

...

^code is-dark-field (1 before, 1 after)

...

^code init-is-dark (1 before, 2 after)

...

^code mark-closures (1 before, 1 after)

...

^code mark-open-upvalues (3 before, 1 after)

...

^code mark-globals (2 before, 1 after)

...

^code gray-table-h (1 before, 2 after)

...

^code gray-table

...

^code mark-compiler-roots (1 before, 1 after)

...

^code memory-include-compiler (1 before, 1 after)

...

^code gray-roots-h (1 before, 2 after)

...

^code gray-roots

...

^code compiler-include-memory (1 before, 1 after)

...

### tracing

...

^code call-trace-references (1 before, 1 after)

...

^code add-to-gray-stack (1 before, 1 after)

<!--
    // Not using reallocate() here because we don't want to trigger the
    // GC inside a GC!
-->

...

^code vm-gray-stack (1 before, 1 after)

...

^code init-gray-stack (1 before, 2 after)

...

^code free-gray-stack (2 before, 1 after)

...

^code trace-references

...

^code blacken-object

...

^code blacken-upvalue (2 before, 1 after)

...

^code blacken-function (1 before, 1 after)

...

^code gray-array

...

^code blacken-closure (1 before, 1 after)

...

^code check-is-dark (1 before, 1 after)

...

### sweeping

...

^code call-sweep (1 before, 1 after)

...

^code sweep

...

^code sweep-strings (1 before, 1 after)

...

^code table-remove-white-h (2 before, 2 after)

...

^code table-remove-white

...

### when to gc

...

^code reallocate-track (1 before, 1 after)

...

^code vm-fields (1 before, 2 after)

...

^code init-gc-fields (1 before, 2 after)

...

^code update-next-gc (1 before, 1 after)

...

^code heap-grow-factor (1 before, 2 after)

### debug log

...

^code define-log-gc (1 before, 2 after)

...

^code debug-log-allocate (1 before, 1 after)

...

^code log-before-collect (1 before, 2 after)

...

^code log-after-collect (2 before, 1 after)

...

^code log-gray-object (1 before, 1 after)

...

^code log-blacken-object (1 before, 1 after)

...

^code log-free-object (1 before, 1 after)

...

^code debug-log-includes (1 before, 2 after)

...

### stress

...

^code define-stress-gc (1 before, 2 after)

...

^code stress-gc (1 before, 1 after)

...

^code add-constant-push (1 before, 1 after)

**todo: remove comment**

...

^code add-constant-pop (1 before, 1 after)

...

^code chunk-include-vm (1 before, 2 after)

...

^code push-string (2 before, 1 after)

...

^code pop-string (1 before, 2 after)

...

^code concatenate-peek (1 before, 2 after)

...

^code concatenate-pop (1 before, 1 after)
