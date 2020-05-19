import 'dart:io';

import 'package:glob/glob.dart';
import 'package:mustache_template/mustache_template.dart';
import 'package:path/path.dart' as p;
import 'package:sass/sass.dart' as sass;

import 'package:tool/src/book.dart';
import 'package:tool/src/markdown.dart';
import 'package:tool/src/source_code.dart';
import 'package:tool/src/text.dart';

// The "(?!-)" is a hack. scanning.md has an inline code sample containing a
// "--" operator. We don't want that to get matched, so fail the match if the
// character after the "-- " is "-", which is the next character in the code
// sample.
final _emDashPattern = RegExp(r"\s+--\s(?!-)");

final _codeOptionsPattern = RegExp(r"([-a-z0-9]+) \(([^)]+)\)");
final _beforePattern = RegExp(r"(\d+) before");
final _afterPattern = RegExp(r"(\d+) after");

void main(List<String> arguments) {
  formatFiles();
//  for (var entry in Directory("book").listSync()) {
//    if (entry.path.endsWith(".md")) {
//      print(entry.path);
//    }
//  }

//  buildSass(skipUpToDate: true);

  /*
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

  print("{} words".format(total_words))
   */
}

/// Process each Markdown file.
void formatFiles({bool skipUpToDate = false}) {
  // TODO: Support formatting a single file.
/*
def format_files(skip_up_to_date, one_file=None):
  code_mod = max(
      latest_mod("c/ *.c"),
      latest_mod("c/ *.h"),
      latest_mod("java/com/craftinginterpreters/tool/ *.java"),
      latest_mod("java/com/craftinginterpreters/lox/ *.java"))

  # Reload the source snippets if the code was changed.
  global source_code
  global last_code_load_time
  if not last_code_load_time or code_mod > last_code_load_time:
    source_code = code_snippets.load()
    last_code_load_time = time.time()

  # See if any of the templates were modified. If so, all pages will be rebuilt.
  templates_mod = latest_mod("asset/template/ *.html")
*/

  // TODO: Temp. Just one chapter for now.
  formatFile(Page.all[10]);
  return;

  for (var page in Page.all) {
//    if (one_file == None or page_file == one_file) {
    formatFile(page);
//      format_file(file, skip_up_to_date, max(code_mod, templates_mod))
//    }
  }
}

