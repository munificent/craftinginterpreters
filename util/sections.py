#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""

Parses the Java and C source files and separates out their sections. This is
used by both split_chapters.py (to make the chapter-specific source files to
test against) and build.py (to include code sections into the book).

There are a few kinds of section markers:

//> [chapter] [number]

    This marks the following code as being added in snippet [number] in
    [chapter]. A section marked like this must be closed with...

    If this is beginning a new section in the same chapter, the name is omitted.

//< [chapter] [number]

    Ends the previous innermost //> section and returns the whatever section
    surrounded it. The chapter and number are redundant, but are required to
    validate that we're exiting the section we intend to.

/* [chapter] [number] < [end chapter] [end number]
...
*/

    This marks the code in the rest of the block comment as being added in
    section [number] in [chapter]. It is then replaced or removed in section
    [end number] in [end chapter].

    Since this section doesn't end up in the final version of the code, it's
    commented out in the source.

    After the block comment, this returns to the previous section, if any.

"""

import os
import re
import sys

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

BLOCK_PATTERN = re.compile(r'/\* ([A-Za-z\s]+) (\d+) < ([A-Za-z\s]+) (\d+)')
BEGIN_SECTION_PATTERN = re.compile(r'//> (\d+)')
END_SECTION_PATTERN = re.compile(r'//< (\d+)')
BEGIN_CHAPTER_PATTERN = re.compile(r'//> ([A-Za-z\s]+) (\d+)')
END_CHAPTER_PATTERN = re.compile(r'//< ([A-Za-z\s]+) (\d+)')

class SourceCode:
  """ All of the source files in the book. """

  def __init__(self):
    self.files = []

    # The chapter/number pairs of every parsed section. Used to ensure we don't
    # try to create the same section twice.
    self.all_sections = {}


  def find_all(self, chapter):
    """ Gets the list of sections that occur in [chapter]. """
    sections = {}

    # TODO: If this is slow, could organize directly by sections in SourceCode.
    for file in self.files:
      for line in file.lines:
        if line.chapter == chapter:
          if not line.number in sections:
            sections[line.number] = Section(file)
          # TODO: Enable this check once we can.
          # else if sections[line.number].file.path != file.path:
          #   raise "{} {} appears in two files, {} and {}".format(
          #       chapter, line.number, sections[line.number].file.path, file.path)
          sections[line.number].added.append(line.text)
        elif line.end_chapter == chapter:
          if not line.end_number in sections:
            sections[line.end_number] = Section(file)
          # TODO: Enable this check once we can.
          # else if sections[line.number].file.path != file.path:
          #   raise "{} {} appears in two files, {} and {}".format(
          #       chapter, line.number, sections[line.number].file.path, file.path)
          sections[line.end_number].removed.append(line.text)

    # TODO: Check for discontiguous sections that are interrupted by earlier
    # chapters.
    return sections

  def split_chapter(self, file, chapter, number):
    """ Gets the code for [file] as it appears at [number] of [chapter]. """
    index = get_chapter_index(chapter)

    source_file = None
    for source in self.files:
      if source.path == file:
        source_file = source
        break

    if source_file == None:
      raise Exception('Could not find file "{}"'.format(file))

    output = ""
    for line in source_file.lines:
      if (line.is_present(index, number)):
        # Hack. In generate_ast.java, we split up a parameter list among
        # multiple chapters, which leads to hanging commas in some cases.
        # Remove them.
        if line.text.strip().startswith(")") and output.endswith(",\n"):
          output = output[:-2] + "\n"

        output += line.text + "\n"
    return output


class SourceFile:
  def __init__(self, path):
    self.path = path
    self.lines = []

  def language(self):
    return 'java' if self.path.endswith('java') else 'c'

  def nice_path(self):
    return self.path.replace('com/craftinginterpreters/', '')


class SourceLine:
  def __init__(self, text, chapter, number, end_chapter, end_number):
    self.text = text
    self.chapter = chapter
    self.number = number
    self.end_chapter = end_chapter
    self.end_number = end_number

  def chapter_index(self):
    return get_chapter_index(self.chapter)

  def end_chapter_index(self):
    return get_chapter_index(self.end_chapter)

  def is_present(self, chapter_index, section_number):
    """ If this line exists by [section_number] of [chapter_index]. """
    if chapter_index < self.chapter_index():
      # We haven't gotten to its chapter yet.
      return False
    elif chapter_index == self.chapter_index():
      if section_number < self.number:
        # We haven't reached this section yet.
        return False

    if self.end_chapter != None:
      if chapter_index > self.end_chapter_index():
        # We are past the chapter where it is removed.
        return False
      elif chapter_index == self.end_chapter_index():
        if section_number > self.end_number:
          # We are past this section where it is removed.
          return False

    return True


class Section:
  def __init__(self, file):
    self.file = file
    self.added = []
    self.removed = []


class ParseState:
  def __init__(self, parent, chapter, number, end_chapter=None, end_number=None):
    self.parent = parent
    self.chapter = chapter
    self.number = number
    self.end_chapter = end_chapter
    self.end_number = end_number

def chapter_name(number):
  """Given a chapter number, returns its name."""
  if number < 14:
    return JAVA_CHAPTERS[number - 4]

  return C_CHAPTERS[number - 14]


def get_chapter_index(name):
  """Given the name of a chapter, finds its number."""
  if name in JAVA_CHAPTERS:
    return 4 + JAVA_CHAPTERS.index(name)

  if name in C_CHAPTERS:
    return 14 + C_CHAPTERS.index(name)

  raise Exception('Unknown chapter "{}".'.format(name))


def load_file(source_code, source_dir, path):
  relative = os.path.relpath(path, source_dir)

  # Don't process the generated files. We only worry about GenerateAst.java.
  if relative == "com/craftinginterpreters/lox/Expr.java": return
  if relative == "com/craftinginterpreters/lox/Stmt.java": return

  file = SourceFile(relative)
  source_code.files.append(file)

  line_num = 1
  state = ParseState(None, None, None)
  handled = False

  def fail(message):
    print("{} line {}: {}".format(relative, line_num, message), file=sys.stderr)
    sys.exit(1)

  def push(chapter, number, end_chapter=None, end_number=None):
    nonlocal state
    nonlocal handled

    # 99 is a magic number for sections in chapters that haven't been done yet.
    # Don't worry about duplication.
    if number != 99:
      name = "{} {}".format(chapter, number)
      if name in source_code.all_sections:
        fail('Duplicate section "{}" is also in {}'.format(name,
            source_code.all_sections[name]))
      source_code.all_sections[name] = "{} line {}".format(relative, line_num)

    state = ParseState(state, chapter, number, end_chapter, end_number)
    handled = True

  def pop():
    nonlocal state
    nonlocal handled
    state = state.parent
    handled = True

  # Split the source file into chunks.
  with open(path, 'r') as input:
    for line in input:
      line = line.rstrip()
      handled = False

      match = BLOCK_PATTERN.match(line)
      if match:
        push(match.group(1), int(match.group(2)), match.group(3), int(match.group(4)))

      if line.strip() == '*/' and state.end_chapter:
        pop()

      match = BEGIN_SECTION_PATTERN.match(line)
      if match:
        number = int(match.group(1))
        if number < state.number:
          fail("Can't push an earlier number {} from {}.".format(number, state.number))
        elif number == state.number:
          fail("Can't push to same number {}.".format(number))
        push(state.chapter, number)

      match = END_SECTION_PATTERN.match(line)
      if match:
        number = int(match.group(1))
        if number != state.number:
          fail("Expecting to pop {} but got {}.".format(state.number, number))
        if state.parent.chapter == None:
          fail('Cannot pop last state "{} {}".'.format(state.chapter, state.number))
        pop()

      match = BEGIN_CHAPTER_PATTERN.match(line)
      if match:
        chapter = match.group(1)
        number = int(match.group(2))

        if state.chapter != None:
          old_chapter_index = get_chapter_index(state.chapter)
          new_chapter_index = get_chapter_index(chapter)

          if chapter == state.chapter and number == state.number:
            fail('Pushing same state "{} {}"'.format(chapter, number))
          if chapter == state.chapter:
            fail('Pushing same chapter, just use "//>> {}"'.format(number))
          if new_chapter_index < old_chapter_index:
            fail('Can\'t push earlier chapter "{}" from "{}".'.format(
                chapter, state.chapter))
        push(chapter, number)

      match = END_CHAPTER_PATTERN.match(line)
      if match:
        chapter = match.group(1)
        number = int(match.group(2))
        if chapter != state.chapter or number != state.number:
          fail('Expecting to pop "{} {}" but got "{} {}".'.format(
              state.chapter, state.number, chapter, number))
        if state.parent.chapter == None:
          fail('Cannot pop last state "{} {}".'.format(state.chapter, state.number))
        pop()

      if not handled:
        if not state.chapter:
          fail("No section in effect.".format(relative))

        file.lines.append(SourceLine(line, state.chapter, state.number, state.end_chapter, state.end_number))

      line_num += 1

    # ".parent.parent" because there is always the top "null" state.
    if state.parent != None and state.parent.parent != None:
      print("{}: Ended with more than one state on the stack.".format(relative), file=sys.stderr)
      s = state
      while s.parent != None:
        print("  {} {}".format(s.chapter, s.number), file=sys.stderr)
        s = s.parent
      sys.exit(1)

  # TODO: Validate that we don't define two sections with the same chapter and
  # number. A section may end up in disjoint lines in the final output because
  # a later section is inserted in it, but it shouldn't be explicitly authored
  # that way.


def load():
  """Creates a new SourceCode object and loads all of the files into it."""
  source_code = SourceCode()

  def walk(dir, extensions, callback):
    dir = os.path.abspath(dir)
    for path in os.listdir(dir):
      nfile = os.path.join(dir, path)
      if os.path.isdir(nfile):
        walk(nfile, extensions, callback)
      elif os.path.splitext(path)[1] in extensions:
        callback(nfile)

  walk("java", [".java"],
      lambda path: load_file(source_code, "java", path))

  walk("c", [".c", ".h"],
      lambda path: load_file(source_code, "c", path))

  return source_code
