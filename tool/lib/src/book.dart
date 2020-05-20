import 'package:tool/src/source_code.dart';

import 'page.dart';
import 'text.dart';

// TODO: Make private?
// TODO: Make less stringly typed.
const tableOfContents = [
  {
    'name': '',
    'chapters': [
      {
        'name': 'Crafting Interpreters',
      },
      {
        'name': 'Table of Contents',
      }
    ],
  },
  {
    'name': 'Welcome',
    'chapters': [
      {'name': 'Introduction', 'design_note': "What's in a Name?"},
      {
        'name': 'A Map of the Territory',
      },
      {'name': 'The Lox Language', 'design_note': "Expressions and Statements"}
    ]
  },
  {
    'name': 'A Tree-Walk Interpreter',
    'chapters': [
      {'name': 'Scanning', 'design_note': "Implicit Semicolons"},
      {
        'name': 'Representing Code',
      },
      {'name': 'Parsing Expressions', 'design_note': "Logic Versus History"},
      {
        'name': 'Evaluating Expressions',
        'design_note': 'Static and Dynamic Typing'
      },
      {
        'name': 'Statements and State',
        'design_note': 'Implicit Variable Declaration'
      },
      {'name': 'Control Flow', 'design_note': 'Spoonfuls of Syntactic Sugar'},
      {
        'name': 'Functions',
      },
      {
        'name': 'Resolving and Binding',
      },
      {'name': 'Classes', 'design_note': 'Prototypes and Power'},
      {
        'name': 'Inheritance',
      }
    ]
  },
  {
    'name': 'A Bytecode Virtual Machine',
    'chapters': [
      {'name': 'Chunks of Bytecode', 'design_note': 'Test Your Language'},
      {'name': 'A Virtual Machine', 'design_note': 'Register-Based Bytecode'},
      {
        'name': 'Scanning on Demand',
      },
      {'name': 'Compiling Expressions', 'design_note': "It's Just Parsing"},
      {
        'name': 'Types of Values',
      },
      {'name': 'Strings', 'design_note': 'String Encoding'},
      {
        'name': 'Hash Tables',
      },
      {
        'name': 'Global Variables',
      },
      {
        'name': 'Local Variables',
      },
      {
        'name': 'Jumping Back and Forth',
        'design_note': 'Considering Goto Harmful'
      },
      {
        'name': 'Calls and Functions',
      },
      {'name': 'Closures', 'design_note': 'Closing Over the Loop Variable'},
      {'name': 'Garbage Collection', 'design_note': 'Generational Collectors'},
      {
        'name': 'Classes and Instances',
      },
      {'name': 'Methods and Initializers', 'design_note': 'Novelty Budget'},
      {
        'name': 'Superclasses',
      },
      {
        'name': 'Optimization',
      }
    ]
  },
  {
    'name': 'Backmatter',
    'chapters': [
      {
        'name': 'Appendix I',
      },
      {
        'name': 'Appendix II',
      }
    ],
  },
];

/// TODO: This is basically a global, eagerly-loaded God object. If we want to
/// handle incremental refresh better, this should probably be less monolithic.
class Book {
  final List<PartPage> parts = [];
  final List<Page> pages = [];

  final SourceCode code = SourceCode();

  /// The chapter pages that have code.
  Iterable<Page> get codeChapters => pages
      .where((page) =>
          page is ChapterPage &&
          page.part != null &&
          page.part.title != "Backmatter")
      .toList();

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

      PartPage partPage;

      // There are no part pages for the front- and backmatter.
      if (part["name"] != "") {
        partPage = PartPage(partName, partNumber);
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

        var chapterPage = ChapterPage(name, partPage, chapterNumber,
            chapterIndex, chapter["design_note"] as String);
        pages.add(chapterPage);
        if (partPage != null) partPage.chapters.add(chapterPage);
      }
    }

    code.load(this);
  }

  // TODO: The casts here are kind of hokey. Do something cleaner.
  /// Looks for a page with [title].
  ChapterPage findChapter(String title) =>
      pages.firstWhere((page) => page is ChapterPage && page.title == title)
          as ChapterPage;

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

/*
def adjacent_type(title, offset):
  '''Generate template data to link to the previous or next page.'''
  page_index = PAGES.index(title) + offset
  if page_index < 0 or page_index >= len(PAGES): return None

  return TYPES[page_index]


def chapter_number(name):
  """
  Given the name of a chapter (or part of end matter page), finds its number.
  """
  if name in NUMBERS:
    return NUMBERS[name]

  # The chapter has no number. This is true for the appendices.
  return ""


def get_language(name):
  if CODE_CHAPTERS.index(name) < CODE_CHAPTERS.index("Chunks of Bytecode"):
    return "java"
  return "c"


def get_short_name(name):
  number = chapter_number(name)

  first_word = name.split()[0].lower().replace(',', '')
  if first_word == "a" or first_word == "the":
    first_word = name.split()[1].lower()
  if first_word == "user-defined":
    first_word = "user"

  return "chap{0:02d}_{1}".format(number, first_word)

*/
