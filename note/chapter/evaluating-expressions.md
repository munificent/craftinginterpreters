When talking about comparison operators, talk about how allowing comparison on
non-numeric types can be handy for polymorphic sorted collections.

--

Regarding error handling: a language should either give you enough features to
let you prevent an error, or to handle it. For example, in Vox, there's no way
to check the type of an object, but also you get a runtime error that you can't
catch if you pass an object of the wrong type to an operator. That's bad.

--

We allow "+" to mix strings and other types because there's no other way to
stringify.
