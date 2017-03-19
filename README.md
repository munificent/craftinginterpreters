This is the repo used for the in-progress book "[Crafting Interpreters][]". It
contains the Markdown text of the book, full implementations of both
interpreters, as well as the build system to weave the two together into the
final site.

[crafting interpreters]: http://craftinginterpreters.com

If you find an error or have a suggestion, please do file an issue here. Thank
you!

## Implementations in Other Languages

Some brave souls have ported (or are porting, depending on when you read this)
the interpreters in the book to other languages. If Java and C aren't your bag,
take a look at:

* **C++:** [ikatanic/lox-cpp](https://github.com/ikatanic/lox-cpp)
* **C#:** [ByronMayne/LoxSharp](https://github.com/ByronMayne/LoxSharp),
    [tslater2006/Gravlox](https://github.com/tslater2006/Gravlox)
* **Ceylon:** [leomindez/celox](https://github.com/leomindez/celox)
* **Go:** [paulja/glox](https://github.com/paulja/glox)
* **JavaScript:** [brandly/lox.js](https://github.com/brandly/lox.js)
* **Kotlin:** [GraydenH/klox](https://github.com/GraydenH/klox)
* **Lua:** [ryanplusplus/llox](https://github.com/ryanplusplus/llox)
* **Python:** [selectnull/pylox](https://github.com/selectnull/pylox)
* **Rust:** [rodaine/rlox](https://github.com/rodaine/rlox),
    [HarveyHunt/loxr](https://github.com/HarveyHunt/loxr),
    [mariosangiorgio/rulox](https://github.com/mariosangiorgio/rulox)
* **SPARK:** [aeszter/lox-spark](https://github.com/aeszter/lox-spark)
* **Swift:** [alexito4/slox](https://github.com/alexito4/slox)
* **Wren:** [CodogoFreddie/wlox](https://github.com/CodogoFreddie/wlox)

Are you porting jlox or clox to another language? Let me know and I'll add you
to this list.

## Contributing

One of the absolute best things about writing a book online and putting it out
there before it's done is that people like you have been kind enough to give me
feedback, point out typos, and find other errors or unclear text.

If you'd like to do that, great! You can just file bugs here on the repo, or
send a pull request if you're so inclined. If you want to send a pull request,
but don't want to get the build system set up to regenerate the HTML too, don't
worry about it. I'll do that when I pull it in.

## Building Stuff

I am a terribly forgetful, error-prone mammal, so I automated as much as I
could.

### Prerequisites

I develop on an OS X machine, but any POSIX system should work too. With a
little extra effort, you should be able to get this working on Windows as well,
though I can't help you out much.

Most of the work is orchestrated by make. The build scripts, test runner, and
other utilities are all written in Python 3. The makefile assumes `python3` is
on your PATH.

You'll need to install a few Python packages:

```sh
$ pip3 install markdown jinja2 pygments
```

The makefile also assumes Ruby (in particular `gem`) is on your PATH. You'll
need to install this gem:

```sh
$ gem install sass
```

In order to get syntax highlighting for Lox itself working, you need to plug in
its custom Pygments lexer:

```sh
$ python3 util/pygments/setup.py develop
```

In order to compile the two interpreters, you need some C compiler on your path
as well as `javac`.

### Building

Once you've got that setup, try:

```sh
$ make
```

If everything is working, that will generate the site for the book as well as
compiling the two interpreters clox and jlox. You can run either interpreter
right from the root of the repo:

```sh
$ clox
$ jlox
```

### Hacking on the book

The Markdown and snippets of source code are weaved together into the final
HTML using a hand-written little static site generator, `util/build.py`. It's
a fairly simple static site generator. The generated HTML is committed in
the repo under `site/`. It is built from a combination of Markdown for prose,
which lives in `book/`, and snippets of code that are weaved in from the Java
and C implementations in `java/` and `c/`. (All of those funny looking comments
in the source code are how it knows which snippet goes where.)

The script that does all the magic is `util/build.py`. You can run that
directly, or run:

```sh
$ make book
```

That generates the entire site in one batch. If you are incrementally working
on it, you'll want to run the development server:

```sh
$ make serve
```

This runs a little HTTP server on localhost rooted at the `site/` directory.
Any time you request a page, it regenerates any files whose sources have been
changed, including Markdown files, interpreter source files, templates, and
assets. Just let that keep running, edit files locally, and refresh your
browser to see the changes.

### Building the interpreters

You can build each interpreter like so:

```sh
$ make clox
$ make jlox
```

This builds the final version of each interpreter as it appears at the end of
its part in the book.

You can also see what the interpreters look like at the end of each chapter.
(I use this to make sure they are working even in the middle of the book.) This
is driven by a script, `util/split_chapters.py` that uses the same comment
markers for the code snippets to determine which chunks of code are present in
each chapter. It takes only the snippets that have been seen by the end of each
chapter and produces a new copy of the source in `gen/`, one directory for
each chapter's code. (These are also an easier way to view the source code
since they have all of the distracting marker comments stripped out.)

Then, each of those can be built separately. Run:

```sh
$ make c_chapters
```

And in the `build/` directory, you'll get an executable for each chapter, like
`chap14_chunks`, etc. Likewise:

```sh
$ make java_chapters
```

This compiles the Java code to classfiles in `build/gen/` in a subdirectory for
each chapter.

## Repository Layout

*   `asset/` – Sass files and jinja2 templates used to generate the site.
*   `book/` - Markdown files for the text of each chapter.
*   `build/` - Intermediate files and other build output (except for the site)
    itself go here. Not committed to Git.
*   `c/` – Source code of clox, the interpreter written in C. Also contains an
    XCode project, if that's your thing.
*   `gen/` – Java source files generated by GenerateAst.java go here. Not
    committed.
*   `java/` – Source code of jlox, the interpreter written in Java.
*   `note/` – Various research, notes, TODOs, and other miscellanea.
*   `note/answers` – Sample answers for the challenges. No cheating!
*   `site/` – The final generated site. The contents of this directory directly
    mirror craftinginterpreters.com. Most content here is generated by build.py,
    but fonts, images, and JS only live here. Everything is committed, even the
    generated content.
*   `test/` – Test cases for the Lox implementations.
*   `util/` – Tools and build scripts. The test runner and build system that
    generate the site from the Markdown and source files live here.
