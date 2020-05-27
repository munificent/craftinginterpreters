import 'code_tag.dart';
import 'location.dart';
import 'page.dart';
import 'snippet.dart';
import 'source_file_parser.dart';
import 'text.dart';

import 'package:glob/glob.dart';
import 'package:path/path.dart' as p;

const _tableOfContents = {
  '': [
    'Crafting Interpreters',
    'Table of Contents',
  ],
  'Welcome': [
    'Introduction',
    'A Map of the Territory',
    'The Lox Language',
  ],
  'A Tree-Walk Interpreter': [
    'Scanning',
    'Representing Code',
    'Parsing Expressions',
    'Evaluating Expressions',
    'Statements and State',
    'Control Flow',
    'Functions',
    'Resolving and Binding',
    'Classes',
    'Inheritance',
  ],
  'A Bytecode Virtual Machine': [
    'Chunks of Bytecode',
    'A Virtual Machine',
    'Scanning on Demand',
    'Compiling Expressions',
    'Types of Values',
    'Strings',
    'Hash Tables',
    'Global Variables',
    'Local Variables',
    'Jumping Back and Forth',
    'Calls and Functions',
    'Closures',
    'Garbage Collection',
    'Classes and Instances',
    'Methods and Initializers',
    'Superclasses',
    'Optimization',
  ],
  'Backmatter': [
    'Appendix I',
    'Appendix II',
  ],
};

/// The contents of the Markdown and source files for the book, loaded and
/// parsed.
class Book {
  final List<Page> parts = [];
  final List<Page> pages = [];

  final Map<CodeTag, Snippet> _snippets = {};

  Book() {
    var partIndex = 1;
    var chapterIndex = 1;
    var inMatter = false;

    // Load the pages.
    for (var part in _tableOfContents.keys) {
      // Front- and backmatter have no names, pages, or numbers.
      var partNumber = "";
      inMatter = part == "" || part == "Backmatter";
      if (!inMatter) {
        partNumber = roman(partIndex);
        partIndex += 1;
      }

      // There are no part pages for the front- and backmatter.
      Page partPage;
      if (part != "") {
        partPage = Page(part, null, partNumber, pages.length);
        pages.add(partPage);
        parts.add(partPage);
      }

      for (var chapter in _tableOfContents[part]) {
        var chapterNumber = "";
        if (inMatter) {
          // Front- and backmatter chapters are specially numbered.
          if (chapter == "Appendix I") {
            chapterNumber = "A1";
          } else if (chapter == "Appendix II") {
            chapterNumber = "A2";
          }
        } else {
          chapterNumber = chapterIndex.toString();
          chapterIndex++;
        }

        var page = Page(chapter, partPage, chapterNumber, pages.length);
        pages.add(page);
        if (partPage != null) partPage.chapters.add(page);
      }
    }

    // Load the source files.
    for (var language in ["java", "c"]) {
      for (var file in Glob("$language/**.{c,h,java}").listSync()) {
        var shortPath = p.relative(file.path, from: language);
        var sourceFile = SourceFileParser(this, file.path, shortPath).parse();

        // Create snippets from the lines in the file.
        var lineIndex = 0;
        for (var line in sourceFile.lines) {
          var snippet = _snippets.putIfAbsent(
              line.start, () => Snippet(sourceFile, line.start));
          snippet.addLine(lineIndex, line);

          if (line.end != null) {
            var endSnippet = _snippets.putIfAbsent(
                line.end, () => Snippet(sourceFile, line.end));
            endSnippet.removeLine(lineIndex, line);
          }

          lineIndex++;
        }
      }
    }

    for (var snippet in _snippets.values) {
      if (snippet.tag.name == "not-yet") continue;
      if (snippet.tag.name == "omit") continue;
      snippet.calculateContext();
    }
  }

  /// Looks for a page with [title].
  Page findChapter(String title) =>
      pages.firstWhere((page) => page.title == title);

  /// Looks for a page with [number];
  Page findNumber(String number) =>
      pages.firstWhere((page) => page.numberString == number);

  /// Gets the [Page] [offset] pages before or after this one.
  Page adjacentPage(Page start, int offset) {
    var index = pages.indexOf(start) + offset;
    if (index < 0 || index >= pages.length) return null;
    return pages[index];
  }

  Snippet findSnippet(CodeTag tag) => _snippets[tag];
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
  final CodeTag start;

  /// The last snippet where this line is removed, or null if the line reaches
  /// the end of the book.
  final CodeTag end;

  SourceLine(this.text, this.location, this.start, this.end);

  /// Returns true if this line exists by the time we reach [tag].
  bool isPresent(CodeTag tag) {
    // If we haven't reached this line's snippet yet.
    if (tag < start) return false;

    // If we are past the snippet where it is removed.
    if (end != null && tag >= end) return false;

    return true;
  }

  String toString() {
    var result = "${text.padRight(72)} // $start";
    if (end != null) result += " < $end";
    return result;
  }
}
