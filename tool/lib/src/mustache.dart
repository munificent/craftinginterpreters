/// Creates the data map and renders the Mustache templates to HTML.
import 'dart:io';

import 'package:mustache_template/mustache_template.dart';
import 'package:path/path.dart' as p;

import 'book.dart';
import 'page.dart';
import 'text.dart';

/// Maintains the cache of loaded partials and allows rendering templates.
class Mustache {
  /// The directory where template files can be found.
  final String _templateDirectory;

  final Map<String, Template> _templates = {};

  Mustache([String templateDirectory])
      : _templateDirectory = templateDirectory ?? p.join("asset", "mustache");

  String render(Book book, Page page, String body, {String template}) {
    var part = page.part?.title;

    var up = "Table of Contents";
    if (part != null) {
      up = part;
    } else if (page.title == "Table of Contents") {
      up = "Crafting Interpreters";
    }

    var previousPage = book.adjacentPage(page, -1);
    var nextPage = book.adjacentPage(page, 1);
    String nextType;
    if (nextPage != null && nextPage.isChapter) {
      nextType = "Chapter";
    } else if (nextPage != null && nextPage.isPart) {
      nextType = "Part";
    }

    List<Map<String, dynamic>> chapters;
    if (page.isPart) {
      chapters = _makeChapterList(page);
    }

    var isFrontmatter = const {
      "Acknowledgements",
      "Dedication",
    }.contains(page.title);

    var data = <String, dynamic>{
      "is_chapter": part != null,
      "is_part": part == null && page.title != null && !isFrontmatter,
      "is_frontmatter": isFrontmatter,
      "title": page.title,
      "part": part,
      "body": body,
      "sections": _makeSections(page),
      "chapters": chapters,
      "design_note": page.designNote,
      "has_design_note": page.designNote != null,
      "has_challenges": page.hasChallenges,
      "has_challenges_or_design_note":
          page.hasChallenges || page.designNote != null,
      "has_number": page.numberString != "",
      "number": page.numberString,
      // Previous page.
      "has_prev": previousPage != null,
      "prev": previousPage?.title,
      "prev_file": previousPage?.fileName,
      // Next page.
      "has_next": nextPage != null,
      "next": nextPage?.title,
      "next_file": nextPage?.fileName,
      "next_type": nextType,
      "has_up": up != null,
      "up": up,
      "up_file": up != null ? toFileName(up) : null,
      // TODO: Only need this for contents page.
      "part_1": _makePartData(book, 0),
      "part_2": _makePartData(book, 1),
      "part_3": _makePartData(book, 2),
    };

    return _load(template ?? page.template).renderString(data);
  }

  Map<String, dynamic> _makePartData(Book book, int partIndex) {
    var partPage = book.parts[partIndex];
    return <String, dynamic>{
      "title": partPage.title,
      "number": partPage.numberString,
      "file": partPage.fileName,
      "chapters": _makeChapterList(partPage)
    };
  }

  List<Map<String, dynamic>> _makeChapterList(Page part) {
    return [
      for (var chapter in part.chapters)
        <String, dynamic>{
          "title": chapter.title,
          "number": chapter.numberString,
          "file": chapter.fileName,
          "design_note": chapter.designNote?.replaceAll("'", "&rsquo;"),
        }
    ];
  }

  List<Map<String, dynamic>> _makeSections(Page page) {
    var sections = <Map<String, dynamic>>[];

    for (var header in page.headers.values) {
      if (!header.isSpecial && header.level == 2) {
        sections.add(<String, dynamic>{
          "name": header.name,
          "anchor": header.anchor,
          "index": header.headerIndex
        });
      }
    }

    return sections;
  }

  Template _load(String name) {
    return _templates.putIfAbsent(name, () {
      var path = p.join(_templateDirectory, "$name.html");
      return Template(File(path).readAsStringSync(),
          name: path, partialResolver: _load);
    });
  }
}
