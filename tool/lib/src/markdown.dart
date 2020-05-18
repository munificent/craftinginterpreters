import 'package:markdown/markdown.dart';

String renderMarkdown(String contents) {
  return markdownToHtml(contents.toString(),
      inlineSyntaxes: [
        _ApostropheSyntax(),
        _SmartLeftQuoteSyntax(),
        _SmartRightQuoteSyntax()
      ],
      extensionSet: ExtensionSet.gitHubFlavored);
}

// TODO: These do the wrong thing when inside inline backticks. At that point,
// they should be left alone, but these syntaxes still kick in and transform
// the characters.

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
