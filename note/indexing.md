## Acronyms

The expanded form gets most of the locators. The acronym gets a "See" cross-ref
to the expanded form:

    National Basketball Association 3, 5, 7, 9
    NBA See National Basketball Association

If the prose explains the acronym, the acronym also gets a locator.

## Subheadings

Subheadings are for attributes of the main heading, not a taxonomic grouping.
I wouldn't do:

    animals
      mammals
        bats

Nor:

    literal
      number
        decimal
        integer

Remember, indexes are mostly for looking things up by name, not defining a
hierarchy.

In cases where there are subtopics related to a main topic, prefer flattening
the subtopics and putting the locators there and then add "See also" cross-refs
to the main topic:

    double-precision 4, 5, 6
    floating point 1, 2, 3
    number 6, 7
      See also double-precision
      See also floating point

## Double-posting

Don't. Not because it's not useful but because it's too much of a headache.
Instead, use cross-links.

If the synonym is defined in the book, add an entry for the page and a separate
"See [also]" link for the main term. If the synonym is not defined in the book,
use a "See" link.

## Languages

Include references to programming languages. Prefer giving them subtopics since
there are so many.

Don't link to C and Java just because they are the implementation languages for
clox and jlox. Only mention them when there is something interesting about those
languages in particular.

## clox and jlox

Don't bother distinguishing entries by jlox and clox, like "error handling, in
jlox". The reader can tell pretty easily from the page number which interpreter
an entry is for.

## Other stuff

*   When indexing a design pattern, do "<Name> design pattern". Note the pattern
    name is capitalized.

*   Add entries for jlox classes named "<name> class". (Probably link the
    generated AST classes to the appendix.)

*   Add entries for jlox interfaces named "<name> interface".

*   Add entries for jlox and clox enum types named "<name> enum".

*   Add entries for clox struct named "<name> struct".

*   Add entries for each clox opcode. Link to the place where the opcode itself
    is defined in the enum.

*   Don't add entries for methods and functions.

## TODO at end

*   Go through and make sure I caught all the classes, structs, and enums that
    should have entries.

*   Make sure all opcodes have entries.

*   For topics with a lot of page numbers (like most language names), go through
    and see which ones can have subtopics. Or just remove some of them if they
    don't add value.

*   Look for topics that should be collapsed like "dynamic typing" and "dynamic
    types".
