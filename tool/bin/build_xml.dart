import 'dart:io';

import 'package:path/path.dart' as p;

import 'package:tool/src/book.dart';
import 'package:tool/src/markdown/markdown.dart';
import 'package:tool/src/markdown/xml_renderer.dart';
import 'package:tool/src/mustache.dart';
import 'package:tool/src/page.dart';
import 'package:tool/src/term.dart' as term;

/// Generate the XML used to import into InDesign.

Future<void> main(List<String> arguments) async {
  var book = Book();
  var mustache = Mustache();

  await Directory(p.join("build", "xml")).create(recursive: true);

  for (var page in book.pages) {
    if (!page.isChapter) continue;

    if (arguments.isNotEmpty && page.fileName != arguments.first) continue;

    _buildPage(book, mustache, page);
  }

  // Output a minimal XML file that contains all tags used in the book.
  var allTagsPath = p.join("build", "xml", "all-tags.xml");
  File(allTagsPath)
      .writeAsStringSync("<chapter>\n${XmlRenderer.tagFileBuffer}\n</chapter>");
}

void _buildPage(Book book, Mustache mustache, Page page) {
  var xml = renderMarkdown(book, page, page.lines, xml: true);

  // Write the output.
  var xmlPath = p.join("build", "xml", "${page.fileName}.xml");
  File(xmlPath).writeAsStringSync(xml);

  print("${term.green('-')} ${page.numberString}. ${page.title}");
}
