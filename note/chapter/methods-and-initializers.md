
--

When talking about bound functions, explain how this is where JavaScript's
confusing "this" behavior comes from.

Explain how in Lox it's important that closurizing does bind "this" because the
body of the method presumes "this" is an instance of the surrounding class, in
particular around how "super" is handled.

--

Talk about but don't bother implementing checks for duplicate method
definitions.

--

When implementing methods in clox, only do bound methods first and then
profile. Then introduce invocation instructions to see the speed difference.

--

OP_INVOKE instructions are an optimization to avoid creating OBJ_BOUND_FUNCTIONs
for each call. Here's the benchmark/invocation.vox results:

with OP_INVOKE   0.693169
no OP_INVOKE     3.13613

--

Because "this" is treated like local variable by compiler, closures over "this"
work automatically.
