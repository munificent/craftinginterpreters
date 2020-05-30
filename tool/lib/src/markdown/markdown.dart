import 'package:markdown/markdown.dart';

import '../book.dart';
import '../page.dart';
import 'block_syntax.dart';
import 'code_syntax.dart';
import 'inline_syntax.dart';
import 'renderer.dart';

final _inlineSyntaxes = [
  // Put inline Markdown code syntax before our smart quotes so that
  // quotes inside `code` spans don't get smartened.
  CodeSyntax(),
  EllipseSyntax(),
  ApostropheSyntax(),
  SmartQuoteSyntax(),
  EmDashSyntax(),
];

String renderMarkdown(Book book, Page page, List<String> lines) {
  var document = Document(
      blockSyntaxes: [
        BookHeaderSyntax(page),
        CodeTagBlockSyntax(book, page),
        HighlightedCodeBlockSyntax(),
      ],
      inlineSyntaxes: _inlineSyntaxes,
      extensionSet: ExtensionSet.gitHubFlavored);

  return Renderer().render(document.parseLines(lines)) + '\n';
}
