^title Calls and Functions
^part A Bytecode Virtual Machine

- Function Objects
- Compiling to Function Objects
- Call Frames
- Function Declarations
- Function Calls
- Native Functions
- Return Statements
- Stack Traces




## stuff to mention

- function prototype -- code but not closure -- is stored as constant in containing chunk
- how early fortran compilers used self-modifying code to handle returns

- challenge to store ip in local variable and see how affects performance

## Function Objects

...

^code obj-function (2 before, 2 after)

...

^code object-include-chunk (1 before, 1 after)

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

## Compiling to Function Objects

...

^code function-fields (1 before, 1 after)

...

^code function-type-enum

...

^code current-chunk (2 before, 2 after)

...

^code call-init-compiler (1 before, 2 after)

...

^code init-compiler (1 after)

...

^code init-function (1 before, 1 after)

...

^code init-function-name (1 before, 1 after)

**todo: move local slot init to separate snippet**

...

^code disassemble-end (1 before, 1 after)

...

^code end-compiler (1 after)

...

^code end-function (1 before, 1 after)

...

^code return-function (4 before, 1 after)

...

^code compile-h (2 before, 2 after)

...

^code compile-signature (1 after)

...

^code call-end-compiler (4 before, 1 after)

...

## Call Frames

...

^code frame-array (1 before, 1 after)

...

^code call-frame (1 before, 2 after)

...

^code frame-max (2 before, 2 after)

...

^code reset-frame-count (1 before, 1 after)

...

^code run (1 before, 1 after)

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

^code trace-execution (1 before, 1 after)

...

^code interpret-stub (1 before, 2 after)

...

^code end-interpret (1 before, 1 after)

**todo: figure out how to handle deletion**

...

## Function Declarations

...

^code match-fun (1 before, 1 after)

...

^code fun-declaration

...

^code compile-function

**todo: split up into pieces?**

...

^code enclosing-field (1 before, 1 after)

...

^code store-enclosing (1 before, 1 after)

...

^code restore-enclosing (4 before, 1 after)

...

## Function Calls

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

...

^code call

...

^code check-arity (1 before, 1 after)

...

^code check-overflow (2 before, 1 after)

...

^code update-frame-after-call (2 before, 1 after)

...

^code interpret (2 before, 2 after)

...

^code interpret-return (1 before, 1 after)

...

^code return-nil (1 before, 2 after)

...

^code disassemble-call (1 before, 1 after)

...

^code simple-instruction-n

...

## Native Functions

...

^code obj-native (1 before, 2 after)

...

^code new-native-h (1 before, 1 after)

...

^code new-native

...

^code obj-type-native (2 before, 2 after)

...

^code free-native (4 before, 1 after)

...

^code print-native (2 before, 1 after)

...

^code is-native (1 before, 1 after)

...

^code as-native (1 before, 1 after)

...

^code call-native (2 before, 2 after)

...

^code define-native

...

^code clock-native (1 before, 2 after)

...

^code define-native-clock (1 before, 1 after)

...

^code vm-include-time (1 before, 2 after)

...

^code vm-include-object (2 before, 1 after)

...

## Return Statements

...

^code match-return (1 before, 1 after)

...

^code return-statement

...

^code return-from-script (1 before, 1 after)

## Stack Traces

...

^code runtime-error-stack (2 before, 2 after)

...
