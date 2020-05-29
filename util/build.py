#!./util/env/bin/python3
# -*- coding: utf-8 -*-

import codecs
import datetime
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
import term

CODE_OPTIONS_PATTERN = re.compile(r'([-a-z0-9]+) \(([^)]+)\)')
BEFORE_PATTERN = re.compile(r'(\d+) before')
AFTER_PATTERN = re.compile(r'(\d+) after')

ASIDE_COMMENT_PATTERN = re.compile(r' ?<span class="c1">// \[([-a-z0-9]+)\] *</span>')
ASIDE_WITH_COMMENT_PATTERN = re.compile(r' ?<span class="c1">// (.+) \[([-a-z0-9]+)\] *</span>')
# The "(?!-)" is a hack. scanning.md has an inline code sample containing a
# "--" operator. We don't want that to get matched, so fail the match if the
# character after the "-- " is "-", which is the next character in the code
# sample.
EM_DASH_PATTERN = re.compile(r'\s+--\s(?!-)')

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
      for chapter in part['chapters']:
        chapter_number = book.chapter_number(chapter['name'])
        chapters.append([chapter_number, chapter['name']])
      break

  return chapters


def format_code(language, length, lines):
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
    markup += line.ljust(length, ' ') + '\n'

  markup += '```'

  html = markdown.markdown(markup, extensions=['extra', 'codehilite'])

  if leading_newlines > 0:
    html = html.replace('<pre>', '<pre>' + ('<br>' * leading_newlines))

  if trailing_newlines > 0:
    html = html.replace('</pre>', ('<br>' * trailing_newlines) + '</pre>')

  # Strip off the div wrapper. We just want the <pre>.
  html = html.replace('<div class="codehilite">', '')
  html = html.replace('</div>', '')
  return html


def longest_line(longest, lines):
  """ Returns the length of the longest line in lines, or [longest], whichever
  is longer.
  """
  for line in lines:
    longest = max(longest, len(line))

  return longest


