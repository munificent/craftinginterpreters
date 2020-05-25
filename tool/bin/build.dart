import 'dart:io';

import 'package:glob/glob.dart';
import 'package:path/path.dart' as p;
import 'package:sass/sass.dart' as sass;

import 'package:tool/src/book.dart';
import 'package:tool/src/markdown.dart';
import 'package:tool/src/mustache.dart';
import 'package:tool/src/page.dart';
import 'package:tool/src/snippet.dart';
import 'package:tool/src/syntax/highlighter.dart';
import 'package:tool/src/term.dart' as term;
import 'package:tool/src/text.dart';

// The "(?!-)" is a hack. scanning.md has an inline code sample containing a
// "--" operator. We don't want that to get matched, so fail the match if the
// character after the "-- " is "-", which is the next character in the code
// sample.
final _emDashPattern = RegExp(r"\s+--\s(?!-)");

final _codeOptionsPattern = RegExp(r"([-a-z0-9]+) \(([^)]+)\)");
final _beforePattern = RegExp(r"(\d+) before");
final _afterPattern = RegExp(r"(\d+) after");

final _asideCommentPattern =
    RegExp(r' ?<span class="c1">// \[([-a-z0-9]+)\] *</span>');
final _asideWithCommentPattern =
    RegExp(r' ?<span class="c1">// (.+) \[([-a-z0-9]+)\] *</span>');

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

  var book = Book();
  var mustache = Mustache();

  // TODO: Temp. Just one chapter for now.
//  formatFile(book, mustache, book.findChapter("Classes"));

  var totalWords = 0;
  for (var page in book.pages) {
    totalWords += formatFile(book, mustache, page);
  }
  print("$totalWords words");
}

