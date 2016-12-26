#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""

Parses the Java and C source files and separates out their snippets. This is
used by both split_chapters.py (to make the chapter-specific source files to
test against) and build.py (to include code snippets into the book).

There are a few kinds of snippet markers:

//> [chapter] [name]

    This marks the following code as being added in snippet [name] in
    [chapter]. If this is beginning a new snippet in the same chapter, the name
    is omitted.

//< [chapter] [name]

    Ends the previous innermost //> snippet and returns to whatever snippet
    surrounded it. The chapter and name are redundant, but are required to
    validate that we're exiting the snippet we intend to.

/* [chapter] [name] < [end chapter] [end name]
...
*/

    This marks the code in the rest of the block comment as being added in
    snippet [name] in [chapter]. It is then replaced or removed in snippet
    [end name] in [end chapter].

    Since this snippet doesn't end up in the final version of the code, it's
    commented out in the source.

    After the block comment, this returns to the previous snippet, if any.

"""

import os
import re
import sys

import book

BLOCK_PATTERN = re.compile(
    r'/\* ([A-Z][A-Za-z\s]+) ([-a-z0-9]+) < ([A-Z][A-Za-z\s]+) ([-a-z0-9]+)')
BLOCK_SNIPPET_PATTERN = re.compile(r'/\* < ([-a-z0-9]+)')
BEGIN_SNIPPET_PATTERN = re.compile(r'//> ([-a-z0-9]+)')
END_SNIPPET_PATTERN = re.compile(r'//< ([-a-z0-9]+)')
BEGIN_CHAPTER_PATTERN = re.compile(r'//> ([A-Z][A-Za-z\s]+) ([-a-z0-9]+)')
END_CHAPTER_PATTERN = re.compile(r'//< ([A-Z][A-Za-z\s]+) ([-a-z0-9]+)')

# Hacky regexes that matches a function, method or constructor declaration.
FUNCTION_PATTERN = re.compile(r'(\w+)>* (\w+)\(')
CONSTRUCTOR_PATTERN = re.compile(r'^  ([A-Z][a-z]\w+)\(')
CLASS_PATTERN = re.compile(r'(public )?class (\w+)')

# Reserved words that can appear like a return type in a function declaration
# but shouldn't be treated as one.
KEYWORDS = ['new', 'return']

class SourceCode:
  """ All of the source files in the book. """

  def __init__(self):
    self.files = []
    self.snippet_tags = book.get_chapter_snippet_tags()

    # The errors that occurred when parsing the source code, mapped to the
    # chapter where the error should be displayed.
    self.errors = {}
    for chapter in book.CODE_CHAPTERS:
      self.errors[chapter] = []


  def find_snippet_tag(self, chapter, name):
    snippets = self.snippet_tags[chapter]

    if name in snippets:
      return snippets[name]

    if name != 'not-yet':
      print('Error: "{}" does not use snippet "{}".'.format(chapter, name),
          file=sys.stderr)

    # Synthesize a fake one so we can keep going.
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
    snippets = {}

    first_lines = {}
    last_lines = {}

    # Create a new snippet for [name] if it doesn't already exist.
    def ensure_snippet(name, line_num):
      if not name in snippets:
        snippet = Snippet(file, name)
        snippets[name] = snippet
        first_lines[snippet] = line_num
        return snippet

      snippet = snippets[name]
      if name != 'not-yet' and snippet.file.path != file.path:
        print('Error: "{}" appears in two files, {} and {}.'.format(
                chapter, name, snippet.file.path, file.path),
            file=sys.stderr)

      return snippet

    # TODO: If this is slow, could organize directly by snippets in SourceCode.
    # Find the lines added and removed in each snippet.
    for file in self.files:
      line_num = 0
      for line in file.lines:
        if line.start.chapter == chapter:
          snippet = ensure_snippet(line.start.name, line_num)
          snippet.added.append(line.text)
          last_lines[snippet] = line_num

          if len(snippet.added) == 1:
            snippet.function = line.function
            snippet.clas = line.clas

        if line.end and line.end.chapter == chapter:
          snippet = ensure_snippet(line.end.name, line_num)
          snippet.removed.append(line.text)
          last_lines[snippet] = line_num

        line_num += 1

    # Find the surrounding context lines and location for each snippet.
    for name, snippet in snippets.items():
      current_snippet = self.snippet_tags[chapter][name]

      # Look for preceding lines.
      i = first_lines[snippet] - 1
      before = []
      while i >= 0 and len(before) <= 5:
        line = snippet.file.lines[i]
        if line.is_present(current_snippet):
          before.append(line.text)

          if line.function and not snippet.preceding_function:
            snippet.preceding_function = line.function
        i -= 1
      snippet.context_before = before[::-1]

      # Look for following lines.
      i = last_lines[snippet] + 1
      after = []
      while i < len(snippet.file.lines) and len(after) <= 5:
        line = snippet.file.lines[i]
        if line.is_present(current_snippet):
          after.append(line.text)
        i += 1
      snippet.context_after = after

    return snippets

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
  def __init__(self, text, function, clas, start, end):
    self.text = text
    self.function = function
    self.clas = clas
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


class Snippet:
  """
  A snippet of source code that is inserted in the book.
  """

  def __init__(self, file, name):
    self.file = file
    self.name = name
    self.context_before = []
    self.added = []
    self.removed = []
    self.context_after = []

    self.function = None
    self.preceding_function = None
    self.clas = None

  def location(self):
    """Describes where in the file this snippet appears."""

    if len(self.context_before) == 0:
      # No lines before the snippet, it must be a new file.
      return 'create new file'

    if self.preceding_function and self.function != self.preceding_function:
      return 'add after <em>{}</em>()'.format(self.preceding_function)
    elif self.function:
      return 'in <em>{}</em>()'.format(self.function)
    elif self.clas:
      return 'in class <em>{}</em>'.format(self.clas)

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
  current_class = None

  def error(message):
    print("Error: {} line {}: {}".format(relative, line_num, message),
        file=sys.stderr)
    source_code.errors[state.start.chapter].append(
        "{} line {}: {}".format(relative, line_num, message))

  def push(chapter, name, end_chapter=None, end_name=None):
    nonlocal state
    nonlocal handled

    start = source_code.find_snippet_tag(chapter, name)
    end = None
    if end_chapter:
      end = source_code.find_snippet_tag(end_chapter, end_name)

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

      match = CONSTRUCTOR_PATTERN.match(line)
      if match:
        current_function = match.group(1)

      match = CLASS_PATTERN.match(line)
      if match:
        current_class = match.group(2)


      match = BLOCK_PATTERN.match(line)
      if match:
        push(match.group(1), match.group(2), match.group(3), match.group(4))

      match = BLOCK_SNIPPET_PATTERN.match(line)
      if match:
        name = match.group(1)
        push(state.start.chapter, state.start.name, state.start.chapter, name)

      if line.strip() == '*/' and state.end:
        pop()

      match = BEGIN_SNIPPET_PATTERN.match(line)
      if match:
        name = match.group(1)
        tag = source_code.find_snippet_tag(state.start.chapter, name)
        if tag < state.start:
          error("Can't push earlier snippet {} from {}.".format(name, state.start.name))
        elif tag == state.start:
          error("Can't push to same snippet {}.".format(name))
        push(state.start.chapter, name)

      match = END_SNIPPET_PATTERN.match(line)
      if match:
        name = match.group(1)
        if name != state.start.name:
          error("Expecting to pop {} but got {}.".format(state.start.name, name))
        if state.parent.start.chapter == None:
          error('Cannot pop last state {}.'.format(state.start))
        pop()

      match = BEGIN_CHAPTER_PATTERN.match(line)
      if match:
        chapter = match.group(1)
        name = match.group(2)

        if state.start != None:
          old_chapter = book.chapter_number(state.start.chapter)
          new_chapter = book.chapter_number(chapter)

          if chapter == state.start.chapter and name == state.start.name:
            error('Pushing same snippet "{} {}"'.format(chapter, name))
          if chapter == state.start.chapter:
            error('Pushing same chapter, just use "//>> {}"'.format(name))
          if new_chapter < old_chapter:
            error('Can\'t push earlier chapter "{}" from "{}".'.format(
                chapter, state.start.chapter))
        push(chapter, name)

      match = END_CHAPTER_PATTERN.match(line)
      if match:
        chapter = match.group(1)
        name = match.group(2)
        if chapter != state.start.chapter or name != state.start.name:
          fail('Expecting to pop "{} {}" but got "{} {}".'.format(
              state.start.chapter, state.start.name, chapter, name))
        if state.parent.start.chapter == None:
          fail('Cannot pop last state "{}".'.format(state.start))
        pop()

      if not handled:
        if not state.start:
          fail("No snippet in effect.".format(relative))

        source_line = SourceLine(line, current_function, current_class,
            state.start, state.end)
        file.lines.append(source_line)

      # Hacky. Detect the end of the function or class. Assumes everything is
      # nicely indented.
      if path.endswith('.java') and line == '  }':
        current_function = None
      elif (path.endswith('.c') or path.endswith('.h')) and line == '}':
        current_function = None

      if path.endswith('.java') and line == '}':
        current_class = None

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

  # TODO: Validate that we don't define two snippets with the same chapter and
  # number. A snippet may end up in disjoint lines in the final output because
  # a later snippet is inserted in it, but it shouldn't be explicitly authored
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
