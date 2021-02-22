import 'dart:io';

import 'package:glob/glob.dart';
import 'package:mime_type/mime_type.dart';
import 'package:path/path.dart' as p;
import 'package:sass/sass.dart' as sass;
import 'package:shelf/shelf.dart' as shelf;
import 'package:shelf/shelf_io.dart' as io;

import 'package:tool/src/book.dart';
import 'package:tool/src/markdown/markdown.dart';
import 'package:tool/src/mustache.dart';
import 'package:tool/src/page.dart';
import 'package:tool/src/term.dart' as term;
import 'package:tool/src/text.dart';

/// Aside comment marker in highlighted code.
final _asideHighlightedCommentPattern =
    RegExp(r' ?<span class="c">// \[([-a-z0-9]+)\] *</span>');

/// Aside comment marker in highlighted code with a comment too.
final _asideHighlightedWithCommentPattern =
    RegExp(r' ?<span class="c">// (.+) \[([-a-z0-9]+)\] *</span>');

/// Aside comment marker in context lines which are not syntax highlighted.
final _asideCommentPattern = RegExp(r' +// \[([-a-z0-9]+)\]');

/// Aside comment marker in context lines which are not syntax highlighted with
/// a comment too.
final _asideWithCommentPattern = RegExp(r' +// (.+) \[([-a-z0-9]+)\]');

Future<void> main(List<String> arguments) async {
  _buildSass();
  _buildPages();

  if (arguments.contains("--serve")) {
    await _runServer();
  }
}

/// Process each Markdown file.
void _buildPages({bool skipUpToDate = false}) {
  var watch = Stopwatch()..start();
  var book = Book();
  var mustache = Mustache();

  DateTime dependenciesModified;
  if (skipUpToDate) {
    dependenciesModified = _mostRecentlyModified(
        ["asset/mustache/*.html", "c/*.{c,h}", "java/**.java"]);
  }

  var proseWords = 0;
  var codeLines = 0;
  var totalWords = 0;
  for (var page in book.pages) {
    var metrics = _buildPage(book, mustache, page,
        dependenciesModified: dependenciesModified);
    proseWords += metrics[0];
    codeLines += metrics[1];
    totalWords += metrics[2];
  }

  if (totalWords > 0) {
    var seconds = (watch.elapsedMilliseconds / 1000).toStringAsFixed(2);
    print("Built ${term.green(proseWords.withCommas)} words and "
        "${term.cyan(codeLines.withCommas)} lines of code "
        "(${totalWords.withCommas} total words) in $seconds seconds");
  }
}

List<int> _buildPage(Book book, Mustache mustache, Page page,
    {DateTime dependenciesModified}) {
  // See if the HTML is up to date.
  if (dependenciesModified != null &&
      _isUpToDate(page.htmlPath, page.markdownPath, dependenciesModified)) {
    return [0, 0, 0];
  }

  var proseCount = 0;
  var codeLineCount = 0;
  for (var line in page.lines) proseCount += line.wordCount;

  var wordCount = proseCount;
  for (var tag in page.codeTags) {
    var snippet = book.findSnippet(tag);
    if (snippet == null) {
      print("No snippet for $tag");
      continue;
    }

    codeLineCount += snippet.added.length;
    for (var line in snippet.added) wordCount += line.wordCount;
    for (var line in snippet.contextBefore) wordCount += line.wordCount;
    for (var line in snippet.contextAfter) wordCount += line.wordCount;
  }

  var body = renderMarkdown(book, page, page.lines);
  var output = mustache.render(book, page, body);

  // Turn aside markers in code into spans. In the empty span case, insert a
  // zero-width space because Chrome seems to lose the span's position if it has
  // no content.
  // <span class="c">// [repl]</span>
  // TODO: Do this directly in the syntax highlighter instead of after the fact.
  output = output.replaceAllMapped(_asideHighlightedCommentPattern,
      (match) => '<span name="${match[1]}"> </span>');
  output = output.replaceAllMapped(_asideHighlightedWithCommentPattern,
      (match) => '<span class="c" name="${match[2]}">// ${match[1]}</span>');
  output = output.replaceAllMapped(
      _asideCommentPattern, (match) => '<span name="${match[1]}"> </span>');
  output = output.replaceAllMapped(_asideWithCommentPattern,
      (match) => '<span name="${match[2]}">// ${match[1]}</span>');

  // Write the output.
  File(page.htmlPath).writeAsStringSync(output);

  var words = "$wordCount words";
  if (codeLineCount > 0) words += ", $codeLineCount loc";
  words = term.gray("($words)");

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

  return [proseCount, codeLineCount, wordCount];
}

/// Process each SASS file.
void _buildSass({bool skipUpToDate = false}) {
  var moduleModified = _mostRecentlyModified(["asset/sass/*.scss"]);

  for (var source in Glob("asset/*.scss").listSync()) {
    var scssPath = p.normalize(source.path);
    var cssPath =
        p.join("site", p.basenameWithoutExtension(source.path) + ".css");

    if (skipUpToDate && _isUpToDate(cssPath, scssPath, moduleModified)) {
      continue;
    }

    var output =
        sass.compile(scssPath, color: true, style: sass.OutputStyle.expanded);
    File(cssPath).writeAsStringSync(output);
    print("${term.green('-')} $cssPath");
  }
}

Future<void> _runServer() async {
  Future<shelf.Response> handleRequest(shelf.Request request) async {
    var filePath = p.normalize(p.fromUri(request.url));
    if (filePath == ".") filePath = "index.html";
    var extension = p.extension(filePath).replaceAll(".", "");

    // Refresh files that are being requested.
    if (extension == "html") {
      _buildPages(skipUpToDate: true);
    } else if (extension == "css") {
      _buildSass(skipUpToDate: true);
    }

    try {
      var contents = await File(p.join("site", filePath)).readAsBytes();
      return shelf.Response.ok(contents, headers: {
        HttpHeaders.contentTypeHeader: mimeFromExtension(extension)
      });
    } on FileSystemException {
      print(
          "${term.red(request.method)} Not found: ${request.url} ($filePath)");
      return shelf.Response.notFound("Could not find '$filePath'.");
    }
  }

  var handler = const shelf.Pipeline().addHandler(handleRequest);

  var server = await io.serve(handler, "localhost", 8000);
  print("Serving at http://${server.address.host}:${server.port}");
}

/// Returns `true` if [outputPath] was generated after [inputPath] and more
/// recently than [dependenciesModified].
bool _isUpToDate(
    String outputPath, String inputPath, DateTime dependenciesModified) {
  var outputModified = File(outputPath).lastModifiedSync();
  var inputModified = File(inputPath).lastModifiedSync();
  return outputModified.isAfter(dependenciesModified) &&
      outputModified.isAfter(inputModified);
}

/// The most recently modified time of all files that match [globs].
DateTime _mostRecentlyModified(List<String> globs) {
  DateTime latest;
  for (var glob in globs) {
    for (var entry in Glob(glob).listSync()) {
      if (entry is File) {
        var modified = entry.lastModifiedSync();
        if (latest == null || modified.isAfter(latest)) latest = modified;
      }
    }
  }

  return latest;
}
