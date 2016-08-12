#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

import codecs
import glob
import os
import re

JAVA_CHAPTERS = [
  "Framework",
  "Scanning",
  "Representing Code",
  "Parsing Expressions",
  "Evaluating Expressions",
  "Statements and State",
  "Control Flow",
  "Functions",
  "Resolving and Binding",
  "Classes",
  "Inheritance",
  "Reaching the Summit"
]

C_CHAPTERS = [
  "Chunks of Bytecode",
  "A Virtual Machine",
  "Scanning on Demand",
  "Compiling Expressions",
  "Types of Values",
  "Strings",
  "Hash Tables",
  # TODO: Give unique name.
  "Statements",
  "Global Variables",
  "Jumping Forward and Back",

  # For stuff that hasn't been bucketed in a chapter yet.
  "Uhh",
]

LINE_SECTION_PATTERN = re.compile(r'//[>=]=')
BLOCK_SECTION_PATTERN = re.compile(r'/\*[>=]=')

EQUALS_PATTERN = re.compile(r'/[/*]== (.*)')
RANGE_PATTERN = re.compile(r'/[/*]>= (.*) <= (.*)')
MIN_PATTERN = re.compile(r'/[/*]>= (.*)')


def chapter_to_package(chapters, chapter_offset, index):
  name = chapters[index].split()[0].lower()
  if name == "a":
    name = chapters[index].split()[1].lower()
  return "chap{0:02d}_{1}".format(index + chapter_offset, name)


def parse_range(chapters, line):
  match = EQUALS_PATTERN.match(line)
  if match:
    chapter = chapters.index(match.group(1))
    return chapter, chapter

  match = RANGE_PATTERN.match(line)
  if match:
    min_chapter = chapters.index(match.group(1))
    max_chapter = chapters.index(match.group(2))
    return min_chapter, max_chapter

  match = MIN_PATTERN.match(line)
  if match:
    min_chapter = chapters.index(match.group(1))
    return min_chapter, 999

  raise Exception("Invalid line: '" + line + "'")


def split_file(source_dir, chapters, chapter_offset, path, chapter_index):
  relative = os.path.relpath(path, source_dir)
  directory = os.path.dirname(relative)

  # Don't split the generated files.
  if relative == "com/craftinginterpreters/vox/Expr.java": return
  if relative == "com/craftinginterpreters/vox/Stmt.java": return

  min_chapter = 999
  max_chapter = 999

  # Some chunks of code are replaced in later chapters, so they are commented
  # out in the canonical source. This tracks when we are in one of those.
  in_block_comment = False

  # Read the source file and preprocess it.
  output = ""
  with open(path, 'r') as input:
    # Read each line, preprocessing the special codes.
    line_num = 1
    for line in input:
      if LINE_SECTION_PATTERN.match(line):
        min_chapter, max_chapter = parse_range(chapters, line)
      elif BLOCK_SECTION_PATTERN.match(line):
        min_chapter, max_chapter = parse_range(chapters, line)
        in_block_comment = True
      elif in_block_comment and line.strip() == "*/":
        min_chapter = None
        max_chapter = None
        in_block_comment = False
      elif min_chapter == None:
        print "{} {}: No section after block comment".format(relative, line_num)
        min_chapter = 0
        max_chapter = 999
      elif chapter_index >= min_chapter and chapter_index <= max_chapter:
        # Hack. In generate_ast.java, we split up a parameter list among
        # multiple chapters, which leads to hanging commas in some cases.
        # Remove them.
        if line.strip().startswith(")") and output.endswith(",\n"):
          output = output[:-2] + "\n"
        output += line

      line_num += 1

  # Write the output.
  if output:
    package = chapter_to_package(chapters, chapter_offset, chapter_index)
    output_path = os.path.join("gen", package, relative)

    # Don't overwrite it if it didn't change, so the makefile doesn't think it
    # was touched.
    if os.path.exists(output_path):
      with open(output_path, 'r') as file:
        previous = file.read()
        if output == previous: return

    ensure_dir(os.path.join("gen", package, directory))
    with codecs.open(output_path, "w", encoding="utf-8") as out:
      print output_path
      out.write(output)


def ensure_dir(path):
  if not os.path.exists(path):
      os.makedirs(path)


def walk(dir, extensions, callback):
  """
  Walks [dir], and executes [callback] on each file.
  """

  dir = os.path.abspath(dir)
  for path in os.listdir(dir):
    nfile = os.path.join(dir, path)
    if os.path.isdir(nfile):
      walk(nfile, extensions, callback)
    elif os.path.splitext(path)[1] in extensions:
      callback(nfile)


# The Java chapters.
for i, chapter in enumerate(JAVA_CHAPTERS):
  walk("java", [".java"],
      lambda path: split_file("java", JAVA_CHAPTERS, 4, path, i))

# The C chapters.
for i, chapter in enumerate(C_CHAPTERS):
  walk("c", [".c", ".h"],
      lambda path: split_file("c", C_CHAPTERS, 16, path, i))