// TODO: Move to library.
// TODO: Skip up to date stuff.
void formatFile(Page page) {
  print(page.markdownPath);
// def format_file(path, skip_up_to_date, dependencies_mod):
//
//  # See if the HTML is up to date.
//  if skip_up_to_date:
//    source_mod = max(os.path.getmtime(path), dependencies_mod)
//    dest_mod = os.path.getmtime(output_path)
//
//    if source_mod < dest_mod:
//      return
//
  var title = "TODO";
  String part;
  var templateFile = 'page';

//  errors = []
  // TODO: Something better typed.
  var sections = <Map<String, String>>[];
  var headerIndex = 0;
  var subheaderIndex = 0;
  var hasChallenges = false;
  String designNote;
//  snippets = None

  // Read the markdown file and preprocess it.
  var buffer = StringBuffer();

  // Read each line, preprocessing the special codes.
  for (var line in File(page.markdownPath).readAsLinesSync()) {
    var stripped = line.trimLeft();
    var indentation = line.substring(0, line.length - stripped.length);

    if (line.startsWith("^")) {
      var commandLine = stripped.substring(1).trim();
      var space = commandLine.indexOf(" ");
      var command = commandLine.substring(0, space);
      var argument = commandLine.substring(space + 1).trim();

      switch (command) {
        case "code":
          insertSnippet(page, buffer, argument);
//          insertSnippet(buffer, snippets, arg, errors);
          break;
        case "part":
          part = argument;
          break;
        case "template":
          templateFile = argument;
          break;
        case "title":
          title = argument;

          // Remove any discretionary hyphens from the title.
          // TODO: Still needed?
          title = title.replaceAll("&shy;", "");

//          // Load the code snippets now that we know the title.
//          snippets = source_code.find_all(title)

//          # If there were any errors loading the code, include them.
//          if title in book.CODE_CHAPTERS:
//            errors.extend(source_code.errors[title])
          break;
        default:
          throw "Unknown command '$command'.";
      }
    } else if (stripped.startsWith("## Challenges")) {
      hasChallenges = true;
      buffer.writeln('<h2><a href="#challenges" name="challenges">'
          'Challenges</a></h2>');
    } else if (stripped.startsWith("## Design Note:")) {
      designNote = stripped.substring('## Design Note:'.length + 1);
      buffer.writeln('<h2><a href="#design-note" name="design-note">'
          'Design Note: $designNote</a></h2>');
    } else if (stripped.startsWith("# ") ||
        stripped.startsWith("## ") ||
        stripped.startsWith("### ")) {
      // Build the section navigation from the headers.
      var index = stripped.indexOf(" ");
      var headerType = stripped.substring(0, index);
      var header = pretty(stripped.substring(index).trim());

      var anchor = toFileName(header);

      // Add an anchor to the header.
      buffer.write("$indentation$headerType ");

      if (headerType.length == 2) {
        headerIndex += 1;
        subheaderIndex = 0;
      } else if (headerType.length == 3) {
        subheaderIndex += 1;
      }

      var number = "${page.numberString}&#8202;.&#8202;$headerIndex";
      if (headerType.length == 3) number += "&#8202;.&#8202;$subheaderIndex";

      buffer.writeln('<a href="$anchor" name="$anchor">'
          '<small>$number</small> $header</a>\n');

      // Build the section navigation.
      if (headerType.length == 2) {
        sections.add({
          "name": header,
          "anchor": toFileName(header),
          "index": headerIndex.toString()
        });
      }
    } else {
      buffer.writeln(pretty(line));
    }
  }

//  # Validate that every snippet for the chapter is included.
//  for name, snippet in snippets.items():
//    if name != 'not-yet' and name != 'omit' and snippet != False:
//      errors.append("Unused snippet {}".format(name))
//
//  # Show any errors at the top of the file.
//  if errors:
//    error_markdown = ""
//    for error in errors:
//      error_markdown += "**Error: {}**\n\n".format(error)
//    contents = error_markdown + contents
//

  // TODO: Do this in a cleaner way.
  // Fix up em dashes. We do this on the entire contents instead of in pretty()
  // so that we can handle surrounding whitespace even when the "--" is at the
  // beginning of end of a line in Markdown.
  var contents = buffer
      .toString()
      .replaceAll(_emDashPattern, '<span class="em">&mdash;</span>');

//  # Allow processing markdown inside some tags.
//  contents = contents.replace('<aside', '<aside markdown="1"')
//  contents = contents.replace('<div class="challenges">', '<div class="challenges" markdown="1">')
//  contents = contents.replace('<div class="design-note">', '<div class="design-note" markdown="1">')

  var body = renderMarkdown(contents);

//  # Turn aside markers in code into spans. In the empty span case, insert a
//  # zero-width space because Chrome seems to lose the span's position if it has
//  # no content.
//  # <span class="c1">// [repl]</span>
//  body = ASIDE_COMMENT_PATTERN.sub(r'<span name="\1"> </span>', body)
//  body = ASIDE_WITH_COMMENT_PATTERN.sub(r'<span class="c1" name="\2">// \1</span>', body)

  var up = "Table of Contents";
  if (part != null) {
    up = part;
  } else if (title == "Table of Contents") {
    up = "Crafting Interpreters";
  }

  var data = {
    "has_title": title != null,
    "title": title,
    "has_part": part != null,
    "part": part,
    "body": body,
    "sections": sections,
    // TODO:
//    "chapters": get_part_chapters(title),
    "chapters": ["TODO", "chapters"],
    "design_note": designNote,
    "has_design_note": designNote != null,
    "has_challenges": hasChallenges,
    "has_challenges_or_design_note": hasChallenges || designNote != null,
    "has_number": page.numberString != "",
    "number": page.numberString,
    // Previous page.
    "has_prev": page.previous != null,
    "prev": page.previous?.title,
    "prev_file": page.previous?.fileName,
    "prev_type": page.previous?.type,
    // Next page.
    "has_next": page.next != null,
    "next": page.next?.title,
    "next_file": page.next?.fileName,
    "next_type": page.next?.type,
    "has_up": up != null,
    "up": up,
    "up_file": up != null ? toFileName(up) : null,
    "toc": tableOfContents
  };

  // TODO: Move to separate lib.
  var partials = <String, Template>{};
  Template resolvePartial(String partial) {
    return partials.putIfAbsent(partial, () {
      var path = p.join("asset", "mustache", "$partial.html");
      return Template(File(path).readAsStringSync(),
          name: path, partialResolver: resolvePartial);
    });
  }

  var templatePath = p.join("asset", "mustache", "$templateFile.html");
  var template = Template(File(templatePath).readAsStringSync(),
      name: templatePath, partialResolver: resolvePartial);
  var output = template.renderString(data);

  // TODO: Temp hack. Insert some whitespace to match the old Markdown.
  output = output.replaceAll("</p></aside>", "</p>\n</aside>");
  output = output.replaceAll("</p><aside", "</p>\n<aside");
  output = output.replaceAll("</div><div", "</div>\n<div");

  // Write the output.
  File(page.htmlPath).writeAsStringSync(output);

//  global total_words
//  global num_chapters
//  global empty_chapters
//
//  word_count = len(contents.split(None))
//  num = book.chapter_number(title)
//  if num:
//    num = '{}. '.format(num)
//
//  # Non-chapter pages aren't counted like regular chapters.
//  if part:
//    num_chapters += 1
//    if word_count < 50:
//      empty_chapters += 1
//      print("    " + term.gray("{}{}".format(num, title)))
//    elif part != "Backmatter" and word_count < 2000:
//      empty_chapters += 1
//      print("  {} {}{} ({} words)".format(
//          term.yellow("-"), num, title, word_count))
//    else:
//      total_words += word_count
//      print("  {} {}{} ({} words)".format(
//          term.green("✓"), num, title, word_count))
//  elif title in ["Crafting Interpreters", "Table of Contents"]:
//    print("{} {}{}".format(term.green("•"), num, title))
//  else:
//    if word_count < 50:
//      print("    " + term.gray("{}{}".format(num, title)))
//    else:
//      print("{} {}{} ({} words)".format(
//          term.green("✓"), num, title, word_count))
}

