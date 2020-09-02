import 'dart:io';

import 'package:path/path.dart' as p;

import 'package:tool/src/book.dart';
import 'package:tool/src/markdown/markdown.dart';
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
}

void _buildPage(Book book, Mustache mustache, Page page) {
  var body = renderMarkdown(book, page, page.lines, xml: true);
  var output = mustache.render(book, page, body, template: "in_design");

  // Turn aside markers in code into spans. In the empty span case, insert a
  // zero-width space because Chrome seems to lose the span's position if it has
  // no content.
  // <span class="c">// [repl]</span>
  // TODO: Do this directly in the syntax highlighter instead of after the fact.
//  output = output.replaceAllMapped(_asideHighlightedCommentPattern,
//      (match) => '<span name="${match[1]}"> </span>');
//  output = output.replaceAllMapped(_asideHighlightedWithCommentPattern,
//      (match) => '<span class="c" name="${match[2]}">// ${match[1]}</span>');
//  output = output.replaceAllMapped(
//      _asideCommentPattern, (match) => '<span name="${match[1]}"> </span>');
//  output = output.replaceAllMapped(_asideWithCommentPattern,
//      (match) => '<span name="${match[2]}">// ${match[1]}</span>');

//  output = output.replaceAll('<br>', '<br/>');

  // Write the output.
  var xmlPath = p.join("build", "xml", "${page.fileName}.xml");
  File(xmlPath).writeAsStringSync(output);

  print("${term.green('-')} ${page.numberString}. ${page.title}");
}
