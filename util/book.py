#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Tracks metadata about the material in the book.
"""
from functools import total_ordering
import os
import re

# Matches the name in a `^code` tag and ignores the rest.
SNIPPET_TAG_PATTERN = re.compile(r'\s*\^code ([-a-z0-9]+).*')
TITLE_PATTERN = re.compile(r'\^title (.*)')

TOC = [
  {
    'name': '',
    'chapters': [
      {
        'name': 'Crafting Interpreters',
        'topics': [],
      },
      {
        'name': 'Table of Contents',
        'topics': [],
      }
    ],
  },
  {
    'name': 'Welcome',
    'chapters': [
      {
        'name': 'Introduction',
        'topics': [
          'Why learn programming languages?',
          'How this book is organized'
        ],
        'design_note': "What's in a Name?"
      },
      {
        'name': 'A Map of the Territory',
        'topics': [
          'Interpreters and compilers', 'Phases of a compiler',
          'Transpilers', 'Just-in-time compilation'
        ],
      },
      {
        'name': 'The Lox Language',
        'topics': [
          'Dynamic typing', 'Automatic memory management', 'Built-in types',
          'Expressions', 'Statements', 'Object-orientation', 'Prototypes'
        ],
        'design_note': "Expressions and Statements"
      }
    ]
  },
  {
    'name': 'A Tree-Walk Interpreter',
    'chapters': [
      {
        'name': 'Scanning',
        'topics': [
          'Tokens', 'Token types', 'Lexical analysis', 'Regular languages',
          'Lookahead', 'Reserved words', 'Error reporting'
        ],
        'design_note': "Implicit Semicolons"
      },
      {
        'name': 'Representing Code',
        'topics': [
          'Abstract syntax trees', 'Expression trees', 'Generating AST classes',
          'The Visitor pattern', 'Pretty printing'
        ],
      },
      {
        'name': 'Parsing Expressions',
        'topics': [
          'Expression nodes', 'Recursive descent', 'Precedence',
          'Associativity', 'Primary expressions', 'Syntax errors'
        ],
        'design_note': "Logic Versus History"
      },
      {
        'name': 'Evaluating Expressions',
        'topics': [
          'The Interpreter pattern', 'Tree-walk interpretation',
          'Subexpressions', 'Runtime errors', 'Type checking', 'Truthiness'
        ],
        'design_note': 'Static and Dynamic Typing'
      },
      {
        'name': 'Statements and State',
        'topics': [
          'Statement nodes', 'Blocks', 'Expression statements', 'Variables',
          'Assignment', 'Lexical scope', 'Environments'
        ],
        'design_note': 'Implicit Variable Declaration'
      },
      {
        'name': 'Control Flow',
        'topics': [
          'If statements', 'While statements', 'For statements', 'Desugaring',
          'Logical operators', 'Short-circuit evaluation'
        ],
        'design_note': 'Spoonfuls of Syntactic Sugar'
      },
      {
        'name': 'Functions',
        'topics': [
          'Function declarations', 'Formal parameters', 'Call expressions',
          'Arguments', 'Return statements', 'Function objects', 'Closures',
          'Arity', 'Native functions'
        ],
      },
      {
        'name': 'Resolving and Binding',
        'topics': ['Name resolution', 'Early binding', 'Static errors'],
      },
      {
        'name': 'Classes',
        'topics': [
          'Class declarations', 'Fields', 'Properties',
          'Get and set expressions', 'Constructors', 'Initializers', 'this',
          'Method references'
        ],
        'design_note': 'Prototypes and Power'
      },
      {
        'name': 'Inheritance',
        'topics': ['Superclasses', 'Overriding', 'Calling superclass methods'],
      }
    ]
  },
  {
    'name': 'A Bytecode Virtual Machine',
    'chapters': [
      {
        'name': 'Chunks of Bytecode',
        'topics': [
          'Allocation', 'Dynamic arrays', 'Code chunks', 'Constant tables',
          'Instruction arguments', 'Disassembly'
        ],
        'design_note': 'Test Your Language'
      },
      {
        'name': 'A Virtual Machine',
        'topics': [
          'Bytecode instructions', 'The stack', 'Instruction pointer',
          'Loading constants', 'Arithmetic instructions', 'Interpreter loop',
          'Instruction dispatch'
        ],
        'design_note': 'Register-Based Bytecode'
      },
      {
        'name': 'Scanning on Demand',
        'topics': [
          'Reading files', 'Token values', 'Source pointers', 'LL(k) grammars'
        ],
      },
      {
        'name': 'Compiling Expressions',
        'topics': [
          'Pratt parsers', 'Binary operators', 'Unary operators', 'Precedence',
          'Single-pass compilation', 'Code generation'
        ],
        'done': False,
      },
      {
        'name': 'Types of Values',
        'topics': [
          'Tagged unions', 'Boolean values', 'nil',
          'Comparison and equality operators', 'Not operator', 'Runtime errors'
        ],
        'done': False,
      },
      {
        'name': 'Strings',
        'topics': [
          'Objects', 'Reference types', 'Heap tracing', 'Concatenation',
          'Polymorphism'
        ],
        'done': False,
      },
      {
        'name': 'Hash Tables',
        'topics': [
          'Hash functions', 'String hashing', 'Linear probing',
          'Rehashing', 'Reference equality', 'String interning'
        ],
        'done': False,
      },
      {
        'name': 'Global Variables',
        'topics': [
          'Statements', 'Variable declaration', 'Assignment',
          'Global variables table'
        ],
        'done': False,
      },
      {
        'name': 'Local Variables',
        'topics': [
          'Blocks', 'Scope depth', 'Stack variables', 'Name resolution',
          'Byte argument instructions'
        ],
        'done': False,
      },
      {
        'name': 'Jumping Forward and Back',
        'topics': [
          'Jump instructions', 'Conditional jumps', 'Control flow statements',
          'Short-circuiting', 'Backpatching'
        ],
        'done': False,
      },
      {
        'name': 'Calls and Functions',
        'topics': [
          'Calling convention', 'Arguments', 'Call instructions',
          'Native functions', 'Function declarations', 'Parameters',
          'Return statements', 'Function objects', 'Call frames',
          'Stack overflow'
        ],
        'done': False,
      },
      {
        'name': 'Closures',
        'topics': [
          'Upvalues', 'Resolving enclosing locals', 'Closure flattening',
          'Capturing variables', 'Closing upvalues'
        ],
        'done': False,
      },
      {
        'name': 'Garbage Collection',
        'topics': [
          'Roots', 'Stress testing', 'Mark-sweep collection', 'Tracing',
          'Tri-color marking', 'Weak references', 'Heap growth'
        ],
        'done': False,
      },
      {
        'name': 'Classes and Instances',
        'topics': [
          'Property expressions', 'Class declarations', 'Instances', 'Fields',
          'Undefined fields'
        ],
        'done': False,
      },
      {
        'name': 'Methods and Initializers',
        'topics': [
          'Invocation expressions', 'This', 'Method declarations',
          'Initializers', 'Bound methods'
        ],
        'done': False,
      },
      {
        'name': 'Superclasses',
        'topics': [
          'Method inheritance', 'Super invocations'
        ],
        'done': False,
      },
      {
        'name': 'Optimization',
        'topics': [
          'Benchmarking', 'Hash code masking', 'NaN tagging'
        ],
        'done': False,
      }
    ]
  },
  {
    'name': 'Backmatter',
    'chapters': [
      {
        'name': 'Appendix I',
        'topics': [],
      },
      {
        'name': 'Appendix II',
        'topics': [],
      }
    ],
  },
]


def list_code_chapters():
  """Gets the list of titles of the chapters that have code."""
  chapters = []

  def walk_part(part):
    for chapter in part['chapters']:
      chapters.append(chapter['name'])

  walk_part(TOC[2])
  walk_part(TOC[3])
  walk_part(TOC[4])

  return chapters

CODE_CHAPTERS = list_code_chapters()


def roman(n):
  """Convert n to roman numerals."""
  if n <= 3:
    return "I" * n
  elif n == 4:
    return "IV"
  elif n < 10:
    return "V" + "I" * (n - 5)
  else:
    raise "Can't convert " + str(n) + " to Roman."

def number_chapters():
  """Determine the part or chapter numbers for each part or chapter."""
  numbers = {}
  part_num = 1
  chapter_num = 1
  in_matter = False
  for part in TOC:
    # Front- and backmatter have no names, pages, or numbers.
    in_matter = part['name'] == '' or part['name'] == 'Backmatter'
    if not in_matter:
      numbers[part['name']] = roman(part_num)
      part_num += 1

    for chapter in part['chapters']:
      if in_matter:
        # Front- and backmatter chapters are specially numbered.
        name = chapter['name']
        if name == 'Appendix I':
          numbers[chapter['name']] = 'A1'
        elif name == 'Appendix II':
          numbers[chapter['name']] = 'A2'
        else:
          numbers[chapter['name']] = ''
      else:
        numbers[chapter['name']] = chapter_num
        chapter_num += 1

  return numbers

NUMBERS = number_chapters()


def flatten_pages():
  """Flatten the tree of parts and chapters to a single linear list of pages."""
  pages = []
  types = []
  for part in TOC:
    # There are no part pages for the front- and backmatter.
    if part['name']:
      pages.append(part['name'])
      types.append('Part')

    for chapter in part['chapters']:
      pages.append(chapter['name'])
      types.append('Chapter')

  return pages, types

PAGES, TYPES = flatten_pages()


def adjacent_page(title, offset):
  '''The title of the page [offset] pages before or after [title].'''
  page_index = PAGES.index(title) + offset
  if page_index < 0 or page_index >= len(PAGES): return None

  return PAGES[page_index]


def adjacent_type(title, offset):
  '''Generate template data to link to the previous or next page.'''
  page_index = PAGES.index(title) + offset
  if page_index < 0 or page_index >= len(PAGES): return None

  return TYPES[page_index]


def chapter_number(name):
  """
  Given the name of a chapter (or part of end matter page), finds its number.
  """
  if name in NUMBERS:
    return NUMBERS[name]

  # The chapter has no number. This is true for the appendices.
  return ""


def get_language(name):
  if CODE_CHAPTERS.index(name) < CODE_CHAPTERS.index("Chunks of Bytecode"):
    return "java"
  return "c"


def get_file_name(title):
  """
  Given a title like "Hash Tables", converts it to the corresponding file
  name like "hash-tables".
  """
  if title == "Crafting Interpreters":
    return "index"
  if title == "Table of Contents":
    return "contents"

  # Hack. The introduction has a *subheader* named "Challenges" distinct from
  # the challenges section. This function here is also used to generate the
  # anchor names for the links, so handle that one specially so it doesn't
  # collide with the real "Challenges" section.
  if title == "Challenges":
    return "challenges_"

  title = title.lower().replace(" ", "-")
  title = re.sub(r'[,.?!:/"]', '', title)
  return title


def get_markdown_path(title):
  return os.path.join('book', get_file_name(title) + '.md')


def get_short_name(name):
  number = chapter_number(name)

  first_word = name.split()[0].lower().replace(',', '')
  if first_word == "a" or first_word == "the":
    first_word = name.split()[1].lower()
  if first_word == "user-defined":
    first_word = "user"

  return "chap{0:02d}_{1}".format(number, first_word)


@total_ordering
class SnippetTag:
  def __init__(self, chapter, name, index):
    self.chapter = chapter
    self.name = name
    self.chapter_index = chapter_number(chapter)
    self.index = index

    # Hackish. Always want "not-yet" to be the last tag even if it appears
    # before a real tag. That ensures we can push it for other tags that have
    # been named.
    if name == "not-yet":
      self.index = 9999

  def __eq__(self, other):
      return (isinstance(other, SnippetTag) and
          self.chapter_index == other.chapter_index and
          self.index < other.index)

  def __lt__(self, other):
    if self.chapter_index != other.chapter_index:
      return self.chapter_index < other.chapter_index

    return self.index < other.index

  def __repr__(self):
    return "Tag({}|{}: {} {})".format(
        self.chapter_index, self.index, self.chapter, self.name)


def get_chapter_snippet_tags():
  """
  Parses the snippet tags from every chapter. Returns a map of chapter names
  to maps of snippet names to SnippetTags.
  """
  chapters = {}

  for chapter in CODE_CHAPTERS:
    chapters[chapter] = get_snippet_tags(get_markdown_path(chapter))

  # for chapter, tags in chapters.items():
  #   print(chapter)
  #   for tag in tags.values():
  #     print("  {}".format(tag))

  return chapters


def get_snippet_tags(path):
  """
  Parses the Markdown file at [path] and finds all of the `^code` tags.
  Returns a map of snippet names to SnippetTags for them.
  """
  tags = {}

  with open(path, 'r') as input:
    title = None

    for line in input:
      match = TITLE_PATTERN.match(line)
      if match:
        title = match.group(1)

      match = SNIPPET_TAG_PATTERN.match(line)
      if match:
        if title == None:
          raise Exception("Should have found title first.")
        tags[match.group(1)] = SnippetTag(title, match.group(1), len(tags))

  return tags