void insertSnippet(Page page, StringBuffer buffer, String name) {
  // NOTE: If you change this, be sure to update the baked in example snippet
  // in introduction.md.
//def insert_snippet(snippets, arg, contents, errors):

  // Parse the location annotations after the name, if present.
  var showLocation = true;
  var beforeLines = 0;
  var afterLines = 0;

  var match = _codeOptionsPattern.firstMatch(name);
  if (match != null) {
    name = match.group(1);
    var options = match.group(2).split(", ");

    for (var option in options) {
      if (option == "no location") {
        showLocation = false;
      } else if ((match = _beforePattern.firstMatch(option)) != null) {
        beforeLines = int.parse(match.group(1));
      } else if ((match = _afterPattern.firstMatch(option)) != null) {
        afterLines = int.parse(match.group(1));
      }
    }
  }

  // TODO: Don't create every time!
  var sourceCode = SourceCode();
  sourceCode.load();
  var snippets = sourceCode.findAll(page);
  var snippet = snippets[name];

  // TODO: Temp.
  if (snippet == null) {
    print("Could not find snippet $name");
    return;
  }

//  if name not in snippets:
//    errors.append("Undefined snippet {}".format(name))
//    contents += "**ERROR: Missing snippet {}**\n".format(name)
//    return contents
//
//  if snippets[name] == False:
//    errors.append("Reused snippet {}".format(name))
//    contents += "**ERROR: Reused snippet {}**\n".format(name)
//    return contents

//  # Consume it.
//  snippets[name] = False

//  location = []
//  if showLocation:
//    location = snippet.describe_location()

//  # Make sure every snippet shows the reader where it goes.
//  if (showLocation and len(location) <= 1
//      and beforeLines == 0 and afterLines == 0):
//    print("No location or context for {}".format(name))
//    errors.append("No location or context for {}".format(name))
//    contents += "**ERROR: No location or context for {}**\n".format(name)
//    return contents
//
//  # TODO: Show indentation in snippets somehow.
//
//  # Figure out the length of the longest line. We pad all of the snippets to
//  # this length so that the background on the pre sections is as wide as the
//  # entire chunk of code.
//  length = 0
//  if beforeLines > 0:
//    length = longest_line(length, snippet.context_before[-beforeLines:])
//  if snippet.removed and not snippet.added:
//    length = longest_line(length, snippet.removed)
//  if snippet.added_comma:
//    length = longest_line(length, snippet.added_comma)
//  if snippet.added:
//    length = longest_line(length, snippet.added)
//  if afterLines > 0:
//    length = longest_line(length, snippet.context_after[:afterLines])
  // TODO: Temp.
  var length = 64;

  buffer.write('<div class="codehilite">');

//  if beforeLines > 0:
//    before = format_code(snippet.file.language(), length,
//        snippet.context_before[-beforeLines:])
//    if snippet.added:
//      before = before.replace('<pre>', '<pre class="insert-before">')
//    contents += before
//
//  if snippet.added_comma:
//    def replace_last(string, old, new):
//      return new.join(string.rsplit(old, 1))
//
//    comma = format_code(snippet.file.language(), length, [snippet.added_comma])
//    comma = comma.replace('<pre>', '<pre class="insert-before">')
//    comma = replace_last(comma, ',', '<span class="insert-comma">,</span>')
//    contents += comma
//
//  if showLocation:
//    contents += '<div class="source-file">{}</div>\n'.format(
//        '<br>\n'.join(location))
//
//  if snippet.removed and not snippet.added:
//    removed = format_code(snippet.file.language(), length, snippet.removed)
//    removed = removed.replace('<pre>', '<pre class="delete">')
//    contents += removed
//
  if (snippet.added != null) {
    var added = formatCode(snippet.file.language, length, snippet.added);
//    if beforeLines > 0 or afterLines > 0:
//      added = added.replace('<pre>', '<pre class="insert">')
    buffer.writeln(added);
  }
//
//  if afterLines > 0:
//    after = format_code(snippet.file.language(), length,
//        snippet.context_after[:afterLines])
//    if snippet.added:
//      after = after.replace('<pre>', '<pre class="insert-after">')
//    contents += after
//
  buffer.writeln('</div>');
//
//  if showLocation:
//    contents += '<div class="source-file-narrow">{}</div>\n'.format(
//        ', '.join(location))
//
  buffer.writeln("snippet $name, $showLocation, $beforeLines, $afterLines");
}

