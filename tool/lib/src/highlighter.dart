import 'package:charcode/ascii.dart';
import 'package:string_scanner/string_scanner.dart';

/// Match these first because they apply smarter styles in certain contexts.
final _attributePattern = RegExp(r"\.([a-zA-Z_][a-zA-Z0-9_]*)");
final _classPattern = RegExp(r"class (\w+)");
final _packagePattern = RegExp(r"package (\w+(\.\w+)*);");

final _annotationPattern = RegExp(r"@[a-zA-Z_][a-zA-Z0-9_]*");
final _floatPattern = RegExp(r"[0-9]+\.[0-9]+f?");
final _identifierPattern = RegExp(r"[a-zA-Z_][a-zA-Z0-9_]*");
final _integerPattern = RegExp(r"[0-9]+L?");
final _labelPattern = RegExp(r"([a-zA-Z_][a-zA-Z0-9_]*)(\s*)(:)");
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
  "uint8_t": "k",
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
///
/// Does not wrap the result in a <pre> tag.
String formatCode(String language, int length, List<String> lines) {
  // TODO: Pass in StringBuffer.
  var buffer = StringBuffer();
  Highlighter(buffer, length)._highlight(lines);
  return buffer.toString();
}

/// Takes a string of source code and returns a block of HTML with spans for
/// syntax highlighting.
///
/// Wraps the result in a <pre> tag with the given [preClass].
String formatPre(String language, int length, List<String> lines,
    [String preClass]) {
  // TODO: Pass in StringBuffer.
  var buffer = StringBuffer();

  buffer.write("<pre");
  if (preClass != null) buffer.write(' class="$preClass"');
  buffer.write(">");

  Highlighter(buffer, length)._highlight(lines);

  buffer.writeln("</pre>");
  return buffer.toString();
}

class Highlighter {
  final int _lineLength;
  final StringBuffer _buffer;
  StringScanner _scanner;

  Highlighter(this._buffer, this._lineLength);

  String _highlight(List<String> lines) {
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
      } else if (_scanner.scan(_floatPattern)) {
        _token("mf");
      } else if (_scanner.scan(_labelPattern)) {
        var name = _scanner.lastMatch[1];
        var space = _scanner.lastMatch[2];
        var colon = _scanner.lastMatch[3];
        _token("nl", name);
        // TODO: Allowing space here means that this incorrectly parses an
        // identifier before a conditional operator as a label name. Pygments
        // does this wrong so this matches that. Once we aren't trying to match
        // exactly, remove the (\s*) from the regex to fix that.
        _write(space);
        _token("o", colon);
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
