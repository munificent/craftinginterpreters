## Person

Figuring out when to use "we" versus "it" when talking about the code is hard.
It's important to be clear because the prose talks about what the reader needs
to do "define this method", "replace this line", etc. and what the code needs
to do while it's running "match this token", etc.

But it gets really awkward to always use "it" for describing what the code does.
So the rough rules are:

1.  When walking through a hypothetical execution of the code, use "we". Most
    prose explaining the code is like this.

2.  When describing how the code must be changed, what the reader must
    mechanically do, use "we" (and not "you").

3.  When describing how a piece of code works in general, or if it otherwise
    reads better, use "it".

## Formatting

*   Class names are not in code font: "The PrettyPrinter class". Type names in C
    are also formatted normally: Value, Obj, etc. Even built-in types like
    double and uint16_t.

*   File names and extensions are quoted:

    > The file "Expr.java" has extension ".java".

*   C module names are quoted:

    > The "debug" module.

TODO: How do we style keywords used in headers and subheaders?

### Bold and italics

*   The first time a technical term is defined, make it bold. Don't quote it,
    even when referring to the term directly.

*   Consider using italics for a new technical term that isn't explicitly
    defined in order to highlight that it is jargon.

*   In a bullet list, if the bold part is a sentence or part of a sentence,
    emphasize it like normal prose. If the bullet item starts with a standalone
    term, separate it from the subsequent prose with an en dash.

*   Big-O notation: "*O(n)*".

### Code font

*   References to statements like "`if` statement" and "`switch`". Use "`else`
    clause" to refer to that part of an `if` statement's *sytax*, but "then
    branch" and "else branch" to refer to those *concepts*.

*   Use "`return` statement", but "early return". In almost all other cases,
    "return" uses normal type ("return value", "return from", etc.), except when
    "the `return`" refers to a return statement.

*   "Class declaration", but "`class` statement".

*   When referring to the Boolean values true and false, put them in code font,
    as in "returns `true`". Use normal text when referring to truth or falsehood
    in general.

*   Opcodes: "`OP_RETURN`".

*   `nil`, `null` (Java), and `NULL` (C). Simply "null" when used as a verb as
    in "null out the field".

## Punctuation

*   Prose before a Java or C code snippet ends in `:` if the last sentence is
    not a complete sentence or directly refers to the subsequent code. End in
    `.` if it is a reasonable-sounding sentence on its own. This is mainly so
    that we don't use a gratuitous amount of `:` at the end of nearly every
    paragraph.

*   On the other hand, prose before illustrations, Lox examples, and grammar
    snippets can use `:` even when a complete sentence, if the sentence refers
    to the subsequent code or picture.

### Hyphenation

*   If part of a word is emphasized, like "*re*-define", hyphenate at the point
    where the italics change.

*   Hyphenate "left-hand side" and "right-hand side".

*   Always hyphenate:

    *   left-associative
    *   right-associative
    *   non-associative
    *   left-recursive
    *   l-value
    *   r-value
    *   finite-state machine

*   Never hyphenate:

    *    left recursion
    *    call stack
    *    call frame (but "CallFrame" when referring to the struct)

*   Hyphenate when preceding a noun, but not otherwise ("A first-class function
    is first class."):

    *    first class
    *    lowest precedence
    *    start up ("start up the interpreter" versus "startup time")

## Usage

*   Numbers in prose are usually spelled as words when there is a single word
    for them: one, eleven, etc. However, numbers that refer to binary digits are
    always 0 or 1.

### Capitalization

*   Follow common usage to determine which acronyms and abbreviations are all
    caps or not. "COBOL", "Fortran", etc.

*   Design pattern names are capitalized when referring to the pattern itself,
    but not code that implements the pattern (unless the code is the name of the
    actual class). As in: "ExpressionVisitor is a visitor class that implements
    the Visitor pattern."

### Word list

*     opcode
*     Boolean
*     lookup
*     I/O
*     "null" when referring to the null byte at the end of a string
