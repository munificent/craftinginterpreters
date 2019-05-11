^title Calls and Functions
^part A Bytecode Virtual Machine

## stuff to mention

- function prototype -- code but not closure -- is stored as constant in containing chunk

### function obj

...

^code obj-function (2 before, 2 after)

...

^code new-function-h (3 before, 1 after)

...

^code new-function

...

^code obj-type-function (1 before, 2 after)

...

^code free-function (1 before, 1 after)

...

^code print-function (1 before, 1 after)

...

^code is-function (2 before, 1 after)

...

^code as-function (2 before, 1 after)

...

### compiler stack

...

^code compiler-struct-fields (1 before, 1 after)

...

^code function-type-enum (1 before, 2 after)

...

^code current-chunk (2 before, 2 after)

**todo: references ObjFunction's chunk**

...

### calls

...

^code infix-left-paren (1 before, 1 after)

...

^code compile-call

...

^code argument-list

...

^code op-call (1 before, 1 after)

...

^code interpret-call (3 before, 1 after)

...

^code call-value

**todo: needs function and native obj types.**

**todo: split out native to separate snippet.**

...

^code call

...

^code disassemble-call (1 before, 1 after)

...

^code simple-instruction-n

...

### function declaration

...

^code match-fun (1 before, 1 after)

...

^code fun-declaration

...

^code compile-function

**todo: split up into pieces?**

...

### unsorted

...

^code reset-frame-after-call (2 before, 1 after)

...

^code compile-h (2 before, 2 after)

...

^code return-nil (1 before, 2 after)

...

^code init-compiler (1 after)

...

^code init-scope-and-function (1 before, 1 after)

...

^code init-function (1 before, 1 after)

...

^code end-compiler (1 after)

...

^code end-function (1 before, 1 after)

...

^code disassemble-end (1 before, 1 after)

...

^code restore-enclosing (4 before, 1 after)

...

^code return-statement

...

^code match-return (1 before, 1 after)

...

^code compile-signature (1 after)

...

^code call-init-compiler (1 before, 2 after)

...

^code call-end-compiler (4 before, 1 after)

...

^code debug-include-string (1 before, 2 after)

...

^code debug-include-object (1 before, 1 after)

...

^code object-include-chunk (1 before, 1 after)

...

^code is-native (1 before, 1 after)

...

^code as-native (1 before, 1 after)

...

^code obj-type-native (2 before, 2 after)

...

^code obj-native (1 before, 2 after)

...

^code new-native-h (1 before, 1 after)

...

^code new-native

...

^code print-native (2 before, 1 after)

...

^code vm-include-object (2 before, 1 after)

...

^code frame-max (2 before, 2 after)

...

^code call-frame (1 before, 2 after)

...

^code frame-array (1 before, 1 after)

...

^code vm-include-time (1 before, 2 after)

...

^code clock-native (1 before, 1 after)

**todo: fix to show location in sidebar**

...

^code reset-frame-count (1 before, 1 after)

...

^code runtime-error-stack (2 before, 2 after)

...

^code define-native

...

^code define-native-clock (1 before, 1 after)

...

^code run (1 before, 1 after)

...

^code trace-execution (1 before, 1 after)

...

^code push-local (2 before, 1 after)

...

^code set-local (2 before, 1 after)

...

^code jump (2 before, 1 after)

...

^code jump-if-false (2 before, 1 after)

...

^code loop (2 before, 1 after)

...

^code interpret-return (2 before, 1 after)

...

^code interpret (1 before, 2 after)

...

^code end-interpret (1 before, 1 after)

**todo: figure out how to handle deletion**

...

^code free-native (4 before, 1 after)

...

