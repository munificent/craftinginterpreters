import 'package:tool/src/source_code.dart';

import 'page.dart';
import 'text.dart';

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

/// TODO: This is basically a global, eagerly-loaded God object. If we want to
/// handle incremental refresh better, this should probably be less monolithic.
class Book {
  final List<Page> parts = [];
  final List<Page> pages = [];

  final SourceCode code = SourceCode();

  Book() {
    var partIndex = 1;
    var chapterIndex = 1;
    var inMatter = false;

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

    code.load(this);
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
}
