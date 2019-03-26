^title Jumping Forward and Back
^part A Bytecode Virtual Machine

**todo: chapter title wraps and overlaps**

**todo: aside on "goto considered harmful"?**

---

talk about:

- have to patch jumps because don't know where forward dest is until after
  we've emitted more code

- aside on cpu jump instructions that look at flag registers?

- in jlox, used java's control flow to impl ours. here, need something lower
  level since don't use java's ip as our ip

- challenge to research how other bytecodes handle long jumps and do them in
  clox

- challenge: break. make sure to talk about scope

---

## branching

### if

...

^code parse-if (2 before, 1 after)

...

^code if-statement

- will explain why condition still on stack later

...

^code emit-jump

// Emits [instruction] followed by a placeholder for a jump offset. The
// placeholder can be patched by calling [jumpPatch]. Returns the index
// of the placeholder.

^code patch-jump

// Replaces the placeholder argument for a previous CODE_JUMP or
// CODE_JUMP_IF instruction with an offset that jumps to the current
// end of bytecode.

^code jump-ops (1 before, 1 after)

...

^code op-jump (1 before, 1 after)

...

^code read-short (1 before, 1 after)

...

^code undef-read-short (1 before, 1 after)

...

^code op-jump-if-false (1 before, 1 after)

...

^code disassemble-jump (1 before, 1 after)

...

^code jump-instruction (1 before, 1 after)

...

## logic ops

...

^code table-and (1 before, 1 after)

...

^code and

// left operand...
// OP_JUMP_IF       ------.
// OP_POP // left operand |
// right operand...       |
//   <--------------------'
// ...

^code table-or (1 before, 1 after)

...

^code or

// left operand...
// OP_JUMP_IF       ---.
// OP_JUMP          ---+--.
//   <-----------------'  |
// OP_POP // left operand |
// right operand...       |
//   <--------------------'
// ...

- aside on could have jump_if_true instr

## looping

...

^code parse-while (1 before, 1 after)

...

^code white-statement

...

^code emit-loop

...

^code loop-op (1 before, 1 after)

...

^code op-loop (1 before, 1 after)

...

^code disassemble-loop (1 before, 1 after)

...

### for

...

^code parse-for (1 before, 1 after)

...

^code for-statement

// for (var i = 0; i < 10; i = i + 1) print i;
//
//   var i = 0;
// start:                      <--.
//   if (i < 10) goto exit;  --.  |
//   goto body;  -----------.  |  |
// increment:            <--+--+--+--.
//   i = i + 1;             |  |  |  |
//   goto start;  ----------+--+--'  |
// body:                 <--'  |     |
//   print i;                  |     |
//   goto increment;  ---------+-----'
// exit:                    <--'

**todo: illustrate first desugaring to if and while, then desugaring that to the
jump control flow those compile to to show the overall structure**

^code for-initializer (1 before, 2 after)

...

^code for-begin-scope (1 before, 1 after)

...

^code for-end-scope (1 before, 1 after)

...

^code for-exit (2 before, 1 after)

...

^code exit-jump (2 before, 1 after)

...

^code for-increment (2 before, 2 after)
