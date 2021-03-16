import 'package:markdown/markdown.dart';

import '../book.dart';
import '../code_tag.dart';
import '../page.dart';
import '../snippet.dart';
import '../syntax/highlighter.dart';
import '../text.dart';

/// Custom code block formatter that uses our syntax highlighter.
class HighlightedCodeBlockSyntax extends BlockSyntax {
  static final _codeFencePattern = RegExp(r'^(\s*)```(.*)$');

  final bool _isXml;

  RegExp get pattern => _codeFencePattern;

  HighlightedCodeBlockSyntax({bool xml = false}) : _isXml = xml;

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
    var indent = match[1].length;
    var language = match[2];

    var childLines = parseChildLines(parser);

    String code;
    if (language == "text") {
      // Don't syntax highlight text.
      var buffer = StringBuffer();
      if (!_isXml) buffer.write("<pre>");

      for (var line in childLines) {
        // Strip off any leading indentation.
        if (line.length > indent) line = line.substring(indent);
        checkLineLength(line);
        buffer.writeln(line.escapeHtml);
      }

      if (!_isXml) buffer.write("</pre>");

      code = buffer.toString();
    } else {
      code = formatCode(language, childLines, indent: indent, xml: _isXml);
    }

    // Don't wrap in a div for XML.
    if (_isXml) return Element.text("pre", code);

    var element = Element.text("div", code);
    element.attributes["class"] = "codehilite";
    return element;
  }
}

/// Recognizes `^code` tags and inserts the relevant snippet.
class CodeTagBlockSyntax extends BlockSyntax {
  static final _startPattern = RegExp(r'\^code ([a-z0-9-]+)');

  final Book _book;
  final Page _page;
  final bool _isXml;

  CodeTagBlockSyntax(this._book, this._page, {bool xml = false}) : _isXml = xml;

  RegExp get pattern => _startPattern;

  bool canParse(BlockParser parser) =>
      pattern.firstMatch(parser.current) != null;

  Node parse(BlockParser parser) {
    var match = pattern.firstMatch(parser.current);
    var name = match[1];
    parser.advance();

    var codeTag = _page.findCodeTag(name);
    String snippet;
    if (_isXml) {
      snippet = _buildSnippetXml(codeTag, _book.findSnippet(codeTag));
    } else {
      snippet = _buildSnippet(codeTag, _book.findSnippet(codeTag));
    }
    return Text(snippet);
  }
}

String _buildSnippet(CodeTag tag, Snippet snippet) {
  // NOTE: If you change this, be sure to update the baked in example snippet
  // in introduction.md.

  if (snippet == null) {
    print("Undefined snippet ${tag.name}");
    return "<strong>ERROR: Missing snippet ${tag.name}</strong>\n";
  }

  var location = <String>[];
  if (tag.showLocation) location = snippet.locationHtmlLines;

  var buffer = StringBuffer();
  buffer.write('<div class="codehilite">');

  if (snippet.contextBefore.isNotEmpty) {
    _writeContextHtml(buffer, snippet.contextBefore,
        cssClass: snippet.added.isNotEmpty ? "insert-before" : null);
  }

  if (snippet.addedComma != null) {
    var commaLine = formatCode(snippet.file.language, [snippet.addedComma],
        preClass: "insert-before");
    var comma = commaLine.lastIndexOf(",");
    buffer.write(commaLine.substring(0, comma));
    buffer.write('<span class="insert-comma">,</span>');
    buffer.write(commaLine.substring(comma + 1));
  }

  if (tag.showLocation) {
    var lines = location.join("<br>\n");
    buffer.writeln('<div class="source-file">$lines</div>');
  }

  if (snippet.added != null) {
    var added = formatCode(snippet.file.language, snippet.added,
        preClass: tag.beforeCount > 0 || tag.afterCount > 0 ? "insert" : null);
    buffer.write(added);
  }

  if (snippet.contextAfter.isNotEmpty) {
    _writeContextHtml(buffer, snippet.contextAfter,
        cssClass: snippet.added.isNotEmpty ? "insert-after" : null);
  }

  buffer.writeln('</div>');

  if (tag.showLocation) {
    var lines = location.join(", ");
    buffer.writeln('<div class="source-file-narrow">$lines</div>');
  }

  return buffer.toString();
}

String _buildSnippetXml(CodeTag tag, Snippet snippet) {
  var buffer = StringBuffer();

  if (tag.showLocation) buffer.writeln(snippet.locationXml);

  if (snippet.contextBefore.isNotEmpty) {
    _writeContextXml(buffer, snippet.contextBefore, "before");
  }

  if (snippet.addedComma != null) {
    // TODO: How should this look in print?
    buffer.write("TODO added comma");
//    var commaLine = formatCode(snippet.file.language, [snippet.addedComma],
//        preClass: "insert-before", xml: true);
//    var comma = commaLine.lastIndexOf(",");
//    buffer.write(commaLine.substring(0, comma));
//    buffer.write('<span class="insert-comma">,</span>');
//    buffer.write(commaLine.substring(comma + 1));
  }

  if (snippet.added != null) {
    // Use different tags based on whether there is context before, after,
    // neither, or both.
    String insertTag;
    if (tag.beforeCount > 0) {
      if (tag.afterCount > 0) {
        insertTag = "interpreter-between";
      } else {
        insertTag = "interpreter-after";
      }
    } else {
      if (tag.afterCount > 0) {
        insertTag = "interpreter-before";
      } else {
        insertTag = "interpreter";
      }
    }

    buffer.write("<$insertTag>");
    buffer.write(formatCode(snippet.file.language, snippet.added, xml: true));
    buffer.write("</$insertTag>");
  }

  if (snippet.contextAfter.isNotEmpty) {
    _writeContextXml(buffer, snippet.contextAfter, "after");
  }

  return buffer.toString();
}

void _writeContextHtml(StringBuffer buffer, List<String> lines,
    {String cssClass}) {
  buffer.write("<pre");
  if (cssClass != null) buffer.write(' class="$cssClass"');
  buffer.writeln(">");

  for (var line in lines) {
    buffer.writeln(line.escapeHtml);
  }

  buffer.write("</pre>");
}

void _writeContextXml(StringBuffer buffer, List<String> lines, String tag) {
  if (lines.isEmpty) return;

  buffer.write("<context-$tag>");
  for (var line in lines) {
    buffer.writeln(line.escapeHtml);
  }
  buffer.write("</context-$tag>");
}
