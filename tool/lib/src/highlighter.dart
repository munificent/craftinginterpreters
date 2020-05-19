import 'package:string_scanner/string_scanner.dart';

final _classPattern = RegExp(r"class (\w+)");
final _packagePattern = RegExp(r"package (\w+(\.\w+)*);");

final _annotationPattern = RegExp(r"@[a-zA-Z_][a-zA-Z0-9_]*");
final _identifierPattern = RegExp(r"[a-zA-Z_][a-zA-Z0-9_]*");
final _lineCommentPattern = RegExp(r"//.*");
final _operatorPattern = RegExp(r"[(){}[\]!+\-/*:;.,|?=]+");

// TODO: More.
final _keywords = {
  // Reserved words.
  "case": "k",
  "for": "k",
  "if": "k",
  "instanceof": "k",
  "return": "k",
  "switch": "k",
  "while": "k",

  // Constants.
  "false": "kc",
  "null": "kc",
  "true": "kc",

  // Types.
  "bool": "kt",
  "boolean": "kt",
  "double": "kt",
  "int": "kt",
  "void": "kt",

  // Declarators.
  "class": "kd",
  "extends": "kd",
  "implements": "kd",
  "private": "kd",
  "protected": "kd",
  "public": "kd",
};

/// Takes a string of source code and returns a block of HTML with spans for
/// syntax highlighting.
String formatCode(String language, int length, List<String> lines, [String preClass]) {
  // TODO: Pass in StringBuffer.
  var buffer = StringBuffer();

  buffer.write("<pre");
  if (preClass != null) buffer.write(' class="$preClass"');
  buffer.write(">");

  // TODO: Temp hack. Munge it some to match the old build output.
  buffer.write("<span></span>");

//  # Hack. Markdown seems to discard leading and trailing newlines, so we'll
//  # add them back ourselves.
//  leading_newlines = 0
//  while lines and lines[0].strip() == '':
//    lines = lines[1:]
//    leading_newlines += 1
//
//  trailing_newlines = 0
//  while lines and lines[-1].strip() == '':
//    lines = lines[:-1]
//    trailing_newlines += 1
//
  for (var line in lines) {
    // TODO: Reuse scanner for all lines?
    var scanner = StringScanner(line.padRight(length, " "));

    void token(String type, [String text]) {
      text ??= scanner.lastMatch.group(0);
      buffer.write('<span class="$type">$text</span>');
    }

    while (!scanner.isDone) {
      if (scanner.scan("&")) {
        token("o", "&amp;");
      } else if (scanner.scan("<")) {
        token("o", "&lt;");
      } else if (scanner.scan(">")) {
        token("o", "&gt;");
      } else if (scanner.scan(_classPattern)) {
        token("kd", "class");
        buffer.write(" ");
        token("nn", scanner.lastMatch.group(1));
      } else if (scanner.scan(_packagePattern)) {
        token("kn", "package");
        buffer.write(" ");
        token("nn", scanner.lastMatch.group(1));
        token("o", ";");
      } else if (scanner.scan(_annotationPattern)) {
        token("nd");
      } else if (scanner.scan(_identifierPattern)) {
        var identifier = scanner.lastMatch.group(0);
        token(_keywords[identifier] ?? "n");
      } else if (scanner.scan(_lineCommentPattern)) {
        token("c1");
      } else if (scanner.scan(_operatorPattern)) {
        token("o");
      } else {
        buffer.writeCharCode(scanner.readChar());
      }
    }

    buffer.writeln();
  }

//  if leading_newlines > 0:
//    html = html.replace('<pre>', '<pre>' + ('<br>' * leading_newlines))
//
//  if trailing_newlines > 0:
//    html = html.replace('</pre>', ('<br>' * trailing_newlines) + '</pre>')
//
//  # Strip off the div wrapper. We just want the <pre>.
//  html = html.replace('<div class="codehilite">', '')
//  html = html.replace('</div>', '')

  buffer.writeln("</pre>");
  return buffer.toString();
}
