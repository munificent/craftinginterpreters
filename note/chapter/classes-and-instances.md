--

When talking about instances, talk about shadow classes and other more
efficient ways of storing fields than a hash map.

--

When explaining "this", go through closing over it in an inner function.

--

When talking about bound methods, explain how users generally expect
parenthesizing or hoisting a subexpression to not affect a behavior. Bound
methods ensure that:

    foo.bar(arg);

Means the same as:

    (foo.bar)(arg);

And:

    var bar = foo.bar;
    bar(arg);
