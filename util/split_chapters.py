#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import codecs
import glob
import os
import re

import sections

JAVA_CHAPTERS = [
  "Scanning",
  "Representing Code",
  "Parsing Expressions",
  "Evaluating Expressions",
  "Statements and State",
  "Control Flow",
  "Functions",
  "Resolving and Binding",
  "Classes",
  "Inheritance"
]

C_CHAPTERS = [
  "Chunks of Bytecode",
  "A Virtual Machine",
  "Scanning on Demand",
  "Compiling Expressions",
  "Types of Values",
  "Strings",
  "Hash Tables",
  "Global Variables",
  "Local Variables",
  "Jumping Forward and Back",
  "Calls and Functions",
  "Closures",
  "Garbage Collection",
  "Classes and Instances",
  "Methods and Initializers",
  "Superclasses",
  "Optimization"
]

LINE_SECTION_PATTERN = re.compile(r'//[>=]=')
BLOCK_SECTION_PATTERN = re.compile(r'/\*[>=]=')

EQUALS_PATTERN = re.compile(r'/[/*]== (.*)')
RANGE_PATTERN = re.compile(r'/[/*]>= (.*) (<=?) (.*)')
MIN_PATTERN = re.compile(r'/[/*]>= (.*)')


source_code = sections.load()

def chapter_to_package(chapters, chapter_offset, index):
  name = chapters[index].split()[0].lower().replace(',', '')
  if name == "a" or name == "the":
    name = chapters[index].split()[1].lower()
  if name == "user-defined":
    name = "user"
  return "chap{0:02d}_{1}".format(index + chapter_offset, name)


def parse_range(chapters, line):
  match = EQUALS_PATTERN.match(line)
  if match:
    chapter = chapters.index(match.group(1))
    return chapter, chapter

  match = RANGE_PATTERN.match(line)
  if match:
    min_chapter = chapters.index(match.group(1))
    operator = match.group(2)
    max_chapter = chapters.index(match.group(3))
    if operator == '<': max_chapter -= 1
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
  if relative == "com/craftinginterpreters/lox/Expr.java": return
  if relative == "com/craftinginterpreters/lox/Stmt.java": return

  output = source_code.at_chapter(relative, chapters[chapter_index])

  package = chapter_to_package(chapters, chapter_offset, chapter_index)
  output_path = os.path.join("gen", package, relative)

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
      lambda path: split_file("c", C_CHAPTERS, 14, path, i))
