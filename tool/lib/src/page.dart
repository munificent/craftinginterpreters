import 'dart:io';

import 'package:path/path.dart' as p;

import 'book.dart';
import 'snippet_tag.dart';
import 'text.dart';

/// One page (in the HTML sense) of the book.
///
/// Each chapter, part introduction, and backmatter section is a page.
class Page {
  static final all = _flattenPages();

  /// The chapter pages that have code.
  static final codeChapters = all
      .where((page) =>
  page.part != "" && page.part != "" && page.part != "Backmatter")
      .toList();

  /// Looks for a page with [title].
  static Page find(String title) =>
      all.firstWhere((page) => page.title == title);

//  // TODO: Not needed?
//  /// Parses the snippet tags from every chapter. Returns a map of chapter names
//  /// to maps of snippet names to SnippetTags.
//  static Map<Page, Map<String, SnippetTag>> getChapterSnippetTags() {
//    var chapters = {};
//
//    for (var chapter in Page.codeChapters) {
//      chapters[chapter] = chapter.snippetTags();
//
////    chapters.forEach((chapter, tags) {
////      print(chapter);
////      for (var tag in tags.values) {
////        print("  $tag");
////      }
////    });
//    }
//
//    return chapters;
//  }

  /// The title of this page.
  final String title;

  /// The part that contains this page, or null if the page is not a chapter
  /// within a part.
  final String part;

  // TODO: Enum.
  final String type;

  /// The chapter or part number, like "12", "II", or "".
  final String numberString;

  /// If the page is a chapter, the numeric index of it or `null` otherwise.
  final int chapterIndex;

  Map<String, SnippetTag> _snippetTags;

  Page(this.title, this.part, this.type, this.numberString, this.chapterIndex);

  /// The base file path and URI for the page, without any extension.
  String get fileName {
    if (title == "Crafting Interpreters") return "index";
    if (title == "Table of Contents") return "contents";

    // TODO: Is this still needed?
    // Hack. The introduction has a *subheader* named "Challenges" distinct from
    // the challenges section. This function here is also used to generate the
    // anchor names for the links, so handle that one specially so it doesn't
    // collide with the real "Challenges" section.
    if (title == "Challenges") return "challenges_";

    return toFileName(title);
  }

  /// The path to this page's Markdown source file.
  String get markdownPath => p.join("book", "$fileName.md");

  // TODO: Change to "site" when working.
  /// The path to this page's generated HTML file.
  String get htmlPath => p.join("site_dart", "$fileName.html");

  Page get previous => _adjacent(-1);
  Page get next => _adjacent(1);

  /// Flatten the tree of parts and chapters to a single linear list of pages.
  static List<Page> _flattenPages() {
    var pages = <Page>[];

    var partIndex = 1;
    var chapterIndex = 1;
    var inMatter = false;

    for (var part in tableOfContents) {
      // Front- and backmatter have no names, pages, or numbers.
      var partNumber = "";
      var partName = part["name"];
      inMatter = partName == "" || partName == "Backmatter";
      if (!inMatter) {
        partNumber = roman(partIndex);
        partIndex += 1;
      }

      // There are no part pages for the front- and backmatter.
      if (part["name"] != "") {
        pages.add(Page(partName, null, "Part", partNumber, null));
      }

      for (var chapter in part["chapters"]) {
        var name = chapter["name"];

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

        pages.add(Page(name, partName, "Chapter", chapterNumber, chapterIndex));
      }
    }

    return pages;
  }

  SnippetTag findSnippetTag(String name) {
    // TODO: snippetTags() parses the file each time. Do something caching
    // somewhere.
    var tag = snippetTags[name];
    if (tag != null) return tag;

    print("Could not find snippet '$name' in chapter '$title'.");

//    if name != 'not-yet' and name != 'omit':
//      print('Error: "{}" does not use snippet "{}".'.format(chapter, name),
//          file=sys.stderr)

    // Synthesize a fake one so we can keep going.
    return snippetTags[name] = SnippetTag(this, name, snippetTags.length);

//  def last_snippet_for_chapter(self, chapter):
//    """ Returns the last snippet tag appearing in [chapter]. """
//    snippets = self.snippet_tags[chapter]
//    last = None
//    for snippet in snippets.values():
//      if not last or snippet > last:
//        last = snippet
//
//    return last
  }

  /// Parses the page's Markdown file and finds all of the `^code` tags.
  ///
  /// Returns a map of snippet names to SnippetTags for them.
  Map<String, SnippetTag> get snippetTags {
    if (_snippetTags != null) return _snippetTags;

    // TODO: Redundant with code in build.dart that parses commands. Unify?
    final _codeTagPattern = RegExp(r"\s*\^code ([-a-z0-9]+).*");

    _snippetTags = {};

    // TODO: Each Markdown file gets read from disc twice. Once to find all the
    // snippet tags and once when building. Merge those two into one read.
    // (Maybe just cache the read lines in this class?)
    for (var line in File(markdownPath).readAsLinesSync()) {
      var match = _codeTagPattern.firstMatch(line);
      if (match != null) {
        _snippetTags[match.group(1)] =
            SnippetTag(this, match.group(1), _snippetTags.length);
      }
    }

    // Add fake tags for the placeholders.
    _snippetTags["omit"] = SnippetTag(this, "omit", _snippetTags.length);
    _snippetTags["not-yet"] = SnippetTag(this, "not-yet", _snippetTags.length);

    return _snippetTags;
  }

  String toString() => title;

  /// Gets the [Page] [offset] pages before or after this one.
  Page _adjacent(int offset) {
    var index = all.indexOf(this) + offset;
    if (index < 0 || index >= all.length) return null;
    return all[index];
  }
}
