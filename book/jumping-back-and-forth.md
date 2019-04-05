^title Jumping Back and Forth
^part A Bytecode Virtual Machine

**todo: quote (for prev chapter too)**
**todo: chapter title overlaps**

- control flow
- in jlox, used java's own control flow structures for lox's
  - implemented lox if using java if
  - lox while using java while
- how java do it?
- turtles all the way down

- different approach for clox
- bytecode necessitates
- what "control" mean?
- "where" you are in program
- which code executing next
- in jlox, implicit based on which ast node stored in various java vars
- in clox, control explicit
- vm's ip stores address of current instr
- after exec each instr, increment
- how execution moves through program
- if didn't incr, would exec same instr over and over

- control flow just how that ip value changed
- simplest control flow if without else
- evaluates condition -- executes code for cond expr
- if result is truthy, execs then block
- true case not that interesting, just like normal execution
- interesting is false: skips over code in then body
- if set ip to address of instr after then instrs, would skip
- between cond and then body, just need special instruction
- looks at value on stack
- if false, jump over range of instructions land on next instr after then
- distance to jump stored as operand for instruction

- note no "structured programming"
- nothing to prevent jump instruction from jumping into middle of expression,
  out of block, across variable scopes
- basically conditional goto
- at bytecode level, unstructured
- up to us in compiler to make sure don't jump places that break invariants
- in particular, need to make sure as jump around, all paths through code
  leave stack in well-defined state

- exactly how real cpus behave
- beneath every elegant structured lang, gets compiled down to just jumping
  around freely between instructions
- all of that "structure" is maintained by higher level lang
- encoded in syntax limitations of grammar (block scope),
  type system, compiler, etc.

- anyway, condition jump all need to implement if
- get more complicated with else and looping, but let's get started

## if

- front to back

^code parse-if (2 before, 1 after)

- if is statement, keyed of keyword

^code if-statement

- [ever notice that opening paren is useless? need closing one or some token there to tell when condition expression ends and body begins, but `(` is just for humans because unbalanced `)` would look weird]

- compile condition expr
- leaves result on stack
- then emit jump instruction
- will look at condition

- [jump instrs in clox look at value on stack. in real cpu, arith and logic
  instrs calc result and also store extra bits in flags reg for things like
  "result was zero". jump instrs there often look at flags instead of value]

- if true, does nothing and keep executing next instrs which is then body of if
- if false, jumps over then to end of if statement

**todo: illustrate**

- but problem
- jump instruction needs offset for how far to jump to skip over then body
- but haven't compiled then body yet
- don't know how many instrs it has

- use classic trick
- emit jump instruction with placeholder offset
- keep track of where that instruction is
- then compile body
- now we know where need to jump to
- bytecode is just mutable byte buffer
- so go back and replace jump operand with offset

- use two helpers

^code emit-jump

// Emits [instruction] followed by a placeholder for a jump offset. The
// placeholder can be patched by calling [jumpPatch]. Returns the index
// of the placeholder.

- writes given jump instruction and two placeholder bytes for 16-bit offset
- pass in opcode because will actually have couple of kinds of jump instr
- returns offset of instr in chunk

^code patch-jump

// Replaces the placeholder argument for a previous CODE_JUMP or
// CODE_JUMP_IF instruction with an offset that jumps to the current
// end of bytecode.

- call this right after emitting last instr want to jump over
- calculates offset between end of code and given position of jump instr
- replaces operand for instr with offset

- get new instr working

^code jump-if-false-op (1 before, 1 after)

- in vm

^code op-jump-if-false (2 before, 1 after)

- offset is 16-bit operand
- 8-bits not enough for practical use
- [even 16 not very far, real vm probably also have long jump instr]

- checks cond on top of stack
- if falsey, add offset to ip
- using underlying control flow in c to conditionally change ip
- not using it to actually conditionally exec then branch
- [aside from "is falsy" logic, could do this in pure arith. imagine knew that
condition would always be number, zero if false, 1 if true. then just do
    vm.ip += peek(0) * offset]

- if not falsey, leave ip alone, will continue on to next instr
- don't need to do anything else
- after exec this instr, outer vm loop dispatch next instr
- since ip changed, that will be first instr after then branch

- macro to read 16 bit operand

^code read-short (1 before, 1 after)

- reads next two bytes gloms together big-endian

- clean up

^code undef-read-short (1 before, 1 after)

