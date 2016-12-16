#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""

Parses the Java and C source files and separates out their sections. This is
used by both split_chapters.py (to make the chapter-specific source files to
test against) and build.py (to include code sections into the book).

There are two kinds of section markers:

//>= [chapter] [number]

This marks the following code as being added in snippet [number] in [chapter].
The code then remains until the end of the book.

/*>= [chapter] [number] < [end chapter] [end number]
...
*/

This marks the code in the rest of the block comment as being added in snippet
[number] in [chapter]. It is then replaced or removed in snippet [end number]
in [end chapter].

Since this section doesn't end up in the final version of the code, it's
commented out in the source.

The effect of both of these is that the source we hand-edit and store on disc
also reflects the final version of the code.

"""

import os
import re

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

LINE_PATTERN = re.compile(r'//>= ([A-Za-z\s]+) (\d+)')
BLOCK_PATTERN = re.compile(r'/\*>= ([A-Za-z\s]+) (\d+) < ([A-Za-z\s]+) (\d+)')

class SourceCode:
  """ All of the source files in the book. """

  def __init__(self):
    self.files = []

  def find(self, chapter, number):
    """ Gets the lines of code in snippet [number] in [chapter]. """
    result_lines = []

    # TODO: If this is slow, could organize directly by sections in SourceCode.
    for file in self.files:
      for line in file.lines:
        if line.chapter == chapter and line.number == number:
          result_lines.append(line.text)
        elif line.end_chapter == chapter and line.end_number == number:
          raise "Deleted lines not implemented yet"
          pass

      if len(result_lines) > 0:
        return (file, result_lines)

    # TODO: Check for the lines from the same snippet appearing in separate
    # files.

    raise Exception('Could not find lines for "{} {}"'.format(chapter, number))

  def at_chapter(self, file, chapter):
    """ Gets the code for [file] as it appears at the end of [chapter]. """
    index = chapter_index(chapter)

    source_file = None
    for source in self.files:
      if source.path == file:
        source_file = source
        break

    if source_file == None:
      raise Exception('Could not find file "{}"'.format(file))

    output = ""
    for line in source_file.lines:
      if (line.in_chapter(index)):
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

  def in_chapter(self, index):
    """ Returns true if this line exists by the end of chapter [index]. """
    if chapter_index(self.chapter) > index:
      # We haven't gotten to it yet.
      return False

    if self.end_chapter != None:
      if chapter_index(self.end_chapter) <= index:
        # We are past it.
        return False

    return True


def chapter_name(number):
  """Given a chapter number, returns its name."""
  if number < 14:
    return JAVA_CHAPTERS[number - 4]

  return C_CHAPTERS[number - 14]


def chapter_index(name):
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
  chapter = None
  number = None
  end_chapter = None
  end_number = None

  # Split the source file into chunks.
  with open(path, 'r') as input:
    for line in input:
      line = line.rstrip()
      handled = False

      match = LINE_PATTERN.match(line)
      if match:
        chapter = match.group(1)
        number = match.group(2)
        end_chapter = None
        end_number = None
        handled = True

      match = BLOCK_PATTERN.match(line)
      if match:
        chapter = match.group(1)
        number = match.group(2)
        end_chapter = match.group(3)
        end_number = match.group(4)
        handled = True

      if line.strip() == '*/' and end_chapter:
        chapter = None
        number = None
        end_chapter = None
        end_number = None
        handled = True

      if not handled:
        if not chapter:
          raise "{}: No section in effect".format(relative)

        file.lines.append(SourceLine(line, chapter, number, end_chapter, end_number))


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
