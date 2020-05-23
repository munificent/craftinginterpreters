import 'package:charcode/ascii.dart';
import 'package:markdown/markdown.dart';

import 'highlighter.dart';
import 'text.dart';

String renderMarkdown(String contents) {
  return markdownToHtml(contents.toString(),
      blockSyntaxes: [
        HighlightedCodeBlockSyntax(),
        TripleColonCodeBlockSyntax(),
      ],
      inlineSyntaxes: [
        // Put inline Markdown code syntax before our smart quotes so that
        // quotes inside `code` spans don't get smartened.
        CodeSyntax(),
        // Smart quotes.
        _ApostropheSyntax(),
        _SmartQuoteSyntax(),
      ],
      extensionSet: ExtensionSet.gitHubFlavored);
}

class _ApostropheSyntax extends InlineSyntax {
  _ApostropheSyntax() : super(r"'", startCharacter: $apostrophe);

  bool onMatch(InlineParser parser, Match match) {
    var before = -1;
    if (parser.pos > 0) {
      before = parser.charAt(parser.pos - 1);
    }

    var quote = _isRight(before) ? "rsquo" : "lsquo";
    parser.addNode(Text("&$quote;"));
    return true;
  }

  bool _isRight(int before) {
    if (before == $space) return false;

    // Default to right.
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
    if (after == $dot) return true;

    // Default to left.
    return false;
  }
}

/// Custom code block formatter that uses our syntax highlighter.
class HighlightedCodeBlockSyntax extends BlockSyntax {
  // TODO: Can we remove the spaces from this?
  static final _codeFencePattern = RegExp(r'^[ ]{0,3}```(.*)$');

  RegExp get pattern => _codeFencePattern;

  bool canParse(BlockParser parser) =>
      pattern.firstMatch(parser.current) != null;

  List<String> parseChildLines(BlockParser parser) {
    var childLines = <String>[];
    parser.advance();

    while (!parser.isDone) {
      var match = pattern.firstMatch(parser.current);
      if (match == null) {
        childLines.add(parser.current);
        parser.advance();
      } else {
        parser.advance();
        break;
      }
    }

    return childLines;
  }

  Node parse(BlockParser parser) {
    // Get the syntax identifier, if there is one.
    var match = pattern.firstMatch(parser.current);
    var language = match.group(1);

    var childLines = parseChildLines(parser);

    // The Markdown tests expect a trailing newline.
    childLines.add("");

    return _formatCodeLines(language, childLines);
  }
}


/// Custom code block formatter that uses our syntax highlighter.
///
/// Recognizes Python Markdown's ":::language" syntax.
// TODO: Remove this when those code blocks have been migrated to use fences.
class TripleColonCodeBlockSyntax extends BlockSyntax {
  static final _startPattern = RegExp(r'^( *):::(.*)$');
  static final _indentationPattern = RegExp(r'^( *)(.+)$');

  RegExp get pattern => _startPattern;

  bool canParse(BlockParser parser) =>
      pattern.firstMatch(parser.current) != null;

  List<String> parseChildLines(BlockParser parser, [String indentation]) {
    var childLines = <String>[];
    parser.advance();

    while (!parser.isDone) {
      // Stop when we hit a non-empty line whose indentation is less than the
      // ":::" line.
      var match = _indentationPattern.firstMatch(parser.current);
      if (match != null && match[1].length < indentation.length) break;

      childLines.add(parser.current);
      parser.advance();
    }

    return childLines;
  }

  Node parse(BlockParser parser) {
    // Get the syntax identifier, if there is one.
    var match = pattern.firstMatch(parser.current);
    var indentation = match[1];
    var language = match[2];

    var childLines = parseChildLines(parser, indentation);
    return _formatCodeLines(language, childLines);
  }
}

Element _formatCodeLines(String language, List<String> childLines) {
  String code;
  if (language == "text") {
    // Don't syntax highlight text.
    code = escapeHtml(childLines.join("\n"));

    // TODO: The Markdown/Pygments puts a pointless empty span at the
    // beginning. Remove this when not trying to match that.
    code = "<span></span>$code";
  } else {
    // TODO: Find a cleaner way to handle this. Maybe move the trailing
    // newline code into `insertSnippet()`?
    // Remove the trailing empty line so that `formatCode()` doesn't put a
    // <br> at the end.
    childLines.removeLast();
    code = formatCode(language, 72, childLines);
  }

  var element = Element.text("pre", code);

  // TODO: Remove this when no longer trying to match the existing output.
  // Wrap in codehilite div.
  element = Element("div", [element]);
  element.attributes["class"] = "codehilite";
  return element;
}