- now have working control flow

### else

- next add else support

- after compile then branch, compile else

^code compile-else (1 before, 1 after)

- if see else keyword, compile statement for else body

- not enough, though

**todo: illustrate broken flow**

- if cond false, jumps to else branch
- exec else and then keep going after if statement, good
- but if cond true, exec then branch as desired
- but then keep exec right into else branch
- when cond true, after then branch exec, need to jump over else

^code jump-over-else (2 before, 1 after)

- patch offset after end of else body

^code patch-else (1 before, 1 after)

- requires new kind of jump
- that jump not conditional
- then always skips over else
- new instruction

^code jump-op (1 before, 1 after)

- in vm

^code op-jump (1 before, 1 after)

- very similar
- just always applies offset
- now if with else works correctly

**todo: illustrate flow with else**

- little bit to clean up
- note jumpif peeks cond, not pop
- still sitting on stack
- statements must not leave values on stack
- instr is correct
- in next section, want to have that value still on stack for other use
- but if doesn't need
- so simply pop
- need to do in both branches

- when cond true, at beginning of then branch

^code pop-then (1 before, 1 after)

- when cond false, at beginning of else branch

^code pop-end (1 before, 1 after)

- means if bytecode always has else section even if no else *statement*
- simply pops cond before exec next statement

**todo: illustrate flow with empty else that just pops**

- now fully working right
- just dis

^code disassemble-jump (1 before, 1 after)

- uses new helper

^code jump-instruction (1 before, 1 after)

- have first control flow syntax working
- move on to next

## logical operators

- probably remember this from jlox, but logical not just another bin op
- because they short circuit
- don't eval rhs at all if lhs of and is false or lhr of or is true
- to short circuit, need to jump over code for eval rhs
- means really kind of control flow
- just flavor of if then else

- start alphabetically with and
- hook up new operator to parsing tabl

^code table-and (1 before, 1 after)

- calls

^code and

**todo: illustrate**

// left operand...
// OP_JUMP_IF       ------.
// OP_POP // left operand |
// right operand...       |
//   <--------------------'
// ...

- called after lhs has already been compiled and we hit `&&`
- at runtime, result of lhs on top of stack
- if lhs false, entire and expr must be false, so result is lhs and skip right
- instr just what we want
- jump to end
- jump instr leaves value on stack
- which is what want, becomes result of expr
- this is why jump doesn't pop
- need it for logic op

- if lhs truthy, result is whatever rhs results in
- so pop lhs then compile rhs

- finally, patch jump for false case to jump over rhs

- other logic op similar
- add to table

^code table-or (1 before, 1 after)

- calls

^code or

- little different
- in or, if lhs is truthy, that is result and skip rhs
- if falsey, continue eval rhs and that's result

- need to jump if truthy
- could add new instr
- [probably should really, have plenty of free opcodes]
- instead, just desugar

**todo: illustrate**

// left operand...
// OP_JUMP_IF       ---.
// OP_JUMP          ---+--.
//   <-----------------'  |
// OP_POP // left operand |
// right operand...       |
//   <--------------------'
// ...

- do funny double jump
- can only jump when operand is false
- when true, continues to next instr
- so make that instr unconditional jump to end
- then first conditional jump jumps *over* that to code for rhs

- [other option would be to simply emit OP_NOT to negate lhs and then jump
  based on that. but doesn't preserve value. || evals to lhs if truthy, but
  must preserve exactly *which* truthy val]

- probably trying to hard to minimize instrs, but kind of fun
- [also kind of slow...]

// If the operand is *true* we want to keep it, so when it's false,
// jump to the code to evaluate the right operand.

  // If we get here, the operand is true, so jump to the end to keep it.

  // Compile the right operand.


- those three control flow that only do branching
- only jump forward over code
- in other langs, also have switch and maybe conditional operator
- other flow control is looping -- jumping backwards in instruction stream

- [notice no run code in this section? theme of compiler lowering to simpler
  set of vm ops]

## looping

- simplest loop is while
- starts with keyword

^code parse-while (1 before, 1 after)

- calls

^code while-statement

- parenthesized cond
- leaves result on stack
- if condition false, exit loop by jump over body to end
- otherwise, execute body
- since jump leaves value on stack, again need to pop both before body and at
  end
- after body patch jump for exiting loop

- all missing is loop, looks like

^code loop (1 before, 2 after)

