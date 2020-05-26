import 'dart:io';

import 'package:glob/glob.dart';
import 'package:path/path.dart' as p;
import 'package:sass/sass.dart' as sass;

import 'package:tool/src/book.dart';
import 'package:tool/src/markdown/markdown.dart';
import 'package:tool/src/mustache.dart';
import 'package:tool/src/page.dart';
import 'package:tool/src/snippet.dart';
import 'package:tool/src/term.dart' as term;
import 'package:tool/src/text.dart';

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

  var wordCount = 0;
  for (var line in page.lines) wordCount += countWords(line);

  // TODO: Move this into Page.
  Map<String, Snippet> snippets;
  if (page.isChapter) {
    snippets = book.code.findAll(page);
  }

  var body = renderMarkdown(page, snippets, page.lines);
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