def insert_snippet(snippets, arg, contents, errors):
  # NOTE: If you change this, be sure to update the baked in example snippet
  # in introduction.md.
  name = None
  show_location = True
  before_lines = 0
  after_lines = 0

  match = CODE_OPTIONS_PATTERN.match(arg)
  if (match):
    name = match.group(1)
    options = match.group(2).split(', ')

    for option in options:
      if option == "no location":
        show_location = False

      match = BEFORE_PATTERN.match(option)
      if match:
        before_lines = int(match.group(1))

      match = AFTER_PATTERN.match(option)
      if match:
        after_lines = int(match.group(1))
  else:
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

  location = []
  if show_location:
    location = snippet.describe_location()

  # Make sure every snippet shows the reader where it goes.
  if (show_location and len(location) <= 1
      and before_lines == 0 and after_lines == 0):
    print("No location or context for {}".format(name))
    errors.append("No location or context for {}".format(name))
    contents += "**ERROR: No location or context for {}**\n".format(name)
    return contents

  # TODO: Show indentation in snippets somehow.

  # Figure out the length of the longest line. We pad all of the snippets to
  # this length so that the background on the pre sections is as wide as the
  # entire chunk of code.
  length = 0
  if before_lines > 0:
    length = longest_line(length, snippet.context_before[-before_lines:])
  if snippet.removed and not snippet.added:
    length = longest_line(length, snippet.removed)
  if snippet.added_comma:
    length = longest_line(length, snippet.added_comma)
  if snippet.added:
    length = longest_line(length, snippet.added)
  if after_lines > 0:
    length = longest_line(length, snippet.context_after[:after_lines])

  contents += '<div class="codehilite">'

  if before_lines > 0:
    before = format_code(snippet.file.language(), length,
        snippet.context_before[-before_lines:])
    if snippet.added:
      before = before.replace('<pre>', '<pre class="insert-before">')
    contents += before

  if snippet.added_comma:
    def replace_last(string, old, new):
      return new.join(string.rsplit(old, 1))

    comma = format_code(snippet.file.language(), length, [snippet.added_comma])
    comma = comma.replace('<pre>', '<pre class="insert-before">')
    comma = replace_last(comma, ',', '<span class="insert-comma">,</span>')
    contents += comma

  if show_location:
    contents += '<div class="source-file">{}</div>\n'.format(
        '<br>\n'.join(location))

  if snippet.removed and not snippet.added:
    removed = format_code(snippet.file.language(), length, snippet.removed)
    removed = removed.replace('<pre>', '<pre class="delete">')
    contents += removed

  if snippet.added:
    added = format_code(snippet.file.language(), length, snippet.added)
    if before_lines > 0 or after_lines > 0:
      added = added.replace('<pre>', '<pre class="insert">')
    contents += added

  if after_lines > 0:
    after = format_code(snippet.file.language(), length,
        snippet.context_after[:after_lines])
    if snippet.added:
      after = after.replace('<pre>', '<pre class="insert-after">')
    contents += after

  contents += '</div>'

  if show_location:
    contents += '<div class="source-file-narrow">{}</div>\n'.format(
        ', '.join(location))

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

      if line.startswith('^'):
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

      elif stripped.startswith('# ') or stripped.startswith('## ') or stripped.startswith('### '):
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
    if name != 'not-yet' and name != 'omit' and snippet != False:
      errors.append("Unused snippet {}".format(name))

  # Show any errors at the top of the file.
  if errors:
    error_markdown = ""
    for error in errors:
      error_markdown += "**Error: {}**\n\n".format(error)
    contents = error_markdown + contents

  # Fix up em dashes. We do this on the entire contents instead of in pretty()
  # so that we can handle surrounding whitespace even when the "--" is at the
  # beginning of end of a line in Markdown.
  contents = EM_DASH_PATTERN.sub('<span class="em">&mdash;</span>', contents)

  # Allow processing markdown inside some tags.
  contents = contents.replace('<aside', '<aside markdown="1"')
  contents = contents.replace('<div class="challenges">', '<div class="challenges" markdown="1">')
  contents = contents.replace('<div class="design-note">', '<div class="design-note" markdown="1">')
  body = markdown.markdown(contents, extensions=['extra', 'codehilite', 'smarty'])

  # Turn aside markers in code into spans. In the empty span case, insert a
  # zero-width space because Chrome seems to lose the span's position if it has
  # no content.
  # <span class="c1">// [repl]</span>
  body = ASIDE_COMMENT_PATTERN.sub(r'<span name="\1"> </span>', body)
  body = ASIDE_WITH_COMMENT_PATTERN.sub(r'<span class="c1" name="\2">// \1</span>', body)

  # TODO: Temporary code. Apply some clean-up to better match the output of the
  # new Dart build system.

  # Unify some code hilite classes that look the same.
  body = body.replace('<span class="p">', '<span class="o">')
  body = body.replace('<span class="kd">', '<span class="k">')
  body = body.replace('<span class="kt">', '<span class="k">')
  body = body.replace('<span class="kn">', '<span class="k">')
  body = body.replace('<span class="kr">', '<span class="k">')
  body = body.replace('<span class="na">', '<span class="n">')
  body = body.replace('<span class="nn">', '<span class="n">')
  body = body.replace('<span class="nf">', '<span class="n">')
  body = body.replace('<span class="nx">', '<span class="n">')
  body = body.replace('<span class="vg">', '<span class="nc">')
  body = body.replace('<span class="sc">', '<span class="s">')
  body = body.replace('<span class="s1">', '<span class="s">')
  body = body.replace('<span class="s2">', '<span class="s">')

  # Pygments puts empty spans at the end of preprocessor lines.
  body = body.replace('<span class="cp"></span>', '')

  up = 'Table of Contents'
  if part:
    up = part
  elif title == 'Table of Contents':
    up = 'Crafting Interpreters'

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
    'prev_type': book.adjacent_type(title, -1),
    'next': book.adjacent_page(title, 1),
    'next_type': book.adjacent_type(title, 1),
    'up': up,
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
      print("    " + term.gray("{}{}".format(num, title)))
    elif part != "Backmatter" and word_count < 2000:
      empty_chapters += 1
      print("  {} {}{} ({} words)".format(
          term.yellow("-"), num, title, word_count))
    else:
      total_words += word_count
      print("  {} {}{} ({} words)".format(
          term.green("✓"), num, title, word_count))
  elif title in ["Crafting Interpreters", "Table of Contents"]:
    print("{} {}{}".format(term.green("•"), num, title))
  else:
    if word_count < 50:
      print("    " + term.gray("{}{}".format(num, title)))
    else:
      print("{} {}{} ({} words)".format(
          term.green("✓"), num, title, word_count))


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
    print("{} {}".format(term.green("•"), source))


def run_server():
  port = 8000
  handler = RootedHTTPRequestHandler
  server = RootedHTTPServer("site", ('localhost', port), handler)

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

  first_writing_day = datetime.date(2016, 9, 30)
  today = datetime.date.today()
  writing_days = (today - first_writing_day).days
  estimated_writing_days = int(writing_days * (1.0 - (percent_finished / 100)))
  estimated_end_date = today + datetime.timedelta(days=estimated_writing_days)

  print("{}/~{} words, {}% done, estimated to complete on {}".format(
      total_words,
      estimated_word_count,
      percent_finished,
      estimated_end_date))
