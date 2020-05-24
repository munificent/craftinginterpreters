import 'package:glob/glob.dart';
import 'package:path/path.dart' as p;

import 'book.dart';
import 'location.dart';
import 'page.dart';
import 'snippet.dart';
import 'snippet_tag.dart';
import 'source_file_parser.dart';

/// All of the source files in the book.
class SourceCode {
  final List<SourceFile> files = [];

  // TODO: Not needed?
//  final Map<Page, Map<String, SnippetTag>> snippetTags = getChapterSnippetTags();

  SourceCode() {
//    # The errors that occurred when parsing the source code, mapped to the
//    # chapter where the error should be displayed.
//    self.errors = {}
//    for chapter in book.CODE_CHAPTERS:
//      self.errors[chapter] = []
  }

  void load(Book book) {
    for (var language in ["java", "c"]) {
      for (var file in Glob("$language/**.{c,h,java}").listSync()) {
        var shortPath = p.relative(file.path, from: language);
        files.add(SourceFileParser(book, file.path, shortPath).parse());
      }
    }

//  return source_code
  }

  // TODO: Move into Page.
  /// Gets the list of snippets that occur in [chapter].
  Map<String, Snippet> findAll(ChapterPage chapter) {
    // TODO: Type.
    var snippets = <String, Snippet>{};

    var firstLines = <Snippet, int>{};
    var lastLines = <Snippet, int>{};

    // Create a new snippet for [name] if it doesn't already exist.
    Snippet ensureSnippet(SourceFile file, String name, [int lineNum]) {
      if (!snippets.containsKey(name)) {
        var snippet = Snippet(file, name);
        snippets[name] = snippet;
        if (lineNum != null) firstLines[snippet] = lineNum;
        return snippet;
      }

      var snippet = snippets[name];
      if (lineNum != null && !firstLines.containsKey(snippet)) {
        firstLines[snippet] = lineNum;
//      if name != 'not-yet' and name != 'omit' and snippet.file.path != file.path:
//        print('Error: "{} {}" appears in two files, {} and {}.'.format(
//                chapter, name, snippet.file.path, file.path),
//            file=sys.stderr)
      }

      return snippet;
    }

    // TODO: If this is slow, could organize directly by snippets in SourceCode.
    // Find the lines added and removed in each snippet.
    for (var file in files) {
      var lineNum = 0;
      for (var line in file.lines) {
        if (line.start.chapter == chapter) {
          var snippet = ensureSnippet(file, line.start.name, lineNum);
          // TODO: Move most of this logic into snippet.
          snippet.added.add(line.text);
          lastLines[snippet] = lineNum;

          if (snippet.added.length == 1) {
            snippet.location = line.location;
          }
        }

        if (line.end != null && line.end.chapter == chapter) {
          var snippet = ensureSnippet(file, line.end.name);
          snippet.removed.add(line.text);
        }

        lineNum++;
      }
    }

    // Find the surrounding context lines and location for each snippet.
    for (var name in snippets.keys) {
      var snippet = snippets[name];
      var currentSnippet = chapter.snippetTags[name];

      // Look for preceding lines.
      var i = firstLines[snippet] - 1;
      var before = <String>[];
      while (i >= 0 && before.length <= 5) {
        var line = snippet.file.lines[i];
        if (line.isPresent(currentSnippet)) {
          before.add(line.text);

          // Store the most precise preceding location we find.
          if (snippet.precedingLocation == null ||
              line.location.depth > snippet.precedingLocation.depth) {
            snippet.precedingLocation = line.location;
          }
        }

        i--;
      }
      snippet.contextBefore.addAll(before.reversed);

      // Look for following lines.
      i = lastLines[snippet] + 1;
      var after = <String>[];
      while (i < snippet.file.lines.length && after.length <= 5) {
        var line = snippet.file.lines[i];
        if (line.isPresent(currentSnippet)) after.add(line.text);
        i++;
      }
      snippet.contextAfter.addAll(after);
    }

    // Find line changes that just add a trailing comma.
    for (var name in snippets.keys) {
      var snippet = snippets[name];
      if (snippet.added.isNotEmpty &&
          snippet.removed.isNotEmpty &&
          snippet.added.first == snippet.removed.last + ",") {
        snippet.addedComma = snippet.added.first;
        snippet.added.removeAt(0);
        snippet.removed.removeLast();
      }
    }

    return snippets;
  }

//  def split_chapter(self, file, chapter, name):
//    """
//    Gets the code for [file] as it appears at snippet [name] of [chapter].
//    """
//    tag = self.snippet_tags[chapter][name]
//
//    source_file = None
//    for source in self.files:
//      if source.path == file:
//        source_file = source
//        break
//
//    if source_file == None:
//      raise Exception('Could not find file "{}"'.format(file))
//
//    output = ""
//    for line in source_file.lines:
//      if line.is_present(tag):
//        # Hack. In generate_ast.java, we split up a parameter list among
//        # multiple chapters, which leads to hanging commas in some cases.
//        # Remove them.
//        if line.text.strip().startswith(")") and output.endswith(",\n"):
//          output = output[:-2] + "\n"
//
//        output += line.text + "\n"
//    return output
}

/// A single source file whose code is included in the book.
class SourceFile {
  final String path;
  final List<SourceLine> lines = [];

  SourceFile(this.path);

  String get language => path.endsWith("java") ? "java" : "c";

  String get nicePath => path.replaceAll("com/craftinginterpreters/", "");
}

/// A line of code in a [SourceFile] and the metadata for it.
class SourceLine {
  final String text;
  final Location location;

  /// The first snippet where this line appears in the book.
  final SnippetTag start;

  /// The last snippet where this line is removed, or null if the line reaches
  /// the end of the book.
  final SnippetTag end;

  SourceLine(this.text, this.location, this.start, this.end);

  /// Returns true if this line exists by the time we reach [snippet].
  bool isPresent(SnippetTag snippet) {
    // If we haven't reached this line's snippet yet.
    if (snippet < start) return false;

    // If we are past the snippet where it is removed.
    if (end != null && snippet >= end) return false;

    return true;
  }

  String toString() {
    var result = "${text.padRight(72)} // $start";
    if (end != null) result += " < $end";
    return result;
  }
}
