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

  // TODO: These are just for compatibility with the old output. The Markdown
  // files already have a number of other non-ASCII characters which are not
  // escaped. I think I only did these because an old Python Markdown didn't
  // handle non-ASCII. Remove this.
  UnicodeEscapeSyntax("à".codeUnitAt(0), "agrave"),
  UnicodeEscapeSyntax("ï".codeUnitAt(0), "iuml"),
  UnicodeEscapeSyntax("ø".codeUnitAt(0), "oslash"),
  UnicodeEscapeSyntax("æ".codeUnitAt(0), "aelig"),
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
