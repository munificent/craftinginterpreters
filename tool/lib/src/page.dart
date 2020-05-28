import 'package:path/path.dart' as p;

import 'code_tag.dart';
import 'page_parser.dart';
import 'text.dart';

/// One page (in the HTML sense) of the book.
///
/// Each chapter, part introduction, and backmatter section is a page.
class Page {
  /// The title of this page.
  final String title;

  /// The chapter or part number, like "12", "II", or "".
  final String numberString;

  /// The numeric index of the page in chapter order.
  ///
  /// Used to determine which order snippets appear in the book.
  final int ordinal;

  /// If this page is a part page, the list of chapter pages it contains.
  final List<Page> chapters = [];

  /// If this page is a chapter page, the part that contains this page.
  final Page part;

  PageFile _file;

  Page(this.title, this.part, this.numberString, this.ordinal);

  /// The base file path and URI for the page, without any extension.
  String get fileName => toFileName(title);

  /// The path to this page's Markdown source file.
  String get markdownPath => p.join("book", "$fileName.md");

  // TODO: Change to "site" when working.
  /// The path to this page's generated HTML file.
  String get htmlPath => p.join("build", "site_dart", "$fileName.html");

  /// Whether this page is a chapter page, as opposed to a part.
  bool get isChapter => part != null;

  /// Whether this page is a part page, as opposed to a chapter.
  bool get isPart => part == null;

  /// The code language used for this chapter page or `null` if this isn't one
  /// of the main chapter pages.
  String get language {
    if (isPart) return null;
    if (part.title == "A Tree-Walk Interpreter") return "java";
    if (part.title == "A Bytecode Virtual Machine") return "c";
    return null;
  }

  String get shortName {
    var number = numberString.padLeft(2, "0");

    var words = title.split(" ");
    var word = words.first.toLowerCase();
    if (word == "a" || word == "the") word = words[1].toLowerCase();

    return "chap${number}_$word";
  }

  List<String> get lines => _ensureFile().lines;

  String get template => _ensureFile().template;

  Map<String, Header> get headers => _ensureFile().headers;

  bool get hasChallenges => _ensureFile().hasChallenges;

  String get designNote => _ensureFile().designNote;

  Iterable<CodeTag> get codeTags => _ensureFile().codeTags.values;

  CodeTag findCodeTag(String name) {
    // Return fake tags for the placeholders.
    if (name == "omit") return CodeTag(this, "omit", 9998, 0, 0, false);
    if (name == "not-yet") return CodeTag(this, "omit", 9999, 0, 0, false);

    var codeTag = _ensureFile().codeTags[name];
    if (codeTag != null) return codeTag;

    throw ArgumentError("Could not find code tag '$name'.");
  }

  String toString() => title;

  /// Lazily parse the Markdown file for the page.
  PageFile _ensureFile() => _file ??= parsePage(this);
}

/// The data for a page parsed from the Markdown source.
class PageFile {
  final List<String> lines;
  final String template;
  final Map<String, Header> headers;
  final bool hasChallenges;

  /// The name of the design note in this page, or `null` if there is none.
  final String designNote;

  final Map<String, CodeTag> codeTags;

  PageFile(this.lines, this.template, this.headers, this.hasChallenges,
      this.designNote, this.codeTags);
}

/// A section header in a page.
class Header {
  /// The header depth: 1 is the page title, 2 header, 3 subheader.
  final int level;
  final int headerIndex;
  final int subheaderIndex;
  final String name;

  Header(this.level, this.headerIndex, this.subheaderIndex, this.name);

  /// Whether this header is for the special "Challenges" or "Design Note"
  /// sections.
  bool get isSpecial => isChallenges || isDesignNote;

  bool get isChallenges {
    // Check for a subheader because there is a "Challenges" *subheader* in
    // the Introduction.
    return name == "Challenges" && level == 2;
  }

  bool get isDesignNote => name.startsWith("Design Note:");

  String get anchor {
    if (isChallenges) return "challenges";
    if (isDesignNote) return "design-note";
    return toFileName(name);
  }
}
