import 'package:charcode/ascii.dart';
import 'package:string_scanner/string_scanner.dart';

import 'grammar.dart' as grammar;
import 'language.dart';

/// Takes a string of source code and returns a block of HTML with spans for
/// syntax highlighting.
///
/// Wraps the result in a <pre> tag with the given [preClass].
String formatCode(String language, List<String> lines, [String preClass]) {
  return Highlighter(language)._highlight(lines, preClass);
}

class Highlighter {
  final StringBuffer _buffer = StringBuffer();
  StringScanner scanner;
  final Language language;

  /// Whether we are in a multi-line macro started on a previous line.
  bool _inMacro = false;

  Highlighter(String language)
      : language = grammar.languages[language] ??
            (throw "Unknown language '$language'.");

  String _highlight(List<String> lines, [String preClass]) {
    _buffer.write("<pre");
    if (preClass != null) _buffer.write(' class="$preClass"');
    _buffer.writeln(">");
    lines.forEach(_scanLine);
    _buffer.write("</pre>");
    return _buffer.toString();
  }

  void _scanLine(String line) {
    if (line.trim().isEmpty) {
      _buffer.writeln();
      return;
    }

    /// Hackish. If the line ends with `\`, then it is a multi-line macro
    /// definition and we want to highlight subsequent lines like preprocessor
    /// code too.
    if (language == grammar.c && line.endsWith("\\")) _inMacro = true;

    if (_inMacro) {
      writeToken("a", line);
    } else {
      scanner = StringScanner(line);
      while (!scanner.isDone) {
        var found = false;
        for (var rule in language.rules) {
          if (rule.apply(this)) {
            found = true;
            break;
          }
        }

        if (!found) _writeChar(scanner.readChar());
      }
    }

    if (_inMacro && !line.endsWith("\\")) _inMacro = false;

    _buffer.writeln();
  }

  void writeToken(String type, [String text]) {
    text ??= scanner.lastMatch[0];
    _buffer.write('<span class="$type">');
    writeText(text);
    _buffer.write('</span>');
  }

  void writeText(String string) {
    for (var i = 0; i < string.length; i++) {
      _writeChar(string.codeUnitAt(i));
    }
  }

  void _writeChar(int char) {
    switch (char) {
      case $less_than:
        _buffer.write("&lt;");
        break;
      case $greater_than:
        _buffer.write("&gt;");
        break;
      case $single_quote:
        _buffer.write("&#39;");
        break;
      case $double_quote:
        _buffer.write("&quot;");
        break;
      case $ampersand:
        _buffer.write("&amp;");
        break;
      default:
        _buffer.writeCharCode(char);
    }
  }
}
