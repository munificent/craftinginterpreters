import 'package:charcode/ascii.dart';
import 'package:markdown/markdown.dart';

class EllipseSyntax extends InlineSyntax {
  EllipseSyntax() : super(r"\.\.\.", startCharacter: $dot);

  bool onMatch(InlineParser parser, Match match) {
    parser.addNode(Text("&hellip;"));
    return true;
  }
}

class ApostropheSyntax extends InlineSyntax {
  ApostropheSyntax() : super(r"'", startCharacter: $apostrophe);

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
    if (before == $lf) return false;

    // Default to right.
    return true;
  }
}

class SmartQuoteSyntax extends InlineSyntax {
  SmartQuoteSyntax() : super(r'"', startCharacter: $double_quote);

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

    if (after == $colon) return true;
    if (after == $comma) return true;
    if (after == $dot) return true;

    // Default to left.
    return false;
  }
}

class EmDashSyntax extends InlineSyntax {
  EmDashSyntax() : super(r"\s--\s");

  bool onMatch(InlineParser parser, Match match) {
    parser.addNode(Text('<span class="em">&mdash;</span>'));
    return true;
  }
}

/// Replaces a single Unicode code point with its HTML escape sequence.
class UnicodeEscapeSyntax extends InlineSyntax {
  final String _escape;

  UnicodeEscapeSyntax(int character, this._escape)
      : super(String.fromCharCode(character), startCharacter: character);

  bool onMatch(InlineParser parser, Match match) {
    parser.addNode(Text("&$_escape;"));
    return true;
  }
}
