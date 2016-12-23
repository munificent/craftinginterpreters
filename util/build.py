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

import book
import code_snippets

GRAY = '\033[1;30m'
GREEN = '\033[32m'
RED = '\033[31m'
DEFAULT = '\033[0m'
PINK = '\033[91m'
YELLOW = '\033[33m'

CODE_BEFORE_PATTERN = re.compile(r'([-a-z0-9]+) \((\d+) before\)')
CODE_AFTER_PATTERN = re.compile(r'([-a-z0-9]+) \((\d+) after\)')
CODE_AROUND_PATTERN = re.compile(r'([-a-z0-9]+) \((\d+) before, (\d+) after\)')


num_chapters = 0
empty_chapters = 0
total_words = 0

source_code = None

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
    # Refresh files that are being requested.
    if path.endswith(".html"):
      format_files(True, path.replace(".html", "").replace("/", ""))
    if path.endswith(".css"):
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


def pretty(text):
  '''Use nicer HTML entities and special characters.'''
  text = text.replace(" -- ", "&#8202;&mdash;&#8202;")
  text = text.replace(" --\n", "&#8202;&mdash;&#8202;")
  text = text.replace("à", "&agrave;")
  text = text.replace("ï", "&iuml;")
  text = text.replace("ø", "&oslash;")
  text = text.replace("æ", "&aelig;")
  return text


def get_part_chapters(title):
  """If [title] is the title of a part, returns a list of pairs of chapter
  numbers and names."""
  chapters = []
  for part in book.TOC:
    if title == part['name']:
      chapter_number = book.chapter_number(part['chapters'][0]['name'])
      for chapter in part['chapters']:
        chapters.append([chapter_number, chapter['name']])
        chapter_number += 1
      break

  return chapters


def format_code(language, lines):
  markup = '```{}\n'.format(language)

  # Hack. Markdown seems to discard leading and trailing newlines, so we'll
  # add them back ourselves.
  leading_newlines = 0
  while lines and lines[0].strip() == '':
    lines = lines[1:]
    leading_newlines += 1

  trailing_newlines = 0
  while lines and lines[-1].strip() == '':
    lines = lines[:-1]
    trailing_newlines += 1

  for line in lines:
    markup += line + '\n'

  markup += '```'

  html = markdown.markdown(markup, ['extra', 'codehilite'])

  if leading_newlines > 0:
    html = html.replace('<pre>', '<pre>' + ('<br>' * leading_newlines))

  if trailing_newlines > 0:
    html = html.replace('</pre>', ('<br>' * trailing_newlines) + '</pre>')

  # Strip off the div wrapper. We just want the <pre>.
  html = html.replace('<div class="codehilite">', '')
  html = html.replace('</div>', '')
  return html


def insert_snippet(snippets, arg, contents, errors):
  name = None
  before_lines = 0
  after_lines = 0

  match = CODE_BEFORE_PATTERN.match(arg)
  if match:
    # "^code name (2 before)"
    name = match.group(1)
    before_lines = int(match.group(2))

  match = CODE_AFTER_PATTERN.match(arg)
  if match:
    # "^code name (2 after)"
    name = match.group(1)
    after_lines = int(match.group(2))

  match = CODE_AROUND_PATTERN.match(arg)
  if match:
    # "^code name (1 before, 2 after)"
    name = match.group(1)
    before_lines = int(match.group(2))
    after_lines = int(match.group(3))

  if not name:
    # Otherwise, the arg is just the name of the snippet.
    name = arg

  if name not in snippets:
    errors.append("Undefined snippet {}".format(name))
    contents += "**ERROR: Missing snippet {}**\n".format(name)
    return contents

  if snippets[name] == False:
    errors.append("Reused snippet {}".format(name))
    contents += "**ERROR: Reused snippet {}**\n".format(name)
    return contents

  snippet = snippets[name]

  # Consume it.
  snippets[name] = False

  # TODO: Show indentation in snippets somehow.

  contents += '<div class="codehilite">'

  if before_lines > 0:
    before = format_code(snippet.file.language(),
        snippet.context_before[-before_lines:])
    before = before.replace('<pre>', '<pre class="insert-before">')
    contents += before

  where = '<em>{}</em>'.format(snippet.file.nice_path())

  if snippet.location():
    where += '<br>\n{}'.format(snippet.location())

  if snippet.removed and snippet.added:
    where += '<br>\nreplace {} line{}'.format(
        len(snippet.removed), '' if len(snippet.removed) == 1 else 's')
  contents += '<div class="source-file">{}</div>\n'.format(where)

  if snippet.removed and not snippet.added:
    removed = format_code(snippet.file.language(), snippet.removed)
    removed = removed.replace('<pre>', '<pre class="delete">')
    contents += removed

  if snippet.added:
    added = format_code(snippet.file.language(), snippet.added)
    if before_lines > 0 or after_lines > 0:
      added = added.replace('<pre>', '<pre class="insert">')
    contents += added

  if after_lines > 0:
    after = format_code(snippet.file.language(),
        snippet.context_after[:after_lines])
    after = after.replace('<pre>', '<pre class="insert-after">')
    contents += after

  contents += '</div>'

  return contents


