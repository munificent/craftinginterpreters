import 'package:charcode/ascii.dart';
import 'package:markdown/markdown.dart';

import '../format.dart';

class EllipseSyntax extends InlineSyntax {
  final Format _format;

  EllipseSyntax(this._format) : super(r"\.\.\. ?", startCharacter: $dot);

  bool onMatch(InlineParser parser, Match match) {
    // A Unicode ellipsis doesn't have as much space between the dots as
    // Chicago style mandates so do our own thing.
    parser.addNode(Text(_format.isPrint
        ? "&thinsp;.&thinsp;.&thinsp;.&thinsp;"
        : '<span class="ellipse">&thinsp;.&thinsp;.&thinsp;.&nbsp;</span>'));
    return true;
  }
}

class ApostropheSyntax extends InlineSyntax {
  final Format _format;

  ApostropheSyntax(this._format) : super(r"'", startCharacter: $apostrophe);

  bool onMatch(InlineParser parser, Match match) {
    var before = -1;
    if (parser.pos > 0) {
      before = parser.charAt(parser.pos - 1);
    }
    var after = -1;
    if (parser.pos < parser.source.length - 1) {
      after = parser.charAt(parser.pos + 1);
    }

    var isRight = _isRight(before, after);
    String quote;
    if (_format.isPrint) {
      quote = isRight ? "#8217" : "#8216";
    } else {
      quote = isRight ? "rsquo" : "lsquo";
    }
    parser.addNode(Text("&$quote;"));
    return true;
  }

  bool _isRight(int before, int after) {
    // Years like "the '60s".
    if (before == $space && after >= $0 && after <= $9) return true;

    // Possessive after code.
    if (before == $backquote && after == $s) return true;

    if (before == $space) return false;
    if (before == $lf) return false;

    // Default to right.
    return true;
  }
}

class SmartQuoteSyntax extends InlineSyntax {
  final Format _format;

  SmartQuoteSyntax(this._format) : super(r'"', startCharacter: $double_quote);

  bool onMatch(InlineParser parser, Match match) {
    var before = -1;
    if (parser.pos > 0) {
      before = parser.charAt(parser.pos - 1);
    }
    var after = -1;
    if (parser.pos < parser.source.length - 1) {
      after = parser.charAt(parser.pos + 1);
    }

    var isRight = _isRight(before, after);
    String quote;
    if (_format.isPrint) {
      quote = isRight ? "#8221" : "#8220";
    } else {
      quote = isRight ? "rdquo" : "ldquo";
    }

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
  final Format _format;

  EmDashSyntax(this._format) : super(r"\s--\s");

  bool onMatch(InlineParser parser, Match match) {
    parser.addNode(
        Text(_format.isPrint ? '—' : '<span class="em">&mdash;</span>'));
    return true;
  }
}

/// Remove newlines in paragraphs and turn them into spaces since InDesign
/// treats them as line breaks.
class NewlineSyntax extends InlineSyntax {
  NewlineSyntax() : super("\n", startCharacter: $lf);

  bool onMatch(InlineParser parser, Match match) {
    parser.addNode(Text(" "));
    return true;
  }
}
