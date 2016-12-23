#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import codecs
import glob
import os
import re
import sys

import book
import sections


LINE_SECTION_PATTERN = re.compile(r'//[>=]=')
BLOCK_SECTION_PATTERN = re.compile(r'/\*[>=]=')

EQUALS_PATTERN = re.compile(r'/[/*]== (.*)')
RANGE_PATTERN = re.compile(r'/[/*]>= (.*) (<=?) (.*)')
MIN_PATTERN = re.compile(r'/[/*]>= (.*)')

source_code = sections.load()

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


def split_file(chapter_name, path, section=None):
  chapter_number = book.chapter_number(chapter_name)

  source_dir = book.get_language(chapter_name)
  relative = os.path.relpath(path, source_dir)
  directory = os.path.dirname(relative)

  # Don't split the generated files.
  if relative == "com/craftinginterpreters/lox/Expr.java": return
  if relative == "com/craftinginterpreters/lox/Stmt.java": return

  # If we're generating the split for an entire chapter, include all its sections.
  real_section = section if section != None else 999
  output = source_code.split_chapter(relative, chapter_name, real_section)

  package = "section_test"
  if section == None:
    package = book.get_short_name(chapter_name)
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


def split_chapter(chapter, section=None):
  source_dir = book.get_language(chapter)

  def walk(dir):
    dir = os.path.abspath(dir)
    for path in os.listdir(dir):
      nfile = os.path.join(dir, path)
      if os.path.isdir(nfile):
        walk(nfile)
      elif os.path.splitext(path)[1] in [".java", ".h", ".c"]:
        split_file(chapter, nfile, section)

  walk(source_dir)


if len(sys.argv) == 3:
  # Generate the code at a single section.
  chapter = sys.argv[1]
  section = int(sys.argv[2])

  split_chapter(chapter, section)
else:
  for chapter in book.CODE_CHAPTERS:
    # TODO: Uncomment this to split out each individual section.
    # TODO: Need to also pass section to chapter_to_package() to generate
    # directory name.
    # code_sections = source_code.find_all(chapter)
    # for section in code_sections:
    #   split_chapter(chapter, section)
    split_chapter(chapter)
