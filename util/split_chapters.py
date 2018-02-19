#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import codecs
import glob
import os
import re
import sys

import book
import code_snippets


source_code = code_snippets.load()


def split_file(chapter_name, path, snippet=None, index=None):
  chapter_number = book.chapter_number(chapter_name)

  source_dir = book.get_language(chapter_name)
  relative = os.path.relpath(path, source_dir)
  directory = os.path.dirname(relative)

  # Don't split the generated files.
  if relative == "com/craftinginterpreters/lox/Expr.java": return
  if relative == "com/craftinginterpreters/lox/Stmt.java": return

  package = book.get_short_name(chapter_name)
  if snippet:
    package = os.path.join("snippets", package, "{:02}-{}".format(index, snippet))
  output_path = os.path.join("gen", package, relative)

  # If we're generating the split for an entire chapter, include all its
  # snippets.
  if not snippet:
    snippet = source_code.last_snippet_for_chapter(chapter_name).name

  output = source_code.split_chapter(relative, chapter_name, snippet)

  if output:
    # Don't overwrite it if it didn't change, so the makefile doesn't think it
    # was touched.
    if os.path.exists(output_path):
      with open(output_path, 'r') as file:
        previous = file.read()
        if output == previous: return

    # Write the changed output.
    ensure_dir(os.path.join("gen", package, directory))
    with codecs.open(output_path, "w", encoding="utf-8") as out:
      print(output_path)
      out.write(output)
  else:
    # Remove it if it's supposed to be nonexistent.
    if os.path.exists(output_path):
      os.remove(output_path)


def ensure_dir(path):
  if not os.path.exists(path):
      os.makedirs(path)


def split_chapter(chapter, snippet=None, index=None):
  source_dir = book.get_language(chapter)

  def walk(dir):
    dir = os.path.abspath(dir)
    for path in os.listdir(dir):
      nfile = os.path.join(dir, path)
      if os.path.isdir(nfile):
        walk(nfile)
      elif os.path.splitext(path)[1] in [".java", ".h", ".c"]:
        split_file(chapter, nfile, snippet, index)

  walk(source_dir)


if len(sys.argv) == 3:
  # Generate the code at a single snippet.
  chapter = sys.argv[1]
  snippet = int(sys.argv[2])

  split_chapter(chapter, snippet)
else:
  for chapter in book.CODE_CHAPTERS:
    if "Appendix" in chapter:
      # Appendices are "code chapters" because they include snippets, but don't
      # need to be split out and run.
      continue
    # TODO: Uncomment this to split out the chapters at each snippet.
    # TODO: Need to also pass snippet to chapter_to_package() to generate
    # directory name.
    # snippets = source_code.find_all(chapter)
    # index = 1
    # for snippet in snippets.values():
    #   split_chapter(chapter, snippet.name, index)
    #   index += 1
    split_chapter(chapter)
