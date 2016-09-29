When talking about whether or not strings and numbers can have properties added,
consider what that would mean for equality on them.

--

When talking about bound functions, explain how this is where JavaScript's
confusing "this" behavior comes from.

Explain how in Vox it's important that closurizing does bind "this" because the
body of the method presumes "this" is an instance of the surrounding class, in
particular around how "super" is handled.

--

Talk about but don't bother implementing checks for duplicate method
definitions.
