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
FUNCTION_PATTERN = re.compile(r'(\w+)>*\*? (\w+)\(')
CONSTRUCTOR_PATTERN = re.compile(r'^  ([A-Z][a-z]\w+)\(')
TYPE_PATTERN = re.compile(r'(public )?(abstract )?(class|enum|interface) ([A-Z]\w+)')
TYPEDEF_PATTERN = re.compile(r'typedef (enum|struct|union)( \w+)? {')
STRUCT_PATTERN = re.compile(r'struct (s\w+)? {')
TYPEDEF_NAME_PATTERN = re.compile(r'\} (\w+);')

# Reserved words that can appear like a return type in a function declaration
# but shouldn't be treated as one.
KEYWORDS = ['new', 'return', 'throw']

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

    if name != 'not-yet' and name != 'omit':
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
    def ensure_snippet(name, line_num = None):
      if not name in snippets:
        snippet = Snippet(file, name)
        snippets[name] = snippet
        first_lines[snippet] = line_num
        return snippet

      snippet = snippets[name]
      if first_lines[snippet] is None:
        first_lines[snippet] = line_num
      if name != 'not-yet' and name != 'omit' and snippet.file.path != file.path:
        print('Error: "{} {}" appears in two files, {} and {}.'.format(
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
            snippet.location = line.location

        if line.end and line.end.chapter == chapter:
          snippet = ensure_snippet(line.end.name)
          snippet.removed.append(line.text)

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

          # Store the more precise preceding location we find.
          if (not snippet.preceding_location or
              line.location.depth() > snippet.preceding_location.depth()):
            snippet.preceding_location = line.location

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

    # Find line changes that just add a trailing comma.
    for name, snippet in snippets.items():
      if (len(snippet.added) > 0 and
          len(snippet.removed) > 0 and
          snippet.added[0] == snippet.removed[-1] + ","):
        snippet.added_comma = snippet.added[0]
        del snippet.added[0]
        del snippet.removed[-1]

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
      if line.is_present(tag):
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
  def __init__(self, text, location, start, end):
    self.text = text
    self.location = location
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

    return result + " in {}".format(self.location)


class Location:
  """
  The context in which a line of code appears. The chain of types and functions
  it's in.
  """
  def __init__(self, parent, kind, name):
    self.parent = parent
    self.kind = kind
    self.name = name

  def __str__(self):
    result = self.kind + ' ' + self.name
    if self.parent:
      result = str(self.parent) + ' > ' + result
    return result

  def __eq__(self, other):
    return other != None and self.kind == other.kind and self.name == other.name

  @property
  def is_file(self):
    return self.kind == 'file'

  @property
  def is_function(self):
    return self.kind in ['constructor', 'function', 'method']

  def to_html(self, preceding, removed):
    """
    Generates a string of HTML that describes a snippet at this location, when
    following the [preceding] location.
    """

    # Note: The order of these is highly significant.
    if self.kind == 'class' and self.parent and self.parent.kind == 'class':
      return 'nest inside class <em>{}</em>'.format(self.parent.name)

    if self.is_function and preceding == self:
      # We're still inside a function.
      return 'in <em>{}</em>()'.format(self.name)

    if self.is_function and removed:
      # Hack. We don't appear to be in the middle of a function, but we are
      # replacing lines, so assume we're replacing the entire function.
      return '{} <em>{}</em>()'.format(self.kind, self.name)

    if self.parent == preceding and not preceding.is_file:
      # We're nested inside a type.
      return 'in {} <em>{}</em>'.format(preceding.kind, preceding.name)

    if preceding == self and not self.is_file:
      # We're still inside a type.
      return 'in {} <em>{}</em>'.format(self.kind, self.name)

    if preceding.is_function:
      # We aren't inside a function, but we do know the preceding one.
      return 'add after <em>{}</em>()'.format(preceding.name)

    if not preceding.is_file:
      # We aren't inside any function, but we do know what we follow.
      return 'add after {} <em>{}</em>'.format(preceding.kind, preceding.name)

    return None

  def depth(self):
    current = self
    result = 0
    while current:
      result += 1
      current = current.parent

    return result

  def pop_to_depth(self, depth):
    """
    Discard as many children as needed to get to [depth] parents.
    """
    current = self
    locations = []
    while current:
      locations.append(current)
      current = current.parent

    # If we are already shallower, there is nothing to pop.
    if len(locations) < depth + 1: return self

    return locations[-depth - 1]


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

    # If the snippet replaces a line with the same line but with a trailing
    # comma, this is that line (with the comma).
    self.added_comma = None

    self.location = None
    self.preceding_location = None

  def dump(self):
    print(self.name)
    print("prev: {}".format(self.preceding_location))
    print("here: {}".format(self.location))
    for line in self.context_before:
      print("    {}".format(line))
    for line in self.removed:
      print("  - {}".format(line))
    for line in self.added:
      print("  + {}".format(line))
    for line in self.context_after:
      print("    {}".format(line))

  def describe_location(self):
    """
    Describes where in the file this snippet appears. Returns a list of HTML
    strings.
    """
    result = ['<em>{}</em>'.format(self.file.nice_path())]

    if len(self.context_before) == 0 and len(self.context_after) == 0:
      # No lines around the snippet, it must be a new file.
      result.append('create new file')
    elif len(self.context_before) == 0:
      # No lines before the snippet, it must be at the beginning.
      result.append('add to top of file')
    else:
      location = self.location.to_html(self.preceding_location, self.removed)
      if location:
        result.append(location)

    if self.removed and self.added:
      result.append('replace {} line{}'.format(
          len(self.removed), '' if len(self.removed) == 1 else 's'))
    elif self.removed and not self.added:
      result.append('remove {} line{}'.format(
          len(self.removed), '' if len(self.removed) == 1 else 's'))

    if self.added_comma:
      result.append('add <em>&ldquo;,&rdquo;</em> to previous line')

    return result


class ParseState:
  def __init__(self, parent, start, end=None):
    self.parent = parent
    self.start = start
    self.end = end


def load_file(source_code, source_dir, path):
  relative = os.path.relpath(path, source_dir)

  file = SourceFile(relative)
  source_code.files.append(file)

  line_num = 1
  state = ParseState(None, None)
  handled = False

  current_location = Location(None, 'file', file.nice_path())
  location_before_block = None

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
    lines = input.read().splitlines()

    printed_file = False
    line_num = 1
    for line in lines:
      line = line.rstrip()
      handled = False

      # Report any lines that are too long.
      trimmed = re.sub(r'// \[([-a-z0-9]+)\]', '', line)
      if len(trimmed) > 72 and not '/*' in trimmed:
        if not printed_file:
          print("Long line in {}:".format(file.path))
          printed_file = True
        print("{0:4} ({1:2} chars): {2}".format(line_num, len(trimmed), trimmed))

      # See if we reached a new function or method declaration.
      match = FUNCTION_PATTERN.search(line)
      is_function_declaration = False
      if match and "#define" not in line and match.group(1) not in KEYWORDS:
        # Hack. Don't get caught by comments or string literals.
        if '//' not in line and '"' not in line:
          current_location = Location(
              current_location,
              'method' if file.path.endswith('.java') else 'function',
              match.group(2))
          # TODO: What about declarations with aside comments:
          #   void foo(); // [wat]
          is_function_declaration = line.endswith(';')

      match = CONSTRUCTOR_PATTERN.match(line)
      if match:
        current_location = Location(current_location,
                                    'constructor', match.group(1))

      match = TYPE_PATTERN.search(line)
      if match:
        # Hack. Don't get caught by comments or string literals.
        if '//' not in line and '"' not in line:
          current_location = Location(current_location,
                                      match.group(3), match.group(4))

      match = STRUCT_PATTERN.match(line)
      if match:
        current_location = Location(current_location, 'struct', match.group(1))

      match = TYPEDEF_PATTERN.match(line)
      if match:
        # We don't know the name of the typedef.
        current_location = Location(current_location, match.group(1), '???')

      match = BLOCK_PATTERN.match(line)
      if match:
        push(match.group(1), match.group(2), match.group(3), match.group(4))
        location_before_block = current_location

      match = BLOCK_SNIPPET_PATTERN.match(line)
      if match:
        name = match.group(1)
        push(state.start.chapter, state.start.name, state.start.chapter, name)
        location_before_block = current_location

      if line.strip() == '*/' and state.end:
        current_location = location_before_block
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
          error('Expecting to pop "{} {}" but got "{} {}".'.format(
              state.start.chapter, state.start.name, chapter, name))
        if state.start.chapter == None:
          error('Cannot pop last state "{}".'.format(state.start))
        pop()

      if not handled:
        if not state.start:
          error("No snippet in effect.".format(relative))

        source_line = SourceLine(line, current_location, state.start, state.end)
        file.lines.append(source_line)

      match = TYPEDEF_NAME_PATTERN.match(line)
      if match:
        # Now we know the typedef name.
        current_location.name = match.group(1)
        current_location = current_location.parent

      # Use "startswith" to include lines like "} [aside-marker]".
      # TODO: Hacky. Generalize?
      if line.startswith('}'):
        current_location = current_location.pop_to_depth(0)
      elif line.startswith('  }'):
        current_location = current_location.pop_to_depth(1)
      elif line.startswith('    }'):
        current_location = current_location.pop_to_depth(2)

      # If we reached a function declaration, not a definition, then it's done
      # after one line.
      if is_function_declaration:
        current_location = current_location.parent

      # Hack. There is a one-line class in Parser.java.
      if 'class ParseError' in line:
        current_location = current_location.parent

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