- after body, emit loop
- needs to know where to jump back to
- for jumping forward, don't know dest until after compile more, so have to
  go back and patch
- when jumping back, already know point want to jump back to since already
  compiled it
- just need to remember it
- at top of while before condition, capture pos

^code loop-start (1 before, 1 after)

- pass that to emit loop fn

^code emit-loop

- emit new loop instruction
- unconditionally jump back
- from vm perspec, no real difference between jumping forward and back
- both just adding offset to ip
- but chose to use unsigned 16 bit for jump offset
- to jump back in loop, need negative offset
- so add other instruction

- like op_jump, 16 bit unsigned op for offset to ip
- this time offset subtracted
- ip will have stepped past op_loop operand bytes too, so need to include those
  when calc offset

- new instr

^code loop-op (1 before, 1 after)

- in vm

^code op-loop (1 before, 1 after)

- literally only difference from op_jump is `-` instead of `+`

- dis too

^code disassemble-loop (1 before, 1 after)

- because while evals cond at top and jumps over body when cond false, need
  two jumps, forward and back
- total flow looks like:

**todo: illustrate**

### for

- other looping statement is for
- lot more going on compared to while:

- have optional init
  - runs once at top of body
  - basically like separate statement before for loop
- then cond expr
  - works like while where if false, exits loop
  - if omitted, treated as always true
- then increment expr
  - executed at end of each iteration
  - even though textually before body, exec after body
- then body

- compiler will compile that down using jump and loop instrs already have
- flow looks like:

**todo: illustrate**

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

- more complex than necessary
- because single pass compiler, need to compile increment clause before body
  even though exec after
