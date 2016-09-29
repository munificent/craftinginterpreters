--

When explaining IS_XXX() and AS_XXX() macros, talk about how the AS ones do
not check the value and we're responsible for calling the IS one first unless
we are sure it's the right type. If we had a static type system, would not
need the IS checks at runtime.

--

When implementing valueEquals() discuss using memcmp() to compare the unions
and how that doesn't work because of padding bytes, etc.
