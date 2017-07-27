1.  Both of them have significant indentation. To handle that, the scanner
    emits synthetic "{" and "}" tokens (or "indent" and "dedent" as Python
    calls them), as if there were explicit delimiters for each level of
    indentation.

    In order to know when a new line begins or ends one of more levels of
    indentation, the scanner has to track the *previous* indentation value.
    That state has to be stored in the scanner, which means it has a little bit
    of *memory*. That makes it no longer a regular language, which is defined
    to only need to store a single finite number identifying which state it's
    in.

    You *could* make a regular language for significant indentation, by having
    a hardcoded limit to the maximum amount of indentation, but that starts to
    split semantic hairs around the Chomsky hierarchy.

2.  In CoffeeScript, parentheses are option in function calls. You can call a
    function like:

    ```coffeescript
    doStuff withThis
    ```

    Also, there is a nice syntax for lambda functions:

    ```coffeescript
    () -> someLambda
    -> anotherOne
    ```

    On the second line, you can see that you can omit the `()` if the lambda
    takes no parameters. So what does this do:

    ```coffeescript
    someFunction () -> someLambda
    ```

    Does it call `someFunction` with zero parameters and then call the result of
    *that* with one parameter, a lambda? Or does it call `someFunction` with
    one parameter, the lambda? The answer depends on spaces:

    ```coffeescript
    someFunction() -> someLambda
    # Means the same as:
    someFunction()(() -> someLambda)

    someFunction () -> someLambda
    # Means the same as:
    someFunction(() -> someLambda)
    ```

    Ruby has similar corner cases because it also allow omitting the parentheses
    in method calls (which is where CoffeeScript gets it from).

    The C preprocessor relies on spaces to distinguish function macros from
    simple macros:

    ```c
    #define MACRO1 (p) (p)
    #define MACRO2(p) (p)
    ```

    Here, `MACRO1` is a simple text macro that expands to `(p) (p)` when used.
    `MACRO2(p)` is a function-like macro that takes a parameter and expands to
    `(p)` with `p` replaced by the parameter.

3.  Programmers often write "doc comments" above their functions and types. A
    documentation generator or an IDE that shows help text for declarations
    needs access to those comments, so a scanner for those should include them.

    An automated code formatter obviously needs to preserve comments and may
    want to be aware of the original whitespace if some of the author's
    formatting should be preserved.

4.  You can see where I've implemented them for a similar language here:

    https://github.com/munificent/wren/blob/c6eb0be99014d34085e2d24c696aed449e2fb171/src/vm/wren_compiler.c#L663

    The interesting part is the `nesting` variable. Like challenge #1, we
    require some extra state to track the nesting, which makes this not quite
    regular.

    Note also that we need to handle an unterminated block comment.


