#!./util/env/bin/python3
# -*- coding: utf-8 -*-

# Tests that various snippets in the middle of chapters can be compiled without
# error. Ensures that, as much as possible, we have a working program at
# multiple points throughout the chapter.

# TODO: Do this for Java chapters.

import os
from subprocess import Popen, PIPE
import sys

import book
import code_snippets
import split_chapters
import term

source_code = code_snippets.load()

chapter_snippets = {
  "Chunks of Bytecode": [
    "free-array",
    "main-include-chunk",
    "simple-instruction",
    "add-constant",
    "return-after-operand",
  ],
  "A Virtual Machine": [
    "main-include-vm",
    "vm-include-debug",
    "print-return",
    "main-negate",
  ],
  "Scanning on Demand": [
    "init-scanner",
    "error-token",
    "advance",
    "match",
    "newline",
    "peek-next",
    "string",
    "number",
    "identifier-type",
    "check-keyword",
  ],
  "Compiling Expressions": [
    "expression",
    "forward-declarations",
    "precedence-body",
    "infix",
    "define-debug-print-code",
    "dump-chunk"
  ],
  "Types of Values": [
    "op-arithmetic",
    "print-value",
    "disassemble-not",
    "values-equal",
  ],
  "Strings": [
    # "as-string",
    # We could get things working earlier by moving the "Operations on Strings"
    # section before "Strings".
    "value-include-object",
    "vm-include-object-memory",
  ],
  "Hash Tables": [
    "free-table",
    "hash-string",
    "table-add-all",
    "table-get",
    "table-delete",
    "resize-increment-count",
  ],
  "Global Variables": [
    "disassemble-print",
    "disassemble-pop",
    "synchronize",
    "define-global-op",
    "disassemble-define-global",
    "disassemble-get-global",
    "disassemble-set-global",
  ],
  "Local Variables": [
    "local-struct",
    "compiler",
    "end-scope",
    "add-local",
    "too-many-locals",
    "pop-locals",
    "interpret-set-local",
  ],
  "Jumping Back and Forth": [
    "jump-if-false-op",
    "compile-else",
    "jump-op",
    "pop-end",
    "jump-instruction",
    "and",
    "or",
    "while-statement",
    "loop-op",
    "disassemble-loop",
    "for-statement",
  ],
  "Calls and Functions": [
    "as-function",
    "function-type-enum",
    "init-compiler",
    "init-function-slot",
    "return-function",
    "disassemble-end",

    "runtime-error-temp",
    "compile-function",
    "init-function-name",
    "call",
    "interpret",
    "disassemble-call",
    "return-statement",
    "runtime-error-stack",
    "return-from-script",
    "print-native",
    "define-native",
    "vm-include-time"
  ],
  "Closures": [],
  "Garbage Collection": [],
  "Classes and Instances": [],
  "Methods and Initializers": [],
  "Superclasses": [],
  "Optimization": [],
}

all_passed = True

for chapter, snippets in chapter_snippets.items():
  chapter_dir = book.get_short_name(chapter)

  if not snippets:
    print("Warning, no in-chapter snippets for '{}'".format(chapter))
    snippets = source_code.snippet_tags[chapter].keys()

  for snippet_name in snippets:
    snippet = source_code.snippet_tags[chapter][snippet_name]
    split_chapters.split_chapter(chapter, snippet)

    build_name = "{}-{:02}-{}".format(chapter_dir, snippet.index, snippet_name)
    snippet_dir = "{:02}-{}".format(snippet.index, snippet_name)
    source_dir = os.path.join("gen", "snippets", chapter_dir, snippet_dir)

    args = [
      "make",
      "-f", "util/c.make",
      "NAME=" + build_name,
      "MODE=release",
      "SOURCE_DIR=" + source_dir,
      "SNIPPET=true"
    ]
    proc = Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    out, err = proc.communicate()

    if proc.returncode != 0:
      print("{} {} / {}".format(term.red("FAIL"), chapter, snippet_name))
      print(out.decode('utf-8'))
      print(err.decode('utf-8'))
      print()
      all_passed = False
    else:
      print("{} {} / {}".format(term.green("PASS"), chapter, snippet_name))

if not all_passed:
  sys.exit(1)
