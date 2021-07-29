import 'package:markdown/markdown.dart' hide HtmlRenderer;

import '../book.dart';
import '../format.dart';
import '../page.dart';
import 'block_syntax.dart';
import 'code_syntax.dart';
import 'html_renderer.dart';
import 'inline_syntax.dart';
import 'xml_renderer.dart';

String renderMarkdown(Book book, Page page, List<String> lines, Format format) {
  var document = Document(blockSyntaxes: [
    BookHeaderSyntax(page, format),
    CodeTagBlockSyntax(book, page, format),
    HighlightedCodeBlockSyntax(format),
  ], inlineSyntaxes: [
    // Put inline Markdown code syntax before our smart quotes so that
    // quotes inside `code` spans don't get smartened.
    CodeSyntax(),
    EllipseSyntax(format),
    ApostropheSyntax(format),
    SmartQuoteSyntax(format),
    EmDashSyntax(format),
    if (format.isPrint) NewlineSyntax(),
  ], extensionSet: ExtensionSet.gitHubFlavored);

  var ast = document.parseLines(lines);
  if (format.isPrint) {
    return XmlRenderer().render(ast);
  } else {
    return HtmlRenderer().render(ast);
  }
}
