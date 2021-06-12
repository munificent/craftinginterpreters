import 'package:markdown/markdown.dart' hide HtmlRenderer;

import '../book.dart';
import '../page.dart';
import 'block_syntax.dart';
import 'code_syntax.dart';
import 'html_renderer.dart';
import 'inline_syntax.dart';
import 'xml_renderer.dart';

String renderMarkdown(Book book, Page page, List<String> lines,
    {bool xml = false}) {
  var document = Document(blockSyntaxes: [
    BookHeaderSyntax(page, xml: xml),
    CodeTagBlockSyntax(book, page, xml: xml),
    HighlightedCodeBlockSyntax(xml: xml),
  ], inlineSyntaxes: [
    // Put inline Markdown code syntax before our smart quotes so that
    // quotes inside `code` spans don't get smartened.
    CodeSyntax(),
    EllipseSyntax(xml: xml),
    ApostropheSyntax(xml: xml),
    SmartQuoteSyntax(xml: xml),
    EmDashSyntax(xml: xml),
    if (xml) NewlineSyntax(),
  ], extensionSet: ExtensionSet.gitHubFlavored);

  var ast = document.parseLines(lines);
  if (xml) {
    return XmlRenderer().render(ast);
  } else {
    return HtmlRenderer().render(ast);
  }
}
