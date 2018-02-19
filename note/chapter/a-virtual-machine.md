--

Talk about how the order of instructions in the bytecode loop affects
performance. Ideally, the most common instructions are at the top. I think
because of icache misses?

--

- art to designing a bytecode
  - fewer lower level instructions keeps interp loop small and fast
  - too low level can waste too much time on dispatch
  - stack based simpler, does more stack churn, more instructions, smaller inst
  - register based more complex, bigger denser inst
  - designing two languages now
  - nice thing is bytecode is impl detail, so free to redesign without breaking
    users

