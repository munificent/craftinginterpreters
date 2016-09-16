#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import codecs
import glob
import os
import posixpath
import re
import subprocess
import sys
import time
import urllib
from http.server import HTTPServer, SimpleHTTPRequestHandler

import jinja2
import markdown


GRAY = '\033[1;30m'
GREEN = '\033[32m'
RED = '\033[31m'
DEFAULT = '\033[0m'
PINK = '\033[91m'
YELLOW = '\033[33m'

TOC = [
  {
    'name': '',
    'chapters': [
      {
        'name': 'Welcome',
        'topics': [],
      },
      {
        'name': 'Table of Contents',
        'topics': [],
      }
    ],
  },
  {
    'name': 'The Lay of the Land',
    'chapters': [
      {
        'name': 'Introduction',
        'topics': ['Why learn programming languages?', 'Interpreters and compilers', 'Phases of a compiler'],
      },
      {
        'name': 'The Pancake Language',
        'topics': ['Splitting into tokens', 'Kinds of errors', 'REPL', 'Reading files', 'Validating code', 'Running programs'],
      },
      {
        'name': 'The Lox Language',
        'topics': ['Lexical grammars', 'Grammars', 'Extended Backus-Naur Form'],
      }
    ]
  },
  {
    'name': 'The View from the Top',
    'chapters': [
      {
        'name': 'Scanning',
        'topics': ['Tokens', 'Token types', 'Lexical analysis', 'Regular languages', 'Lookahead', 'Reserved words', 'Error reporting'],
        'done': False,
      },
      {
        'name': 'Representing Code',
        'topics': ['Abstract syntax trees', 'Expression trees', 'Generating AST classes', 'The Visitor pattern', 'Pretty printing'],
        'done': False,
      },
      {
        'name': 'Parsing Expressions',
        'topics': ['Expression nodes', 'Recursive descent', 'Precedence', 'Associativity', 'Primary expressions', 'Syntax errors'],
        'done': False,
      },
      {
        'name': 'Evaluating Expressions',
        'topics': ['The Interpreter pattern', 'Tree-walk interpretation', 'Subexpressions', 'Runtime errors', 'Type checking', 'Truthiness'],
        'done': False,
      },
      {
        'name': 'Statements and State',
        'topics': ['Statement nodes', 'Blocks', 'Expression statements', 'Variables', 'Assignment', 'Lexical scope', 'Environments'],
        'done': False,
      },
      {
        'name': 'Control Flow',
        'topics': ['If statements', 'While statements', 'For statements', 'Desugaring', 'Logical operators', 'Short-circuit evaluation'],
        'done': False,
      },
      {
        'name': 'Functions',
        'topics': ['Function declarations', 'Formal parameters', 'Call expressions', 'Arguments', 'Return statements', 'Function objects', 'Closures', 'Arity', 'Native functions'],
        'done': False,
      },
      {
        'name': 'Resolving and Binding',
        'topics': ['Name resolution', 'Early binding', 'Static errors'],
        'done': False,
      },
      {
        'name': 'Classes',
        'topics': ['Class declarations', 'Fields', 'Properties', 'Get and set expressions', 'Constructors', 'Initializers', 'this', 'Method references'],
        'done': False,
      },
      {
        'name': 'Inheritance',
        'topics': ['Superclasses', 'Overriding', 'Calling superclass methods'],
        'done': False,
      }
    ]
  },
  {
    'name': 'The Long Way Down',
    'chapters': [
      {
        'name': 'Chunks of Bytecode',
        'topics': [],
        'done': False,
      },
      {
        'name': 'A Virtual Machine',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Scanning on Demand',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Compiling Expressions',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Types of Values',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Strings',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Hash Tables',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Global Variables',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Local Variables',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Jumping Forward and Back',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Function Calls',
        'topics': [],
        'done': False,
      },
      {
        'name': 'User-Defined Functions',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Closures',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Garbage Collection',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Classes and Instances',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Methods and Initializers',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Superclasses',
        'topics': [],
        'done': False,
      },
      {
        'name': 'Optimization',
        'topics': [],
        'done': False,
      }
    ]
  },
  {
    'name': '',
    'chapters': [
      {
        'name': 'Glossary',
        'topics': [],
        'done': False,
      }
    ]
  }
]


def flatten_pages():
  """Flatten the tree of parts and chapters to a single linear list of pages."""
  pages = []
  for part in TOC:
    # There are no part pages for the front- and backmatter.
    if part['name']:
      pages.append(part['name'])

    for chapter in part['chapters']:
      pages.append(chapter['name'])

  return pages

