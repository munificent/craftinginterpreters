import 'package:charcode/ascii.dart';
import 'package:string_scanner/string_scanner.dart';

import '../term.dart' as term;
import 'grammar.dart' as grammar;
import 'language.dart';

const _maxLineLength = 67;

/// Takes a string of source code and returns a block of HTML with spans for
/// syntax highlighting.
///
/// Wraps the result in a <pre> tag with the given [preClass].
String formatCode(String language, List<String> lines,
    {String preClass, int indent = 0, bool xml = false}) {
  return Highlighter(language, xml: xml)._highlight(lines, preClass, indent);
}

void checkLineLength(String line) {
  final asideCommentPattern = RegExp(r' +// \[([-a-z0-9]+)\]');
  final asideWithCommentPattern = RegExp(r' +// (.+) \[([-a-z0-9]+)\]');

  line = line.replaceAll(asideCommentPattern, '');
  line = line.replaceAll(asideWithCommentPattern, '');

  if (line.length <= _maxLineLength) return;

  print(line.substring(0, _maxLineLength) +
      term.red(line.substring(_maxLineLength)));
}

class Highlighter {
  final bool _isXml;
  final StringBuffer _buffer = StringBuffer();
  StringScanner scanner;
  final Language language;

  /// Whether we are in a multi-line macro started on a previous line.
  bool _inMacro = false;

  Highlighter(String language, {bool xml = false})
      : language = grammar.languages[language] ??
            (throw "Unknown language '$language'."),
        _isXml = xml;

  String _highlight(List<String> lines, String preClass, int indent) {
    if (!_isXml) {
      _buffer.write("<pre");
      if (preClass != null) _buffer.write(' class="$preClass"');
      _buffer.writeln(">");
    }

    for (var line in lines) {
      _scanLine(line, indent);
    }

    if (!_isXml) _buffer.write("</pre>");
    return _buffer.toString();
  }

  void _scanLine(String line, int indent) {
    if (line.trim().isEmpty) {
      _buffer.writeln();
      return;
    }

    // If the entire code block is indented, remove that indentation from the
    // code lines.
    if (line.length > indent) line = line.substring(indent);

    checkLineLength(line);

    // Hackish. If the line ends with `\`, then it is a multi-line macro
    // definition and we want to highlight subsequent lines like preprocessor
    // code too.
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

    if (_isXml) {
      // Only highlight keywords and comments in XML.
      var tag = {"k": "keyword", "c": "comment"}[type];

      if (tag != null) _buffer.write("<$tag>");
      writeText(text);
      if (tag != null) _buffer.write("</$tag>");
    } else {
      _buffer.write('<span class="$type">');
      writeText(text);
      _buffer.write('</span>');
    }
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
