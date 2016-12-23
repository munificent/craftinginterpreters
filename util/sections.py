#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""

Parses the Java and C source files and separates out their sections. This is
used by both split_chapters.py (to make the chapter-specific source files to
test against) and build.py (to include code sections into the book).

There are a few kinds of section markers:

//> [chapter] [name]

    This marks the following code as being added in snippet [name] in
    [chapter]. A section marked like this must be closed with...

    If this is beginning a new section in the same chapter, the name is omitted.

//< [chapter] [name]

    Ends the previous innermost //> section and returns the whatever section
    surrounded it. The chapter and name are redundant, but are required to
    validate that we're exiting the section we intend to.

/* [chapter] [name] < [end chapter] [end name]
...
*/

    This marks the code in the rest of the block comment as being added in
    section [name] in [chapter]. It is then replaced or removed in section
    [end name] in [end chapter].

    Since this section doesn't end up in the final version of the code, it's
    commented out in the source.

    After the block comment, this returns to the previous section, if any.

"""

import os
import re
import sys

import book

BLOCK_PATTERN = re.compile(r'/\* ([A-Za-z\s]+) (\d+) < ([A-Za-z\s]+) (\d+)')
BLOCK_SECTION_PATTERN = re.compile(r'/\* < (\d+)')
BEGIN_SECTION_PATTERN = re.compile(r'//> (\d+)')
END_SECTION_PATTERN = re.compile(r'//< (\d+)')
BEGIN_CHAPTER_PATTERN = re.compile(r'//> ([A-Za-z\s]+) (\d+)')
END_CHAPTER_PATTERN = re.compile(r'//< ([A-Za-z\s]+) (\d+)')

# Hacky regex that matches a method or function declaration.
FUNCTION_PATTERN = re.compile(r'(\w+)>* (\w+)\(')

# Reserved words that can appear like a return type in a function declaration
# but shouldn't be treated as one.
KEYWORDS = ['new', 'return']

class SourceCode:
  """ All of the source files in the book. """

  def __init__(self):
    self.files = []

    # The chapter/number pairs of every parsed section. Used to ensure we don't
    # try to create the same section twice.
    # TODO: Remove now that we have snippet tags.
    self.all_sections = {}

    self.snippet_tags = book.get_chapter_snippet_tags()

  def find_snippet_tag(self, chapter, name):
    snippets = self.snippet_tags[chapter]

    if name in snippets:
      return snippets[name]

    print('Warning: Unknown snippet tag "{} {}".'.format(chapter, name))
    # Synthesize a fake one so we can keep going.
    # TODO: Only do this for some blessed tag name for unfinished chapters.
    snippets[name] = book.SnippetTag(chapter, name, len(snippets))
    return snippets[name]


  def last_snippet_for_chapter(self, chapter):
    """ Returns the last snippet tag appearing in [chapter]. """
    snippets = self.snippet_tags[chapter]
    last = None
    for snippet in snippets.values():
      if not last or snippet > last:
        last = snippet

    return last


  def find_all(self, chapter):
    """ Gets the list of snippets that occur in [chapter]. """
    sections = {}

    first_lines = {}
    last_lines = {}

    # Create a new section for [name] if it doesn't already exist.
    def ensure_section(name, line_num):
      if not name in sections:
        section = Section(file, name)
        sections[name] = section
        first_lines[section] = line_num
        return section

      section = sections[name]
      if name != '99' and section.file.path != file.path:
        raise "{} {} appears in two files, {} and {}".format(
            chapter, name, section.file.path, file.path)

      return section

    # TODO: If this is slow, could organize directly by sections in SourceCode.
    # Find the lines added and removed in each section.
    for file in self.files:
      line_num = 0
      for line in file.lines:
        if line.start.chapter == chapter:
          section = ensure_section(line.start.name, line_num)
          section.added.append(line.text)
          last_lines[section] = line_num

          if line.function and not section.function:
            section.function = line.function

        if line.end and line.end.chapter == chapter:
          section = ensure_section(line.end.name, line_num)
          section.removed.append(line.text)
          last_lines[section] = line_num

        line_num += 1

    if len(sections) == 0: return sections

    # Find the surrounding context lines and location for each section.
    for name, section in sections.items():
      current_snippet = self.snippet_tags[chapter][name]

      # Look for preceding lines.
      i = first_lines[section] - 1
      before = []
      while i >= 0 and len(before) <= 5:
        line = section.file.lines[i]
        if line.is_present(current_snippet):
          before.append(line.text)

          if line.function and not section.preceding_function:
            section.preceding_function = line.function
        i -= 1
      section.context_before = before[::-1]

      # Look for following lines.
      i = last_lines[section] + 1
      after = []
      while i < len(section.file.lines) and len(after) <= 5:
        line = section.file.lines[i]
        if line.is_present(current_snippet):
          after.append(line.text)
        i += 1
      section.context_after = after

    # if chapter == "Scanning":
    #   for number, section in sections.items():
    #     print("Scanning {} - {}".format(number, section.file.path))
    #     for line in section.context_before:
    #       print("   {}".format(line.rstrip()))
    #     for line in section.removed:
    #       print("-- {}".format(line.rstrip()))
    #     for line in section.added:
    #       print("++ {}".format(line.rstrip()))
    #     for line in section.context_after:
    #       print("   {}".format(line.rstrip()))

    #   for section, first in first_lines.items():
    #     last = last_lines[section]
    #     print("Scanning {} - {}: {} to {}".format(
    #         section.name, section.file.path, first, last))

    return sections

  def split_chapter(self, file, chapter, name):
    """
    Gets the code for [file] as it appears at snippet [name] of [chapter].
    """
    tag = self.snippet_tags[chapter][name]

    source_file = None
    for source in self.files:
      if source.path == file:
        source_file = source
        break

    if source_file == None:
      raise Exception('Could not find file "{}"'.format(file))

    output = ""
    for line in source_file.lines:
      if (line.is_present(tag)):
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
  def __init__(self, text, function, start, end):
    self.text = text
    self.function = function
    self.start = start
    self.end = end

  def is_present(self, snippet):
    """
    Returns true if this line exists by the time we reach [snippet].
    """
    if snippet < self.start:
      # We haven't reached this line's snippet yet.
      return False

    if self.end and snippet > self.end:
      # We are past the snippet where it is removed.
      return False

    return True

  def __str__(self):
    result = "{:72} // {}".format(self.text, self.start)

    if self.end:
      result += " < {}".format(self.end)

    if self.function:
      result += " (in {})".format(self.function)

    return result


# TODO: Rename "Snippet"?
class Section:
  def __init__(self, file, name):
    self.file = file
    self.name = name
    self.context_before = []
    self.added = []
    self.removed = []
    self.context_after = []

    self.function = None
    self.preceding_function = None

  def location(self):
    """Describes where in the file this section appears."""

    if len(self.context_before) == 0:
      # No lines before the section, it must be a new file.
      return 'create new file'

    if self.function:
      if self.function != self.preceding_function:
        return 'add after <em>{}</em>()'.format(
            self.preceding_function)
      else:
        return 'in <em>{}</em>()'.format(self.function)

    return None


class ParseState:
  def __init__(self, parent, start, end=None):
    self.parent = parent
    self.start = start
    self.end = end


def load_file(source_code, source_dir, path):
  relative = os.path.relpath(path, source_dir)

  # Don't process the generated files. We only worry about GenerateAst.java.
  if relative == "com/craftinginterpreters/lox/Expr.java": return
  if relative == "com/craftinginterpreters/lox/Stmt.java": return

  file = SourceFile(relative)
  source_code.files.append(file)

  line_num = 1
  state = ParseState(None, None)
  handled = False

  current_function = None

  def fail(message):
    print("{} line {}: {}".format(relative, line_num, message), file=sys.stderr)
    sys.exit(1)

  def push(chapter, name, end_chapter=None, end_name=None):
    nonlocal state
    nonlocal handled

    start = source_code.find_snippet_tag(chapter, name)
    end = None
    if end_chapter:
      end = source_code.find_snippet_tag(end_chapter, end_name)

    # 99 is a magic number for sections in chapters that haven't been done yet.
    # Don't worry about duplication.
    # if number != 99:
    #   name = "{} {}".format(chapter, number)
    #   if name in source_code.all_sections:
    #     fail('Duplicate section "{}" is also in {}'.format(name,
    #         source_code.all_sections[name]))
    #   source_code.all_sections[name] = "{} line {}".format(relative, line_num)

    state = ParseState(state, start, end)
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

      # See if we reached a new function or method declaration.
      match = FUNCTION_PATTERN.search(line)
      if match and match.group(1) not in KEYWORDS:
        # Hack. Don't get caught by comments or string literals.
        if '//' not in line and '"' not in line:
          current_function = match.group(2)

      match = BLOCK_PATTERN.match(line)
      if match:
        push(match.group(1), match.group(2), match.group(3), match.group(4))

      match = BLOCK_SECTION_PATTERN.match(line)
      if match:
        name = match.group(1)
        push(state.start.chapter, state.start.name, state.start.chapter, name)

      if line.strip() == '*/' and state.end:
        pop()

      match = BEGIN_SECTION_PATTERN.match(line)
      if match:
        name = match.group(1)
        # TODO: Look up snippet for name and compare indexes.
        # if number < state.number:
        #   fail("Can't push an earlier snippet {} from {}.".format(name, state.start.name))
        # elif name == state.start.name:
        #   fail("Can't push to same snippet {}.".format(name))
        push(state.start.chapter, name)

      match = END_SECTION_PATTERN.match(line)
      if match:
        name = match.group(1)
        number = int(name)
        if name != state.start.name:
          fail("Expecting to pop {} but got {}.".format(state.start.name, name))
        if state.parent.start.chapter == None:
          fail('Cannot pop last state "{}".'.format(state.start))
        pop()

      match = BEGIN_CHAPTER_PATTERN.match(line)
      if match:
        chapter = match.group(1)
        name = match.group(2)

        if state.start != None:
          old_chapter = book.chapter_number(state.start.chapter)
          new_chapter = book.chapter_number(chapter)

          if chapter == state.start.chapter and name == state.start.name:
            fail('Pushing same state "{} {}"'.format(chapter, name))
          if chapter == state.start.chapter:
            fail('Pushing same chapter, just use "//>> {}"'.format(name))
          if new_chapter < old_chapter:
            fail('Can\'t push earlier chapter "{}" from "{}".'.format(
                chapter, state.start.chapter))
        push(chapter, name)

      match = END_CHAPTER_PATTERN.match(line)
      if match:
        chapter = match.group(1)
        name = match.group(2)
        if chapter != state.start.chapter or name != state.start.name:
          fail('Expecting to pop "{}" but got "{} {}".'.format(
              state.start, chapter, name))
        if state.parent.start.chapter == None:
          fail('Cannot pop last state "{}".'.format(state.start))
        pop()

      if not handled:
        if not state.start:
          fail("No section in effect.".format(relative))

        source_line = SourceLine(line, current_function, state.start, state.end)
        file.lines.append(source_line)

      # Hacky. Detect the end of the function. Assumes everything is nicely
      # indented.
      if path.endswith('.java') and line == '  }':
        current_function = None
      elif (path.endswith('.c') or path.endswith('.h')) and line == '}':
        current_function = None

      line_num += 1

    # ".parent.parent" because there is always the top "null" state.
    if state.parent != None and state.parent.parent != None:
      print("{}: Ended with more than one state on the stack.".format(relative),
          file=sys.stderr)
      s = state
      while s.parent != None:
        print("  {}".format(s.start), file=sys.stderr)
        s = s.parent
      sys.exit(1)

  # TODO: Validate that we don't define two sections with the same chapter and
  # number. A section may end up in disjoint lines in the final output because
  # a later section is inserted in it, but it shouldn't be explicitly authored
  # that way.


def load():
  """Creates a new SourceCode object and loads all of the files into it."""
  source_code = SourceCode()

  def walk(dir, callback):
    dir = os.path.abspath(dir)
    for path in os.listdir(dir):
      nfile = os.path.join(dir, path)
      if os.path.isdir(nfile):
        walk(nfile, callback)
      elif os.path.splitext(path)[1] in [".c", ".h", ".java"]:
        callback(nfile)

  walk("java", lambda path: load_file(source_code, "java", path))
  walk("c", lambda path: load_file(source_code, "c", path))

  return source_code