PAGES = flatten_pages()


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
    in_matter = part['name'] == ''
    if not in_matter:
      numbers[part['name']] = roman(part_num)
      part_num += 1

    for chapter in part['chapters']:
      if in_matter:
        # Front- and backmatter chapters are not numbered.
        numbers[chapter['name']] = ''
      else:
        numbers[chapter['name']] = str(chapter_num)
        chapter_num += 1

  return numbers

NUMBERS = number_chapters()


num_chapters = 0
empty_chapters = 0
total_words = 0


class RootedHTTPServer(HTTPServer):
  """Simple server that resolves paths relative to a given directory.

  From: http://louistiao.me/posts/python-simplehttpserver-recipe-serve-specific-directory/
  """
  def __init__(self, base_path, *args, **kwargs):
    HTTPServer.__init__(self, *args, **kwargs)
    self.RequestHandlerClass.base_path = base_path


class RootedHTTPRequestHandler(SimpleHTTPRequestHandler):
  """Simple handler that resolves paths relative to a given directory.

  From: http://louistiao.me/posts/python-simplehttpserver-recipe-serve-specific-directory/
  """
  def translate_path(self, path):
    # Refresh on request.
    format_files(True)
    build_sass(True)

    path = posixpath.normpath(urllib.parse.unquote(path))
    words = path.split('/')
    words = filter(None, words)
    path = self.base_path
    for word in words:
      drive, word = os.path.splitdrive(word)
      head, word = os.path.split(word)
      if word in (os.curdir, os.pardir):
        continue
      path = os.path.join(path, word)
    return path


def title_to_file(title):
  '''Given a title like "Hash Tables", converts it to the corresponding file
     name like "hash-tables".'''
  if title == "Welcome":
    return "index"
  if title == "Table of Contents":
    return "contents"

  title = title.lower().replace(" ", "-")
  title = re.sub(r'[,.?!:/"]', '', title)
  return title


def adjacent_page(title, offset):
  '''Generate template data to link to the previous or next page.'''
  page_index = PAGES.index(title) + offset
  if page_index < 0 or page_index >= len(PAGES): return None

  return PAGES[page_index]


def pretty(text):
  '''Use nicer HTML entities and special characters.'''
  text = text.replace(" -- ", "&#8202;&mdash;&#8202;")
  text = text.replace("à", "&agrave;")
  text = text.replace("ï", "&iuml;")
  text = text.replace("ø", "&oslash;")
  text = text.replace("æ", "&aelig;")
  return text


def look_up_chapters(title):
  """If [title] is the title of a part, returns the number of the first
     chapter in the part, and the list of chapter names."""
  first_chapter = 0
  chapters = []
  for part in TOC:
    if title == part['name']:
      first_chapter = NUMBERS[part['chapters'][0]['name']]
      for chapter in part['chapters']:
        chapters.append(chapter['name'])
      break

  return first_chapter, chapters


