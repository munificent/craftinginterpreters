import 'dart:io';

import 'package:glob/glob.dart';
import 'package:path/path.dart' as p;
import 'package:sass/sass.dart' as sass;

import 'package:tool/src/book.dart';
import 'package:tool/src/code_tag.dart';
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

final _asideCommentPattern =
    RegExp(r' ?<span class="c1">// \[([-a-z0-9]+)\] *</span>');
final _asideWithCommentPattern =
    RegExp(r' ?<span class="c1">// (.+) \[([-a-z0-9]+)\] *</span>');

void main(List<String> arguments) {
  var watch = Stopwatch()..start();
  var totalWords = formatFiles();
//  for (var entry in Directory("book").listSync()) {
//    if (entry.path.endsWith(".md")) {
//      print(entry.path);
//    }
//  }

  buildSass();

  var seconds = (watch.elapsedMilliseconds / 1000).toStringAsFixed(2);
  print("Built $totalWords words in ${seconds}s");

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
int formatFiles({bool skipUpToDate = false}) {
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

  return totalWords;
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

  var buffer = StringBuffer();
  for (var i = 0; i < page.lines.length; i++) {
    var line = page.lines[i];

    if (page.codeTags.containsKey(i)) {
      // Leave a unique HTML comment where the snippet should go. We will
      // replace this with the real snippet after the Markdown is rendered.
      // This way, the snippet HTML is not run through the Markdown engine,
      // which is pointlessly slow and tends to unnecessarily muck with the
      // HTML.
      buffer.writeln("<!-- snippet $i -->");
    } else if (line.startsWith("^")) {
      // Skip non-code tag lines. We've already parsed them.
    } else if (line.startsWith("# ") ||
        line.startsWith("## ") ||
        line.startsWith("### ")) {
      var header = page.headers[line];

      buffer.write("#" * header.level + " ");

      buffer.write('<a href="#${header.anchor}" name="${header.anchor}">');

      if (!header.isSpecial) {
        buffer.write(
            "<small>${page.numberString}&#8202;.&#8202;${header.headerIndex}");
        if (header.subheaderIndex != null) {
          buffer.write("&#8202;.&#8202;${header.subheaderIndex}");
        }
        buffer.write("</small> ");
      }

      buffer.writeln("${header.name}</a>\n");
    } else {
      buffer.writeln(pretty(line));
    }
  }

  // TODO: Do this in a cleaner way.
  // Fix up em dashes. We do this on the entire contents instead of in pretty()
  // so that we can handle surrounding whitespace even when the "--" is at the
  // beginning of end of a line in Markdown.
  var contents = buffer
      .toString()
      .replaceAll(_emDashPattern, '<span class="em">&mdash;</span>');

  var wordCount = countWords(contents);

  var body = renderMarkdown(contents);

  // Put the snippets in.
  if (page.isChapter) {
    var snippets = book.code.findAll(page);

    page.codeTags.forEach((i, tag) {
      var snippet = snippets[tag.name];
      var snippetHtml = buildSnippet(tag, snippet);
      body = body.replaceAll("<!-- snippet $i -->", snippetHtml);

      wordCount += countWords(snippetHtml);
    });
  }

  var output = mustache.render(book, page, body);

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
  output = output.replaceAll("><aside", ">\n<aside");
  output = output.replaceAll("</aside><", "</aside>\n<");
  output = output.replaceAll("</ol><", "</ol>\n<");
  output = output.replaceAll("</table>\n<", "</table>\n\n<");

  // Python Markdown wraps images in paragraphs.
  output = output.replaceAllMapped(
      RegExp(r'\n(<img [^>]*>)\n'), (match) => '\n<p>${match[1]}</p>\n');
  output = output.replaceAllMapped(
      RegExp(r'\n(<img [^>]*>)</'), (match) => '\n<p>${match[1]}</p>\n</');

  // Python Markdown puts some extra blank lines after the pre tags.
  output = output.replaceAll('</pre></div>\n<', '</pre></div>\n\n\n<');

  // Write the output.
  File(page.htmlPath).writeAsStringSync(output);

  var words = term.gray("($wordCount words)");
  var number = "";
  if (page.numberString.isNotEmpty) {
    number = "${page.numberString}. ";
  }

  if (const ["index", "contents"].contains(page.fileName)) {
    print("${term.green('•')} $number${page.title}");
  } else if (page.isChapter) {
    print("  ${term.green('✓')} $number${page.title} $words");
  } else {
    print("${term.green('✓')} $number${page.title} $words");
  }

  return wordCount;
}

String buildSnippet(CodeTag tag, Snippet snippet) {
  // NOTE: If you change this, be sure to update the baked in example snippet
  // in introduction.md.
  List<String> linesBefore;
  if (tag.beforeCount > 0) {
    linesBefore = snippet.contextBefore
        .sublist(snippet.contextBefore.length - tag.beforeCount);
  }

  List<String> linesAfter;
  if (tag.afterCount > 0) {
    linesAfter = snippet.contextAfter.take(tag.afterCount).toList();
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
  if (tag.showLocation) location = snippet.locationDescription;

//  # Make sure every snippet shows the reader where it goes.
//  if (showLocation and len(location) <= 1
//      and beforeLines == 0 and afterLines == 0):
//    print("No location or context for {}".format(name))
//    errors.append("No location or context for {}".format(name))
//    contents += "**ERROR: No location or context for {}**\n".format(name)
//    return contents
//
//  # TODO: Show indentation in snippets somehow.

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

  if (tag.showLocation) {
    var lines = location.join("<br>\n");
    buffer.writeln('<div class="source-file">$lines</div>');
  }

//  if snippet.removed and not snippet.added:
//    removed = format_code(snippet.file.language(), length, snippet.removed)
//    removed = removed.replace('<pre>', '<pre class="delete">')
//    contents += removed

  if (snippet.added != null) {
    var added = formatCode(snippet.file.language, length, snippet.added,
        tag.beforeCount > 0 || tag.afterCount > 0 ? "insert" : null);
    buffer.write(added);
  }

  if (linesAfter != null) {
    var after = formatCode(snippet.file.language, length, linesAfter,
        snippet.added.isNotEmpty ? "insert-after" : null);
    buffer.write(after);
  }

  buffer.writeln('</div>');

  if (tag.showLocation) {
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
