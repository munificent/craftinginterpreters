This is the repo used to build the in-progress book "[Crafting Interpreters][]".

[crafting interpreters]: http://craftinginterpreters.com

If you find an error or have a suggestion for the book, please do file an issue
here. Thank you!

### How the book is built

It's a fairly simple static site generator. The generated HTML is committed in
the repo under `site/`. It is built from a combination of Markdown for prose,
which lives in `book/`, and snippets of code that are weaved in from the Java
and C implementations in `java/` and `c/`.

The script that does all the magic is `util/build.py`.
