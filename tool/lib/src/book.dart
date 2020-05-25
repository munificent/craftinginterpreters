import 'package:tool/src/source_code.dart';

import 'page.dart';
import 'text.dart';

// TODO: Simplify.
const tableOfContents = [
  {
    'name': '',
    'chapters': [
      {'name': 'Crafting Interpreters'},
      {'name': 'Table of Contents'}
    ],
  },
  {
    'name': 'Welcome',
    'chapters': [
      {'name': 'Introduction'},
      {'name': 'A Map of the Territory'},
      {'name': 'The Lox Language'}
    ]
  },
  {
    'name': 'A Tree-Walk Interpreter',
    'chapters': [
      {'name': 'Scanning'},
      {'name': 'Representing Code'},
      {'name': 'Parsing Expressions'},
      {'name': 'Evaluating Expressions'},
      {'name': 'Statements and State'},
      {'name': 'Control Flow'},
      {'name': 'Functions'},
      {'name': 'Resolving and Binding'},
      {'name': 'Classes'},
      {'name': 'Inheritance'}
    ]
  },
  {
    'name': 'A Bytecode Virtual Machine',
    'chapters': [
      {'name': 'Chunks of Bytecode'},
      {'name': 'A Virtual Machine'},
      {'name': 'Scanning on Demand'},
      {'name': 'Compiling Expressions'},
      {'name': 'Types of Values'},
      {'name': 'Strings'},
      {'name': 'Hash Tables'},
      {'name': 'Global Variables'},
      {'name': 'Local Variables'},
      {'name': 'Jumping Back and Forth'},
      {'name': 'Calls and Functions'},
      {'name': 'Closures'},
      {'name': 'Garbage Collection'},
      {'name': 'Classes and Instances'},
      {'name': 'Methods and Initializers'},
      {'name': 'Superclasses'},
      {'name': 'Optimization'}
    ]
  },
  {
    'name': 'Backmatter',
    'chapters': [
      {'name': 'Appendix I'},
      {'name': 'Appendix II'}
    ],
  },
];

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

    for (var part in tableOfContents) {
      // Front- and backmatter have no names, pages, or numbers.
      var partNumber = "";
      var partName = part["name"] as String;
      inMatter = partName == "" || partName == "Backmatter";
      if (!inMatter) {
        partNumber = roman(partIndex);
        partIndex += 1;
      }

      Page partPage;

      // There are no part pages for the front- and backmatter.
      if (part["name"] != "") {
        partPage = Page(partName, null, partNumber, pages.length);
        pages.add(partPage);
        parts.add(partPage);
      }

      for (var chapter in part["chapters"]) {
        var name = chapter["name"] as String;

        var chapterNumber = "";
        if (inMatter) {
          // Front- and backmatter chapters are specially numbered.
          if (name == "Appendix I") {
            chapterNumber = "A1";
          } else if (name == "Appendix II") {
            chapterNumber = "A2";
          }
        } else {
          chapterNumber = chapterIndex.toString();
          chapterIndex++;
        }

        var chapterPage = Page(name, partPage, chapterNumber, pages.length);
        pages.add(chapterPage);
        if (partPage != null) partPage.chapters.add(chapterPage);
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
