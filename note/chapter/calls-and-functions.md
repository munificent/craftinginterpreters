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