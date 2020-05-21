import 'package:charcode/ascii.dart';
import 'package:string_scanner/string_scanner.dart';

/// Match these first because they apply smarter styles in certain contexts.
final _attributePattern = RegExp(r"\.([a-zA-Z_][a-zA-Z0-9_]*)");
final _classPattern = RegExp(r"class (\w+)");
final _packagePattern = RegExp(r"package (\w+(\.\w+)*);");

final _annotationPattern = RegExp(r"@[a-zA-Z_][a-zA-Z0-9_]*");
final _identifierPattern = RegExp(r"[a-zA-Z_][a-zA-Z0-9_]*");
final _integerPattern = RegExp(r"[0-9]+L?");
final _lineCommentPattern = RegExp(r"//.*");
final _operatorPattern = RegExp(r"[(){}[\]!+\-/*:;.,|?=<>&]+");

final _includePattern = RegExp(r"(#include)(\s+)(.*)");

// TODO: This includes all trailing whitespace, which I guess is technically
// correct but looks a little funny. Pygments works this way. If we don't want
// to match that, a petter regex is "#[a-zA-Z_][a-zA-Z0-9_]*".
final _preprocessorPattern = RegExp(r"#.*");
// TODO: Multi-character escapes?
final _charPattern = RegExp(r"'(\\?.)'");
final _stringEscapePattern = RegExp(r"\\.");

// TODO: More.
final _keywords = {
  // Reserved words.
  "break": "k",
  "case": "k",
  "const": "k",
  "continue": "k",
  "default": "k",
  "do": "k",
  "else": "k",
  "enum": "k",
  "extern": "k",
  "for": "k",
  "if": "k",
  "instanceof": "k",
  "return": "k",
  "static": "k",
  "struct": "k",
  "switch": "k",
  "this": "k",
  "typedef": "k",
  "while": "k",

  // Constants.
  "false": "kc",
  "null": "kc",
  "true": "kc",

  // Types.
  "bool": "k",
  "boolean": "k",
  "char": "k",
  "double": "k",
  "FILE": "k",
  "int": "k",
  "size_t": "k",
  "void": "k",

  // Declarators.
  "class": "k",
  "extends": "k",
  "implements": "k",
  "private": "k",
  "protected": "k",
  "public": "k",

  // Built-in names.
  "NULL": "nb",
  "this": "nb"
};

/// Takes a string of source code and returns a block of HTML with spans for
/// syntax highlighting.
String formatCode(String language, int length, List<String> lines,
        [String preClass]) =>
    Highlighter(length)._highlight(lines, preClass);

class Highlighter {
  final int _lineLength;
  final StringBuffer _buffer = StringBuffer();
  StringScanner _scanner;

  Highlighter(this._lineLength);

  String _highlight(List<String> lines, String preClass) {
    // TODO: Pass in StringBuffer.
    _buffer.write("<pre");
    if (preClass != null) _buffer.write(' class="$preClass"');
    _buffer.write(">");

    // TODO: If we change build to not pass this output through the Markdown
    // parser, then revisit this.
    // Hack. Markdown seems to discard leading and trailing newlines, so we'll
    // add them back ourselves.
    var leadingNewlines = 0;
    while (leadingNewlines < lines.length &&
        lines[leadingNewlines].trim().isEmpty) {
      leadingNewlines++;
      _buffer.write("<br>");
    }

    var trailingNewlines = 0;
    while (trailingNewlines < lines.length - leadingNewlines &&
        lines[lines.length - trailingNewlines - 1].trim().isEmpty) {
      trailingNewlines++;
    }

    // TODO: Temp hack. Munge it some to match the old build output.
    _buffer.write("<span></span>");

    for (var i = leadingNewlines; i < lines.length - trailingNewlines; i++) {
      _scanLine(lines[i]);
    }

    for (var i = 0; i < trailingNewlines; i++) {
      _buffer.write("<br>");
    }

    //  # Strip off the div wrapper. We just want the <pre>.
//  html = html.replace('<div class="codehilite">', '')
//  html = html.replace('</div>', '')

    _buffer.writeln("</pre>");
    return _buffer.toString();
  }

  void _scanLine(String line) {
    if (line.trim().isEmpty) {
      _buffer.writeln();
      return;
    }

    // TODO: Reuse scanner for all lines?
    _scanner = StringScanner(line.padRight(_lineLength, " "));

    while (!_scanner.isDone) {
      if (_scanner.scan(_includePattern)) {
        _token("cp", _scanner.lastMatch[1]);
        _write(_scanner.lastMatch[2]);
        _token("cpf", _scanner.lastMatch[3]);
      } else if (_scanner.scan(_attributePattern)) {
        _token("o", ".");
        _token("n", _scanner.lastMatch[1]);
      } else if (_scanner.scan(_classPattern)) {
        _token("k", "class");
        _buffer.write(" ");
        _token("nc", _scanner.lastMatch[1]);
      } else if (_scanner.scan(_packagePattern)) {
        _token("k", "package");
        _buffer.write(" ");
        _token("n", _scanner.lastMatch[1]);
        _token("o", ";");
      } else if (_scanner.scan(_annotationPattern)) {
        _token("nd");
      } else if (_scanner.scan(_charPattern)) {
        _token("s", "'${_scanner.lastMatch[1]}'");
      } else if (_scanner.scan(_identifierPattern)) {
        var identifier = _scanner.lastMatch[0];
        _token(_keywords[identifier] ?? "n");
      } else if (_scanner.scan(_integerPattern)) {
        _token("mi");
      } else if (_scanner.scan(_lineCommentPattern)) {
        _token("c1");
      } else if (_scanner.scan(_operatorPattern)) {
        _token("o");
      } else if (_scanner.scan(_preprocessorPattern)) {
        _token("cp");
      } else if (_scanner.scan('"')) {
        var start = _scanner.position - 1;

        while (!_scanner.isDone) {
          if (_scanner.scan(_stringEscapePattern)) {
            if (_scanner.position > start) {
              // TODO: Something smarter than "-2" if we need multi-character
              // escapes.
              _token("s", _scanner.substring(start, _scanner.position - 2));
            }
            _token("se");
            start = _scanner.position;
          } else if (_scanner.scanChar($double_quote)) {
            _token("s", _scanner.substring(start, _scanner.position));
            break;
          } else {
            _scanner.position++;
          }
        }
      } else {
        _writeChar(_scanner.readChar());
      }
    }

    _buffer.writeln();
  }

  void _token(String type, [String text]) {
    text ??= _scanner.lastMatch[0];
    _buffer.write('<span class="$type">');
    _write(text);
    _buffer.write('</span>');
  }

  void _write(String string) {
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
