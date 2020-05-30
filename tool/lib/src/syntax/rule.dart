import 'package:charcode/ascii.dart';

import 'grammar.dart' as grammar;
import 'highlighter.dart';

abstract class Rule {
  final RegExp pattern;

  factory Rule(String pattern, String tokenType) =>
      SimpleRule(pattern, tokenType);

  factory Rule.capture(String pattern, List<String> tokenTypes) =>
      CaptureRule(pattern, tokenTypes);

  Rule._(String pattern) : pattern = RegExp(pattern);

  bool apply(Highlighter highlighter) {
    if (!highlighter.scanner.scan(pattern)) return false;
    applyRule(highlighter);
    return true;
  }

  void applyRule(Highlighter highlighter);
}

/// Parses a single regex and outputs the entire matched text as a single token
/// with the given [tokenType].
class SimpleRule extends Rule {
  final String tokenType;

  SimpleRule(String pattern, this.tokenType) : super._(pattern);

  void applyRule(Highlighter highlighter) {
    highlighter.writeToken(tokenType);
  }
}

/// Parses a single regex where each capture group has a corresponding token
/// type. If the type is `""` for some group, the matched string text is output
/// as plain text.
class CaptureRule extends Rule {
  final List<String> tokenTypes;

  CaptureRule(String pattern, this.tokenTypes) : super._(pattern);

  void applyRule(Highlighter highlighter) {
    var match = highlighter.scanner.lastMatch;
    for (var i = 0; i < tokenTypes.length; i++) {
      var type = tokenTypes[i];
      if (type.isNotEmpty) {
        highlighter.writeToken(type, match[i + 1]);
      } else {
        highlighter.writeText(match[i + 1]);
      }
    }
  }
}

/// Parses string literals and the escape codes inside them.
class StringRule extends Rule {
  static final _escapePattern = RegExp(r"\\.");

  StringRule() : super._('"');

  void applyRule(Highlighter highlighter) {
    var scanner = highlighter.scanner;
    var start = scanner.position - 1;

    while (!scanner.isDone) {
      if (scanner.scan(_escapePattern)) {
        if (scanner.position > start) {
          // TODO: Something smarter than "-2" if we need multi-character
          // escapes.
          highlighter.writeToken(
              "s", scanner.substring(start, scanner.position - 2));
        }
        highlighter.writeToken("e");
        start = scanner.position;
      } else if (scanner.scanChar($double_quote)) {
        highlighter.writeToken("s", scanner.substring(start, scanner.position));
        return;
      } else {
        scanner.position++;
      }
    }

    // Error: Unterminated string.
    highlighter.writeToken("err", scanner.substring(start, scanner.position));
  }
}

/// Parses an identifier and resolves keywords for their token type.
class IdentifierRule extends Rule {
  IdentifierRule() : super._(r"[a-zA-Z_][a-zA-Z0-9_]*");

  void applyRule(Highlighter highlighter) {
    var identifier = highlighter.scanner.lastMatch[0];
    var type = highlighter.language.words[identifier] ?? "i";
    highlighter.writeToken(type);
  }
}