- so have to jump over increment to body then back to increment
- [if parse to ast, could output code in more logical order by visiting body
  first. didn't have this problem in jlox]

- build up code
- start with keyword as usual

^code parse-for (1 before, 1 after)

- calls
- if only supported empty clauses like `for(;;)`, look like

^code for-statement

- just bunch of punctuation at top
- then loop body
- then loop back to top
- is valid syntax
- [to exit infinite loop, need return statement which don't have yet, so no
  good idea to use right now]

**todo: illustrate first desugaring to if and while, then desugaring that to the
jump control flow those compile to to show the overall structure**

- next add initializer
- just execs once before anything else, so simple

^code for-initializer (1 before, 2 after)

- syntax little complex since can be either expr or var decl
- use `var` keyword to tell
- for expr case, use exprstmt to handle trailing semicolon too
- in either case, doesn't leave value on stack

- if declare var, need to think about scope
- loop var should not exist past end of loop body, so wrap whole thing in new
  scope

^code for-begin-scope (1 before, 1 after)

- and end at end

^code for-end-scope (1 before, 1 after)

- next is condition to exit
- since condition optional, need to see if actually there
- if not, next token will be `;`, so look for that
- if is condition, compile
- much like while jumps to end
- since leaves cond on stack, need to pop before exec body

- also at end if exited loop

^code for-exit (2 before, 1 after)

- if is no condition expr, no value on stack, so only emit pop when is cond

^code exit-jump (2 before, 1 after)

- saved best for last, increment

^code for-increment (2 before, 2 after)

- again optional
- if absent, next is `)` so look for that
- when increment present, need to parse and compile now, but not exec
- so first emit uncond jump
- jumps over increment code to beginning of body
- then compile increment
- after increment, start loop over, so jump back to top
  - either cond or body if no cond

**todo: explain popping increment expr result**

- just about to compile body, so now patch jump at top of increment to skip
  increment

**todo: illustrate increment part of flow**

- little hairy but all works out
- again no vm changes
- big step towards imperative lang
- now turing complete!
- interesting required very few new instrs
- also interesting that instrs very close to actual turing machine -- jumps
  sort of like moving read head


<div class="challenges">

## Challenges

1.

- if is the main branching statement, but most c fam langs also have switch
- add switch to lox
- grammar

```lox
switchStmt  → "switch" "(" expression ")"
              "{" switchCase* defaultCase? "}" ;
switchCase  → "case" expression ":" statement* ;
defaultCase → "default" ":" statement* ;
```

- to keep easier, not fallthrough or break
- after case statements, jumps to end
- cases match by seeing if value in switch expr equal to case value

1.

- for jlox had challenge to add break
- this time, add continue
- jumps to top of nearest enclosing loop
- should be compile error to have outside of loop
- make sure to think about scope
- what should happen to local variables declared inside body of loop when
  continue jumps?

1.

- try to come up with novel control flow construct for lox
- in practice, rarely good idea because hard to come up something useful enough
  to justify making users learn it
- but fun to try

</div>

<div class="design-note">

## Design Note: Considering Goto Harmful

- discovering that all structured control flow in clox actually uses raw jumps
  like momentin scooby doo when monster rips mask off
- it was goto all along!

- feels dirty
- all know goto evil
- why?

- is true can write really unreadable code with goto
- most programmers today never seen it first hand
- just boogie man tell stories about over campfire

- slain by dijkstra
- "goto considered harmful" famous letter he wrote for acm magazine
- todo: check publication
- dijkstra well deserved fame
- invented several fundamental algorithms
- helped turn programming into real discipline

- letter so compelling effectively killed goto
- most languages after it don't have it
- how many people can say they almost single-handedly destroyed a language
  feature from nearly all future languages?

- given scale of effect, letter must be impressive?
- well
- encourage you to read yourself
- if you can get past dijkstra's self-important faux modest writing style
- ["More recently I discovered why the use of the go to statement has such disastrous effects... . At that time I did not attach too much importance to this discovery; I now submit my considerations for publication because in very recent discussions in which the subject turned up, I have been urged to do so."]

- really short
- good practice for reading cs papers
- reading papers very different for other kinds of text
- different skill
- technique is to rewrite line by line in my own words

- did that with his letter
- came away disappointed
- for someone famed for intellectual rigor, really lacking

- summarize

- as progs, write program, but goal is actual running program
- we create text, but dynamic behavior is what need to understand
- brains better at reasoning about static text than dynamic behavior
- [he asserts this, but doesn't actually offer any evidence]
- so, more we can make dynamic behavior of code follow textual structure,
  easier it is to reason about

- to make prog easier to reason about, dynamic behavior should follow structure
  of prog text as much as possible

- has idea that can define how close two correspond using "coordinate" that
  describes "progress" of running program
- little vague but imagine two computers running same prog with same input data
- pause both of them in middle of running
- what data would need in order to tell if both paused at exactly same point?

- if program just list of statements with no control flow easy: just need to
  know between which two statements paused at
- like ip in our vm or line number in runtime error message

- branching control flow like if and switch does not add any more state
- if line points into middle of statements inside then or else branch, can
  still unambig tell where are

- once add function calls need more
- could be in middle of function that is called by multiple places in program
- two computers could both be at same line in fn, but from different calls
- need line number, but also need to know location of callsite for fn
- that in turn may be inside fn with multiple calls, so location of that
- in other words, callstack you get in debugger showing what line currently in
  in every fn hasn't returned yet

- loops make it harder
- if line number points inside loop, don't know how many iterations have already
  run
- so just says also need loop counter
- if inside multiple nested loops, may need series of them

- really going somewhere
- then says "The unbridled use of the go to statement has an immediate consequence that it becomes terribly hard to find a meaningful set of coordinates in which to describe the process progress."
- doesn't actually offer proof or evidence, just declares it
- and that's it
- another short paragraph saying ifs and loops are "bridled" versions of goto
  that are safer to use
- may discover other useful control flow constructs, but general goto bad

- left unsatisfied

- it is true *never* need goto
- Böhm and Jacopini proved any control flow using goto can be implemented using
  sequence, loops, and branches
- https://en.wikipedia.org/wiki/Structured_program_theorem

- closer example of principle is clox itself
- implements behavior of jump instruction in c without using goto
- basic idea is wrap switch statement in loop
- each case is one labeled region of code
- instead of goto, set local variable that's being switched on
- next turn of loop, jumps to that case
- awful lot like clox's own interpreter loop

- is goto that bad?
- can definitely write bad code with it
- but can write bad code without
- arguably, code write to avoid using goto sometimes more complex than using
  goto
- for example, to exit out of nested loop, usually use extra guard var

- [most today would move code to separate fn and use return statement in
  middle of loop
  - early return itself example of unstructured control flow
  - along with break and continue, like mini-goto
**todo: example**

- so now have extra mutable state to reason about
- if could just jump right out, simpler

- like most, live without goto most of career
- but came away wondering if more for dogmatic reasons than pragmatic
- do actually use early returns and break a lot

- good to learn for those came before us
- but good to be critical as well
- had their own biases and peculiarities as well

</div>