String formatCode(String language, int length, List<String> lines) {
  // TODO: Pass in StringBuffer.
  var markup = "```$language\n";

//  # Hack. Markdown seems to discard leading and trailing newlines, so we'll
//  # add them back ourselves.
//  leading_newlines = 0
//  while lines and lines[0].strip() == '':
//    lines = lines[1:]
//    leading_newlines += 1
//
//  trailing_newlines = 0
//  while lines and lines[-1].strip() == '':
//    lines = lines[:-1]
//    trailing_newlines += 1
//
  for (var line in lines) {
    markup += line.padRight(length, " ") + "\n";
  }

  markup += "```";

  var html = renderMarkdown(markup);

//  if leading_newlines > 0:
//    html = html.replace('<pre>', '<pre>' + ('<br>' * leading_newlines))
//
//  if trailing_newlines > 0:
//    html = html.replace('</pre>', ('<br>' * trailing_newlines) + '</pre>')
//
//  # Strip off the div wrapper. We just want the <pre>.
//  html = html.replace('<div class="codehilite">', '')
//  html = html.replace('</div>', '')
  return html;
}

/// Process each SASS file.
void buildSass({bool skipUpToDate = false}) {
  var modules =
      Glob("asset/sass/*.scss").listSync().map((file) => file.path).toList();

  for (var source in Glob("asset/*.scss").listSync()) {
    var sourcePath = p.normalize(source.path);
    var dest = p.join("site", p.basenameWithoutExtension(source.path) + ".css");

    if (skipUpToDate && !isOutOfDate([...modules, sourcePath], dest)) continue;

    var output =
        sass.compile(sourcePath, color: true, style: sass.OutputStyle.expanded);
    File(dest).writeAsStringSync(output);
    print("$sourcePath -> $dest");
  }
}

/// Returns whether any of the [inputs] have been modified more recently than
/// [output.
bool isOutOfDate(List<String> inputs, String output) {
  var outputModified = File(output).lastModifiedSync();
  for (var input in inputs) {
    var inputModified = File(input).lastModifiedSync();
    if (inputModified.isAfter(outputModified)) return true;
  }

  return false;
}
