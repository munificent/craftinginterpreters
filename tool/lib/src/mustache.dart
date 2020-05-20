/// Creates the data map and renders the Mustache templates to HTML.
import 'dart:io';

import 'package:mustache_template/mustache_template.dart';
import 'package:path/path.dart' as p;

import 'book.dart';
import 'page.dart';
import 'text.dart';

/// Maintains the cache of loaded partials and allows rendering templates.
class Mustache {
  final Map<String, Template> _templates = {};

  String render(String template, Book book, Page page, String body,
      List<Map<String, String>> sections, bool hasChallenges) {
    String part;
    String designNote;

    if (page is ChapterPage) {
      part = page.part.title;
      designNote = page.designNote;
    }

    var up = "Table of Contents";
    if (part != null) {
      up = part;
    } else if (page.title == "Table of Contents") {
      up = "Crafting Interpreters";
    }

    var previousPage = book.adjacentPage(page, -1);
    var nextPage = book.adjacentPage(page, 1);
    String nextType;
    if (nextPage is ChapterPage) {
      nextType = "Chapter";
    } else if (nextPage is PartPage) {
      nextType = "Part";
    }

    var data = <String, dynamic>{
      "has_title": page.title != null,
      "title": page.title,
      "has_part": part != null,
      "part": part,
      "body": body,
      "sections": sections,
      // TODO:
//    "chapters": get_part_chapters(title),
      "chapters": ["TODO", "chapters"],
      "design_note": designNote,
      "has_design_note": designNote != null,
      "has_challenges": hasChallenges,
      "has_challenges_or_design_note": hasChallenges || designNote != null,
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

    return _load(template).renderString(data);
  }

  Map<String, dynamic> _makePartData(Book book, int partIndex) {
    var partPage = book.parts[partIndex];
    return <String, dynamic>{
      "title": partPage.title,
      "number": partPage.numberString,
      "file": partPage.fileName,
      "chapters": [
        for (var chapter in partPage.chapters)
          {
            "title": chapter.title,
            "number": chapter.numberString,
            "file": chapter.fileName,
            "design_note": chapter.designNote?.replaceAll("'", "&rsquo;"),
          }
      ]
    };
  }

  Template _load(String name) {
    return _templates.putIfAbsent(name, () {
      var path = p.join("asset", "mustache", "$name.html");
      return Template(File(path).readAsStringSync(),
          name: path, partialResolver: _load);
    });
  }
}