def format_file(path, skip_up_to_date, dependencies_mod):
  basename = os.path.basename(path)
  basename = basename.split('.')[0]

  output_path = "site/" + basename + ".html"

  # See if the HTML is up to date.
  if skip_up_to_date:
    source_mod = max(os.path.getmtime(path), dependencies_mod)
    dest_mod = os.path.getmtime(output_path)

    if source_mod < dest_mod:
      return

  title = ''
  title_html = ''
  part = None
  template_file = 'page'

  errors = []
  sections = []
  header_index = 0
  subheader_index = 0
  has_challenges = False
  design_note = None
  snippets = None

  # Read the markdown file and preprocess it.
  contents = ''
  with open(path, 'r') as input:
    # Read each line, preprocessing the special codes.
    for line in input:
      stripped = line.lstrip()
      indentation = line[:len(line) - len(stripped)]

      if stripped.startswith('^'):
        command,_,arg = stripped.rstrip('\n').lstrip('^').partition(' ')
        arg = arg.strip()

        if command == 'title':
          title = arg
          title_html = title

          # Remove any discretionary hyphens from the title.
          title = title.replace('&shy;', '')

          # Load the code snippets now that we know the title.
          snippets = source_code.find_all(title)

          # If there were any errors loading the code, include them.
          if title in book.CODE_CHAPTERS:
            errors.extend(source_code.errors[title])
        elif command == 'part':
          part = arg
        elif command == 'template':
          template_file = arg
        elif command == 'code':
          contents = insert_snippet(snippets, arg, contents, errors)
        else:
          raise Exception('Unknown command "^{} {}"'.format(command, arg))

      elif stripped.startswith('## Challenges'):
        has_challenges = True
        contents += '<h2><a href="#challenges" name="challenges">Challenges</a></h2>\n'

      elif stripped.startswith('## Design Note:'):
        has_design_note = True
        design_note = stripped[len('## Design Note:') + 1:]
        contents += '<h2><a href="#design-note" name="design-note">Design Note: {}</a></h2>\n'.format(design_note)

      elif stripped.startswith('#') and not stripped.startswith('####'):
        # Build the section navigation from the headers.
        index = stripped.find(" ")
        header_type = stripped[:index]
        header = pretty(stripped[index:].strip())
        anchor = book.get_file_name(header)
        anchor = re.sub(r'[.?!:/"]', '', anchor)

        # Add an anchor to the header.
        contents += indentation + header_type

        if len(header_type) == 2:
          header_index += 1
          subheader_index = 0
          page_number = book.chapter_number(title)
          number = '{0}&#8202;.&#8202;{1}'.format(page_number, header_index)
        elif len(header_type) == 3:
          subheader_index += 1
          page_number = book.chapter_number(title)
          number = '{0}&#8202;.&#8202;{1}&#8202;.&#8202;{2}'.format(page_number, header_index, subheader_index)

        header_line = '<a href="#{0}" name="{0}"><small>{1}</small> {2}</a>\n'.format(anchor, number, header)
        contents += header_line

        # Build the section navigation.
        if len(header_type) == 2:
          sections.append([header_index, header])

      else:
        contents += pretty(line)

  # Validate that every snippet for the chapter is included.
  for name, snippet in snippets.items():
    if name != 'not-yet' and snippet != False:
      errors.append("Unused snippet {}".format(name))

  # Show any errors at the top of the file.
  if errors:
    error_markdown = ""
    for error in errors:
      error_markdown += "**Error: {}**\n\n".format(error)
    contents = error_markdown + contents

  # Allow processing markdown inside some tags.
  contents = contents.replace('<aside', '<aside markdown="1"')
  contents = contents.replace('<div class="challenges">', '<div class="challenges" markdown="1">')
  contents = contents.replace('<div class="design-note">', '<div class="design-note" markdown="1">')
  body = markdown.markdown(contents, ['extra', 'codehilite', 'smarty'])

  data = {
    'title': title,
    'part': part,
    'body': body,
    'sections': sections,
    'chapters': get_part_chapters(title),
    'design_note': design_note,
    'has_challenges': has_challenges,
    'number': book.chapter_number(title),
    'prev': book.adjacent_page(title, -1),
    'next': book.adjacent_page(title, 1),
    'toc': book.TOC
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
  num = book.chapter_number(title)
  if num:
    num = '{}. '.format(num)

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
  elif title in ["Crafting Interpreters", "Table of Contents"]:
    print("{}•{} {}{}".format(
      GREEN, DEFAULT, num, title))
  else:
    if word_count < 50:
      print("  {}{}{}{}".format(GRAY, num, title, DEFAULT))
    else:
      print("{}✓{} {}{} ({} words)".format(
        GREEN, DEFAULT, num, title, word_count))


def latest_mod(glob_pattern):
  ''' Returns the mod time of the most recently modified file match
      [glob_pattern].
  '''
  latest = None
  for file in glob.iglob(glob_pattern):
    file_mod = os.path.getmtime(file)
    if not latest: latest = file_mod
    latest = max(latest, file_mod)
  return latest


last_code_load_time = None

def format_files(skip_up_to_date, one_file=None):
  '''Process each markdown file.'''

  code_mod = max(
      latest_mod("c/*.c"),
      latest_mod("c/*.h"),
      latest_mod("java/com/craftinginterpreters/tool/*.java"),
      latest_mod("java/com/craftinginterpreters/lox/*.java"))

  # Reload the source snippets if the code was changed.
  global source_code
  global last_code_load_time
  if not last_code_load_time or code_mod > last_code_load_time:
    source_code = code_snippets.load()
    last_code_load_time = time.time()

  # See if any of the templates were modified. If so, all pages will be rebuilt.
  templates_mod = latest_mod("asset/template/*.html")

  for page in book.PAGES:
    page_file = book.get_file_name(page)
    if one_file == None or page_file == one_file:
      file = book.get_markdown_path(page)
      format_file(file, skip_up_to_date, max(code_mod, templates_mod))


def build_sass(skip_up_to_date):
  '''Process each SASS file.'''
  imports_mod = None
  for source in glob.iglob("asset/sass/*.scss"):
    import_mod = os.path.getmtime(source)
    if not imports_mod: imports_mod = import_mod
    imports_mod = max(imports_mod, import_mod)

  for source in glob.iglob("asset/*.scss"):
    dest = "site/" + os.path.basename(source).split(".")[0] + ".css"

    if skip_up_to_date:
      source_mod = max(os.path.getmtime(source), imports_mod)
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
    loader=jinja2.FileSystemLoader('asset/template'),
    lstrip_blocks=True,
    trim_blocks=True)

environment.filters['file'] = book.get_file_name

if len(sys.argv) == 2 and sys.argv[1] == "--watch":
  run_server()
  while True:
    format_files(True)
    build_sass(True)
    time.sleep(0.3)
if len(sys.argv) == 2 and sys.argv[1] == "--serve":
  format_files(True)
  run_server()
else:
  format_files(False)
  build_sass(False)

  average_word_count = total_words // (num_chapters - empty_chapters)
  estimated_word_count = total_words + (empty_chapters * average_word_count)
  percent_finished = total_words * 100 // estimated_word_count

  print("{}/~{} words ({}%)".format(
    total_words, estimated_word_count, percent_finished))
