#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

import codecs
import glob
import os
import re

CHAPTERS = [
  "Framework",
  "Scanning",
  "Syntax Trees",
  "Parsing Expressions",
  "Interpreting ASTs",
  "Variables",
  "Functions",
  "Closures",
  "Control Flow",
  "Classes",
  "Inheritance",
  # TODO: Figure out what to do with this stuff.
  "Uhh"
]

LINE_SECTION_PATTERN = re.compile(r'//[>=]=')
BLOCK_SECTION_PATTERN = re.compile(r'/\*[>=]=')

EQUALS_PATTERN = re.compile(r'/[/*]== (.*)')
RANGE_PATTERN = re.compile(r'/[/*]>= (.*) <= (.*)')
MIN_PATTERN = re.compile(r'/[/*]>= (.*)')


def chapter_to_package(index):
  name = CHAPTERS[index].split()[0].lower()
  return "chap{0:02d}_{1}".format(index + 1, name)


def parse_range(line):
  match = EQUALS_PATTERN.match(line)
  if match:
    chapter = CHAPTERS.index(match.group(1))
    return chapter, chapter

  match = RANGE_PATTERN.match(line)
  if match:
    min_chapter = CHAPTERS.index(match.group(1))
    max_chapter = CHAPTERS.index(match.group(2))
    return min_chapter, max_chapter

  match = MIN_PATTERN.match(line)
  if match:
    min_chapter = CHAPTERS.index(match.group(1))
    return min_chapter, 999

  raise Exception("Invalid line: '" + line + "'")


def split_file(path, chapter_index):
  relative = os.path.relpath(path, "java")
  directory = os.path.dirname(relative)

  # Don't split the generated files.
  if relative == "com/craftinginterpreters/vox/Expr.java": return
  if relative == "com/craftinginterpreters/vox/Stmt.java": return

  min_chapter = 0
  max_chapter = 999

  # Some chunks of code are replaced in later chapters, so they are commented
  # out in the canonical source. This tracks when we are in one of those.
  in_block_comment = False

  # Read the source file and preprocess it.
  output = ""
  with open(path, 'r') as input:
    # Read each line, preprocessing the special codes.
    for line in input:
      if LINE_SECTION_PATTERN.match(line):
        min_chapter, max_chapter = parse_range(line)
      elif BLOCK_SECTION_PATTERN.match(line):
        min_chapter, max_chapter = parse_range(line)
        in_block_comment = True
      elif in_block_comment and line.strip() == "*/":
        in_block_comment = False
      elif chapter_index >= min_chapter and chapter_index <= max_chapter:
        # Hack. In generate_ast.java, we split up a parameter list among
        # multiple chapters, which leads to hanging commas in some cases.
        # Remove them.
        if line.strip().startswith(")") and output.endswith(",\n"):
          output = output[:-2] + "\n"
        output += line

  # Write the output (if this file exists for this chapter).
  if output:
    package = chapter_to_package(chapter_index)
    ensure_dir(os.path.join("gen", package, directory))

    output_path = os.path.join("gen", package, relative)
    with codecs.open(output_path, "w", encoding="utf-8") as out:
      out.write(output)


def ensure_dir(path):
  if not os.path.exists(path):
      os.makedirs(path)


def walk(dir, extension, callback):
  """
  Walks [dir], and executes [callback] on each file.
  """

  dir = os.path.abspath(dir)
  for path in os.listdir(dir):
    nfile = os.path.join(dir, path)
    if os.path.isdir(nfile):
      walk(nfile, extension, callback)
    elif os.path.splitext(path)[1] == extension:
      callback(nfile)


for i, chapter in enumerate(CHAPTERS):
  print i + 1, chapter

  walk("java", ".java", lambda path: split_file(path, i))
