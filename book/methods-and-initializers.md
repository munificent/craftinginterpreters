^title Methods and Initializers
^part A Bytecode Virtual Machine

> When you are on the dancefloor, there is nothing to do but dance.
>
> <cite>Umberto Eco, <em>The Mysterious Flame of Queen Loana</em></cite>

**todo: adjust context lines for each snippet**

## sections

### method declarations

...

^code class-body (1 before, 1 after)

...

^code class-name (1 before, 1 after)

...

^code method

...

^code method-op (1 before, 1 after)

...

^code disassemble-method (2 before, 1 after)

...

^code class-methods (1 before, 1 after)

...

^code init-methods (1 before, 1 after)

...

^code mark-methods (1 before, 1 after)

...

^code free-methods (1 before, 1 after)

...

^code interpret-method (1 before, 1 after)

...

^code define-method

...


### bound method

...

^code obj-bound-method (2 before, 1 after)

...

^code is-bound-method (2 before, 1 after)

...

^code as-bound-method (2 before, 1 after)

...

^code obj-type-bound-method (1 before, 1 after)

...

^code new-bound-method-h (2 before, 1 after)

...

^code new-bound-method

...

^code blacken-bound-method (1 before, 1 after)

...

^code free-bound-method (1 before, 1 after)

...

^code print-bound-method (1 before, 1 after)

...

^code get-method (5 before, 1 after)

...

^code bind-method

...

^code call-bound-method (1 before, 1 after)

...

### this

**todo: illustrate stack layout for function versus method call**

...

^code table-this (1 before, 1 after)

...

^code this

...

^code slot-zero (1 before, 1 after)

...

^code method-type (2 before, 1 after)

...

^code method-type-enum (1 before, 1 after)

...

^code store-receiver (2 before, 2 after)

...

^code this-outside-class (1 before, 1 after)

...

^code current-class (1 before, 2 after)

...

^code class-compiler-struct (1 before, 2 after)

...

^code create-class-compiler (2 before, 1 after)

...

^code push-enclosing (1 before, 1 after)

...

^code pop-enclosing (1 before, 1 after)

...

### initializers

...

^code initializer-name (1 before, 1 after)

...

^code initializer-type-enum (1 before, 1 after)

...

^code return-this (1 before, 1 after)

...

^code call-init (1 before, 1 after)

**todo: split error handling to separate snippet?**

...

^code vm-init-string (1 before, 1 after)

...

^code init-init-string (1 before, 2 after)

...

^code mark-init-string (1 before, 1 after)

...

^code clear-init-string (1 before, 1 after)

...

^code return-from-init (3 before, 1 after)

...

### invocation

...

^code parse-call (3 before, 1 after)

...

^code invoke-op (1 before, 1 after)

...

^code disassemble-invoke (2 before, 1 after)

...

^code invoke-instruction

...

^code interpret-invoke (4 before, 1 after)

**todo: illustrate stack layout**

...

^code invoke

...

^code invoke-from-class

...

**todo: example of storing fn in field**

^code invoke-field (1 before, 1 after)

...