// TODO: Move to library.
// TODO: Skip up to date stuff.
int formatFile(Book book, Mustache mustache, Page page) {
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
  var template = 'page';

//  errors = []
  // TODO: Something better typed.
  var sections = <Map<String, String>>[];
  var headerIndex = 0;
  var subheaderIndex = 0;
  var hasChallenges = false;
  String designNote;

  // TODO: Move this into Page and load lazily.
  Map<String, Snippet> snippets;

  var insertedSnippets = <String>[];

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
          // Leave a unique HTML comment where the snippet should go. We will
          // replace this with the real snippet after the Markdown is rendered.
          // This way, the snippet HTML is not run through the Markdown engine,
          // which is pointlessly slow and tends to unnecessarily muck with the
          // HTML.
          var snippetCode = createSnippet(book, page, snippets, argument);
          buffer.writeln("<!-- snippet ${insertedSnippets.length} -->");
          insertedSnippets.add(snippetCode);
          break;
        case "part":
          // TODO: No longer used. Remove from Markdown files.
          break;
        case "template":
          template = argument;
          break;
        case "title":
          title = argument;

          // Remove any discretionary hyphens from the title.
          // TODO: Still needed?
          title = title.replaceAll("&shy;", "");

          // Load the code snippets now that we know the title.
          if (page is ChapterPage) {
            snippets = book.code.findAll(page);
          }

//          # If there were any errors loading the code, include them.
//          if title in book.CODE_CHAPTERS:
//            errors.extend(source_code.errors[title])
          break;
        default:
          throw "Unknown command '$command'.";
      }
    } else if (stripped.startsWith("## Challenges")) {
      hasChallenges = true;
      buffer.writeln('## <a href="#challenges" name="challenges">'
          'Challenges</a>');
    } else if (stripped.startsWith("## Design Note:")) {
      designNote = stripped.substring('## Design Note:'.length + 1);
      buffer.writeln('## <a href="#design-note" name="design-note">'
          'Design Note: $designNote</a>');
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

      buffer.writeln('<a href="#$anchor" name="$anchor">'
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

  var body = renderMarkdown(contents);

  // Put the snippets in.
  for (var i = 0; i < insertedSnippets.length; i++) {
    body = body.replaceAll("<!-- snippet $i -->", insertedSnippets[i]);
  }

  var output =
      mustache.render(template, book, page, body, sections, hasChallenges);

  // Turn aside markers in code into spans. In the empty span case, insert a
  // zero-width space because Chrome seems to lose the span's position if it has
  // no content.
  // <span class="c1">// [repl]</span>
  // TODO: Do this directly in the syntax highlighter instead of after the fact.
  output = output.replaceAllMapped(
      _asideCommentPattern, (match) => '<span name="${match[1]}"> </span>');
  output = output.replaceAllMapped(_asideWithCommentPattern,
      (match) => '<span class="c1" name="${match[2]}">// ${match[1]}</span>');

  // TODO: Temp hack. Insert some whitespace to match the old Markdown.
  output = output.replaceAll("</p><", "</p>\n<");
//  output = output.replaceAll("</div><", "</div>\n<");
  output = output.replaceAll("><aside", ">\n<aside");
  output = output.replaceAll("</aside><", "</aside>\n<");
  output = output.replaceAll("</ol><", "</ol>\n<");
  output = output.replaceAll("</table>\n<", "</table>\n\n<");
//  output = output.replaceAllMapped(
//      RegExp(r'<div class="source-file-narrow">(.*?)</div>'),
//      (match) => '\n<div class="source-file-narrow">${match[1]}</div>\n');

  // Python Markdown wraps images in paragraphs.
  output = output.replaceAllMapped(
      RegExp(r'\n(<img [^>]*>)\n'), (match) => '\n<p>${match[1]}</p>\n');
  output = output.replaceAllMapped(
      RegExp(r'\n(<img [^>]*>)</'), (match) => '\n<p>${match[1]}</p>\n</');

  // Dart Markdown library puts a newline before <pre>.
//  output = output.replaceAll(
//      '<div class="codehilite">\n<pre>', '<div class="codehilite"><pre>');

  // Python Markdown puts some extra blank lines after the pre tags.
  output = output.replaceAll('</pre></div>\n<', '</pre></div>\n\n\n<');

  // Python Markdown puts a blank line before the closing pre tag.
//  output = output.replaceAll('</span></pre></div>', '</span>\n</pre></div>');

  // Write the output.
  File(page.htmlPath).writeAsStringSync(output);

  // TODO: Do this faster.
  // TODO: Note: This no longer includes the word count of the code snippets.
  // Fix that if I want an accurate count.
  var wordCount = contents.split(RegExp(r"\s+")).length;
  var words = term.gray("($wordCount words)");
  var number = "";
  if (page.numberString.isNotEmpty) {
    number = "${page.numberString}. ";
  }

  if (const ["index", "contents"].contains(page.fileName)) {
    print("${term.green('•')} $number${page.title}");
  } else if (page is ChapterPage) {
    print("  ${term.green('✓')} $number${page.title} $words");
  } else {
    print("${term.green('✓')} $number${page.title} $words");
  }

  return wordCount;
}

String createSnippet(
    Book book, Page page, Map<String, Snippet> snippets, String name) {
  // NOTE: If you change this, be sure to update the baked in example snippet
  // in introduction.md.
//def insert_snippet(snippets, arg, contents, errors):

  // Parse the location annotations after the name, if present.
  var showLocation = true;
  var beforeCount = 0;
  var afterCount = 0;

  var match = _codeOptionsPattern.firstMatch(name);
  if (match != null) {
    name = match.group(1);
    var options = match.group(2).split(", ");

    for (var option in options) {
      if (option == "no location") {
        showLocation = false;
      } else if ((match = _beforePattern.firstMatch(option)) != null) {
        beforeCount = int.parse(match.group(1));
      } else if ((match = _afterPattern.firstMatch(option)) != null) {
        afterCount = int.parse(match.group(1));
      }
    }
  }

  var snippet = snippets[name];

  List<String> linesBefore;
  if (beforeCount > 0) {
    linesBefore = snippet.contextBefore
        .sublist(snippet.contextBefore.length - beforeCount);
  }

  List<String> linesAfter;
  if (afterCount > 0) {
    linesAfter = snippet.contextAfter.take(afterCount).toList();
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

  var location = <String>[];
  if (showLocation) location = snippet.locationDescription;

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
  // Figure out the length of the longest line. We pad all of the snippets to
  // this length so that the background on the pre sections is as wide as the
  // entire chunk of code.
  var length = 0;
  if (linesBefore != null) {
    length = longestLine(length, linesBefore);
  }
  if (snippet.removed.isNotEmpty && snippet.added.isEmpty) {
    length = longestLine(length, snippet.removed);
  }
  if (snippet.addedComma != null) {
    length = longestLine(length, [snippet.addedComma]);
  }
  if (snippet.added.isNotEmpty) {
    length = longestLine(length, snippet.added);
  }
  if (linesAfter != null) {
    length = longestLine(length, linesAfter);
  }

  var buffer = StringBuffer();
  buffer.write('<div class="codehilite">');

  if (linesBefore != null) {
    var before = formatCode(snippet.file.language, length, linesBefore,
        snippet.added.isNotEmpty ? "insert-before" : null);
    buffer.write(before);
  }

  if (snippet.addedComma != null) {
    var commaLine = formatCode(
        snippet.file.language, length, [snippet.addedComma], "insert-before");
    var comma = commaLine.lastIndexOf(",");
    buffer.write(commaLine.substring(0, comma));
    buffer.write('<span class="insert-comma">,</span>');
    buffer.write(commaLine.substring(comma + 1));
  }

  if (showLocation) {
    var lines = location.join("<br>\n");
    buffer.writeln('<div class="source-file">$lines</div>');
  }

//  if snippet.removed and not snippet.added:
//    removed = format_code(snippet.file.language(), length, snippet.removed)
//    removed = removed.replace('<pre>', '<pre class="delete">')
//    contents += removed

  if (snippet.added != null) {
    var added = formatCode(snippet.file.language, length, snippet.added,
        beforeCount > 0 || afterCount > 0 ? "insert" : null);
    buffer.write(added);
  }

  if (linesAfter != null) {
    var after = formatCode(snippet.file.language, length, linesAfter,
        snippet.added.isNotEmpty ? "insert-after" : null);
    buffer.write(after);
  }

  buffer.writeln('</div>');

  if (showLocation) {
    // TODO: Just to match the old output. Delete when not needed.
    buffer.writeln();

    var lines = location.join(", ");
    buffer.writeln('<div class="source-file-narrow">$lines</div>');
  }

  return buffer.toString();
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
    print("${term.green('-')} $dest");
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
