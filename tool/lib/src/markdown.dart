import 'package:markdown/markdown.dart';

String renderMarkdown(String contents) {
  return markdownToHtml(contents.toString(),
      inlineSyntaxes: [
        // Put inline Markdown code syntax before our smart quotes so that
        // quotes inside `code` spans don't get smartened.
        CodeSyntax(),
        // Smart quotes.
        _ApostropheSyntax(),
        _SmartLeftQuoteSyntax(),
        _SmartRightQuoteSyntax()
      ],
      extensionSet: ExtensionSet.gitHubFlavored);
}

class _ApostropheSyntax extends InlineSyntax {
  _ApostropheSyntax() : super(r"(\S)'(\S)");

  bool onMatch(InlineParser parser, Match match) {
    parser.addNode(Text("${match[1]}&rsquo;${match[2]}"));
    return true;
  }
}

class _SmartLeftQuoteSyntax extends InlineSyntax {
  _SmartLeftQuoteSyntax() : super(r'"(\S)', startCharacter: '"'.codeUnitAt(0));

  bool onMatch(InlineParser parser, Match match) {
    parser.addNode(Text("&ldquo;${match[1]}"));
    return true;
  }
}

class _SmartRightQuoteSyntax extends InlineSyntax {
  _SmartRightQuoteSyntax() : super(r'(\S)"');

  bool onMatch(InlineParser parser, Match match) {
    parser.addNode(Text("${match[1]}&rdquo;"));
    return true;
  }
}
