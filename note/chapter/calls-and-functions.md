When talking about stack overflow detection, consider a few approaches:

- Ensure there are as many stack slots as possibly needed for each call frame.
  Then we just need to detect call frame overflow. Can use a lot of stack space.
  Simple and fast.

- Check on each push. Simple but really slow.

- Calculate max stack for each function. Good compromise. Little tedious to
  calculate in compiler.

--

https://en.wikipedia.org/wiki/Function_prologue

--

When explaining call frames, talk about how in early Fortran, without recursion,
the ip for each function could be stored as static data with the function.

--

Talk about combining the CallFrame stack with the value stack.

--

old comment on depth field in Local:

// The depth in the scope chain that this variable was declared at.
// Zero is the outermost scope--parameters for a method, or the first
// local block in top level code. One is the scope within that, etc.

--

storing compiler structs on stack means deep nesting could be problem. deep
nesting also problem with recursive descent in general. may be good to set
max depth and check on recursive calls.