def format_file(path, skip_up_to_date, templates_mod):
  basename = os.path.basename(path)
  basename = basename.split('.')[0]

  output_path = "site/" + basename + ".html"

  # See if the HTML is up to date.
  if skip_up_to_date:
    source_mod = max(os.path.getmtime(path), templates_mod)
    # if os.path.exists(cpp_path(basename)):
    #   source_mod = max(source_mod, os.path.getmtime(cpp_path(basename)))

    dest_mod = os.path.getmtime(output_path)

    if source_mod < dest_mod:
      return

  title = ''
  title_html = ''
  part = None
  template_file = 'page'
  # isoutline = False

  sections = []

  # Read the markdown file and preprocess it.
  contents = ''
  with open(path, 'r') as input:
    # Read each line, preprocessing the special codes.
    for line in input:
      stripped = line.lstrip()
      indentation = line[:len(line) - len(stripped)]

      if stripped.startswith('^'):
        command,_,args = stripped.rstrip('\n').lstrip('^').partition(' ')
        args = args.strip()

        if command == 'title':
          title = args
          title_html = title

          # Remove any discretionary hyphens from the title.
          title = title.replace('&shy;', '')
        elif command == 'part':
          part = args
        elif command == 'template':
          template_file = args
        # elif command == 'code':
        #   contents = contents + include_code(basename, args, indentation)
        # elif command == 'outline':
        #   isoutline = True
        # else:
        #   print "UNKNOWN COMMAND:", command, args

      elif stripped.startswith('#'):
        # Build the section navigation from the headers.
        index = stripped.find(" ")
        header_type = stripped[:index]
        header = pretty(stripped[index:].strip())
        anchor = title_to_file(header)
        anchor = re.sub(r'[.?!:/"]', '', anchor)

        # Add an anchor to the header.
        contents += indentation + header_type
        contents += '<a href="#' + anchor + '" name="' + anchor + '">' + header + '</a>\n'

        # Build the section navigation.
        if len(header_type) == 2:
          sections.append(header)

      else:
        contents += pretty(line)

  first_chapter, chapters = look_up_chapters(title)

  # title_text = title
  # section_header = ""

  # if section != "":
  #   title_text = title + " &middot; " + section
  #   section_href = section.lower().replace(" ", "-")
  #   section_header = '<span class="section"><a href="{}.html">{}</a></span>'.format(
  #     section_href, section)

  # Allow processing markdown inside asides.
  contents = contents.replace('<aside', '<aside markdown="1"')
  body = markdown.markdown(contents, ['extra', 'codehilite', 'smarty'])
  body = body.replace('<aside markdown="1"', '<aside')

  data = {
    'title': title,
    'part': part,
    'body': body,
    'sections': sections,
    'chapters': chapters,
    'first_chapter': first_chapter,
    'number': NUMBERS[title],
    'prev': adjacent_page(title, -1),
    'next': adjacent_page(title, 1),
    'toc': TOC
  }

  template = environment.get_template(template_file + '.html')
  output = template.render(data)

  # Write the output.
  with codecs.open(output_path, "w", encoding="utf-8") as out:
    out.write(output)

  global total_words
  global num_chapters
  global empty_chapters

  word_count = len(contents.split(None))
  num = NUMBERS[title]
  if num:
    num += '. '

  # Non-chapter pages aren't counted like regular chapters.
  if part:
    num_chapters += 1
    if word_count < 50:
      empty_chapters += 1
      print("    {}{}{}{}".format(GRAY, num, title, DEFAULT))
    elif word_count < 2000:
      empty_chapters += 1
      print("  {}-{} {}{} ({} words)".format(
        YELLOW, DEFAULT, num, title, word_count))
    else:
      total_words += word_count
      print("  {}✓{} {}{} ({} words)".format(
        GREEN, DEFAULT, num, title, word_count))
  elif title in ["Welcome", "Table of Contents", "Glossary"]:
    print("{}•{} {}{}".format(
      GREEN, DEFAULT, num, title))
  else:
    if word_count < 50:
      print("  {}{}{}{}".format(GRAY, num, title, DEFAULT))
    else:
      print("{}✓{} {}{} ({} words)".format(
        GREEN, DEFAULT, num, title, word_count))


def format_files(skip_up_to_date):
  '''Process each markdown file.'''

  # See if any of the templates were modified. If so, all pages will be rebuilt.
  templates_mod = None
  for template in glob.iglob("asset/template/*.html"):
    template_mod = os.path.getmtime(template)
    if not templates_mod: templates_mod = template_mod
    templates_mod = max(templates_mod, template_mod)

  for page in PAGES:
    file = os.path.join('book', title_to_file(page) + '.md')
    format_file(file, skip_up_to_date, templates_mod)


def build_sass(skip_up_to_date):
  '''Process each SASS file.'''
  for source in glob.iglob("asset/*.scss"):
    dest = "site/" + os.path.basename(source).split(".")[0] + ".css"

    if skip_up_to_date:
      source_mod = os.path.getmtime(source)
      dest_mod = os.path.getmtime(dest)
      if source_mod < dest_mod:
        continue

    subprocess.call(['sass', source, dest])
    print("{}•{} {}".format(GREEN, DEFAULT, source))


def run_server():
  port = 8000
  handler = RootedHTTPRequestHandler
  server = RootedHTTPServer("site", ('', port), handler)

  print('Serving at port', port)
  server.serve_forever()


environment = jinja2.Environment(
    loader=jinja2.FileSystemLoader('asset/template'))

environment.filters['file'] = title_to_file

if len(sys.argv) == 2 and sys.argv[1] == "--watch":
  run_server()
  while True:
    format_files(True)
    build_sass(True)
    time.sleep(0.3)
if len(sys.argv) == 2 and sys.argv[1] == "--serve":
  run_server()
else:
  format_files(False)
  build_sass(False)

  average_word_count = total_words // (num_chapters - empty_chapters)
  estimated_word_count = total_words + (empty_chapters * average_word_count)
  percent_finished = total_words * 100 // estimated_word_count

  print("{}/~{} words ({}%)".format(
    total_words, estimated_word_count, percent_finished))
