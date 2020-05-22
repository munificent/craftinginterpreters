import 'package:charcode/ascii.dart';
import 'package:markdown/markdown.dart';

String renderMarkdown(String contents) {
  return markdownToHtml(contents.toString(),
      inlineSyntaxes: [
        // Put inline Markdown code syntax before our smart quotes so that
        // quotes inside `code` spans don't get smartened.
        CodeSyntax(),
        // Smart quotes.
        _ApostropheSyntax(),
        _SmartQuoteSyntax()
      ],
      extensionSet: ExtensionSet.gitHubFlavored);
}

class _ApostropheSyntax extends InlineSyntax {
  _ApostropheSyntax() : super(r"'", startCharacter: $apostrophe);

  bool onMatch(InlineParser parser, Match match) {
    parser.addNode(Text("&rsquo;"));
    return true;
  }
}

class _SmartQuoteSyntax extends InlineSyntax {
  _SmartQuoteSyntax() : super(r'"', startCharacter: $double_quote);

  bool onMatch(InlineParser parser, Match match) {
    var before = -1;
    if (parser.pos > 0) {
      before = parser.charAt(parser.pos - 1);
    }
    var after = -1;
    if (parser.pos < parser.source.length - 1) {
      after = parser.charAt(parser.pos + 1);
    }

    var quote = _isRight(before, after) ? "rdquo" : "ldquo";
    parser.addNode(Text("&$quote;"));
    return true;
  }

  bool _isRight(int before, int after) {
    if (after == $space) return true;
    if (before >= $a && before <= $z) return true;
    if (before >= $A && before <= $Z) return true;
    if (before >= $0 && before <= $9) return true;
    if (before == $dot) return true;
    if (before == $question) return true;
    if (before == $exclamation) return true;
    if (after == $comma) return true;

    // Default to left.
    return false;
  }
}
