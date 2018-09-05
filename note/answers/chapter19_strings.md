## 1

This change is mostly mechanical and not too difficult. First, in the type
itself, we change the last field to use the C99 flexible array member syntax:

```c
struct sObjString {
  Obj obj;
  int length;
  // Was:
  // char* chars;
  // Now:
  char chars[];
};
```

This means that, by default, `chars` is treated as having zero size, but still
of array type. It's up to us to allocate enough memory for the ObjString and
as many trailing bytes as we need. This means the little memory macros don't
work, so we'll manually call `reallocate()`.

First, replace `takeString()` and `copyString()` with:

```c
ObjString* makeString(int length) {
  ObjString* string = (ObjString*)allocateObject(
      sizeof(ObjString) + length + 1, OBJ_STRING);
  string->length = length;
  return string;
}

ObjString* copyString(const char* chars, int length) {
  ObjString* string = makeString(length);

  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';

  return string;
}
```

Now that the character buffer is part of the same allocation as the ObjString,
we can't take ownership of an existing character array. Instead, we need to
create the ObjString that the characters will be copied into.

The `makeString()` function allocates an ObjString with as many extra bytes at
the end as the string needs. It also sets the length, but doesn't initialize
the characters.

`copyString()` uses that to make a new string and copy in the given characters.
That's what string literals do. For concatenation, we do:

```c
static void concatenate() {
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());

  int length = a->length + b->length;
  ObjString* result = makeString(length);
  memcpy(result->chars, a->chars, a->length);
  memcpy(result->chars + a->length, b->chars, b->length);
  result->chars[length] = '\0';

  push(OBJ_VAL(result));
}
```

Instead of creating the character array then the string object, we create the
string object first and then write the concatenated string right into it.

Here's how we free it:

```c
  switch (object->type) {
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      // Was:
      // FREE_ARRAY(char, string->chars, string->length + 1);
      // FREE(ObjString, object);
      // Now:
      reallocate(object, sizeof(ObjString) + string->length + 1, 0);
      break;
    }
  }
```

Note that we include the extra size, but also that now only a single
`reallocate()` call is needed.

## 2

This one's also not too bad. A more efficient solution would be to pack the
"is owned" bit into the type tag or as a bitfield next to the length. Of course,
since this is an optimization, the right way to go about it is to profile some
real-world programs and see if this optimization is worth doing.

But the simple implementation looks like this:

We add a field to the struct to track whether it owns the character array:

```c
struct sObjString {
  Obj obj;
  bool ownsChars; // <--
  int length;
  const char* chars; // <--
};
```

We replace `takeString()` and `copyString()` with:

```c
ObjString* makeString(bool ownsChars, char* chars, int length) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->ownsChars = ownsChars;
  string->length = length;
  string->chars = chars;

  return string;
}
```

When we create a string from a literal, we call `makeString()` and have it not
own the characters:

```c
static void string() {
  emitConstant(OBJ_VAL(makeString(false,
      (char*)parser.previous.start + 1, parser.previous.length - 2)));
}
```

And when we concatenate, it does:

```c
static void concatenate() {
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = makeString(true, chars, length); // <--
  push(OBJ_VAL(result));
}
```

We also need to fix `printObject()` since we can't assume strings are terminated
anymore:

```c
void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      // Changed:
      printf("%.*s", AS_STRING(value)->length, AS_CSTRING(value));
      break;
  }
}
```

Finally, when we free a string, we only free the character array if we own it:

```c
static void freeObject(Obj* object) {
  switch (object->type) {
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      if (string->ownsChars) { // <--
        FREE_ARRAY(char, (char*)string->chars, string->length + 1);
      }
      FREE(ObjString, object);
      break;
    }
  }
}
```

## 3

My preference depends on the semantics of dispatching the "+" operator. My
general goals are:

* Do convert the other operand to a string and then concatenate when possible.
* Try to maintain symmetry of the operator.

In some languages, these two goals are in conflict.

In C++, you can do it by defining `+` to take two strings. Then, any type that
wants to allow itself to be a concatenated operand defines an implicit
conversion to string. This works whether the operand is on the left or right.

C# has similar behavior, but built in. If one operand of `+` is a string, the
other is converted to a string by calling `ToString()` on it and the results are
concatenated. I think that works fine.

In languages like Smalltalk where `+` is a method dynamically dispatched on the
left-hand operand, it's harder to make the behavior symmetric. It's easy to
define a `+` method on string that converts the right-hand operand to a string.
But it's harder to define a `+` on all types that converts the receiver to a
string if the right operand is a string.

In that case, I'm not as thrilled about overloading `+` to mean concatenation
and might prefer a different operator. (In Smalltalk, that operator is `,`.)

At a higher level, while I like `+` for concatenation because it's familiar, I
don't think it's a great way to build strings out of parts. I *much* prefer
having string interpolation built into the language.
