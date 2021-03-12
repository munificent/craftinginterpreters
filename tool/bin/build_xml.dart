import 'dart:io';

import 'package:path/path.dart' as p;

import 'package:tool/src/book.dart';
import 'package:tool/src/markdown/markdown.dart';
import 'package:tool/src/mustache.dart';
import 'package:tool/src/page.dart';
import 'package:tool/src/term.dart' as term;

/// Generate the XML used to import into InDesign.

Future<void> main(List<String> arguments) async {
  var tagRegExp = RegExp(r"<([a-z-_0-9]+)");

  var book = Book();
  var mustache = Mustache();

  await Directory(p.join("build", "xml")).create(recursive: true);

  // Keep track of which chapters each tag appear in so I know what to check.
  var tagChapters = <String, Set<String>>{};

  var buffer = StringBuffer();
  buffer.writeln("<all>");

  for (var page in book.pages) {
    if (!page.isChapter) continue;

    if (arguments.isNotEmpty && page.fileName != arguments.first) continue;

    var xml = _buildPage(book, mustache, page);
    buffer.writeln(xml);

    for (var match in tagRegExp.allMatches(xml)) {
      tagChapters.putIfAbsent(match[1], () => {}).add(page.fileName);
    }
  }

  buffer.writeln("</all>");
  var allPath = p.join("build", "xml", "all.xml");
  File(allPath).writeAsStringSync(buffer.toString());

  var tags = tagChapters.keys.toList();
  tags..sort();

  print("");
  print("Tags:");
  for (var tag in tags) {
    var chapters = tagChapters[tag].toList();
    chapters.sort();

    var first = chapters.take(3).toList();
    var rest = chapters.length <= 3 ? "" : " (${chapters.length - 3} more...)";
    print("- $tag: ${first.join(' ')} $rest");
  }
}

String _buildPage(Book book, Mustache mustache, Page page) {
  var xml = renderMarkdown(book, page, page.lines, xml: true);

  // Write the output.
  var xmlPath = p.join("build", "xml", "${page.fileName}.xml");
  File(xmlPath).writeAsStringSync(xml);

  print("${term.green('-')} ${page.numberString}. ${page.title}");
  return xml;
}
