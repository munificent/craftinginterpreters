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
    var indent = match[1].length;
    var language = match[2];

    var childLines = parseChildLines(parser);

    String code;
    if (language == "text") {
      // Don't syntax highlight text.
      var buffer = StringBuffer();
      buffer.write("<pre>");
      for (var line in childLines) {
        // Strip off any leading indentation.
        if (line.length > indent) {
          line = line.substring(indent);
        }
        buffer.writeln(escapeHtml(line));
      }
      buffer.write("</pre>");
      code = buffer.toString();
    } else {
      code = formatCode(language, childLines, indent: indent);
    }

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

  CodeTagBlockSyntax(this._book, this._page);

  RegExp get pattern => _startPattern;

  bool canParse(BlockParser parser) =>
      pattern.firstMatch(parser.current) != null;

  Node parse(BlockParser parser) {
    var match = pattern.firstMatch(parser.current);
    var name = match[1];
    parser.advance();

    var codeTag = _page.findCodeTag(name);
    return Text(_buildSnippet(codeTag, _book.findSnippet(codeTag)));
  }
}

String _buildSnippet(CodeTag tag, Snippet snippet) {
  // NOTE: If you change this, be sure to update the baked in example snippet
  // in introduction.md.

//  if name not in snippets:
//    errors.append("Undefined snippet {}".format(name))
//    contents += "**ERROR: Missing snippet {}**\n".format(name)
//    return contents
//
//  if snippets[name] == False:
//    errors.append("Reused snippet {}".format(name))
//    contents += "**ERROR: Reused snippet {}**\n".format(name)
//    return contents

//  # Consume it.
//  snippets[name] = False

  var location = <String>[];
  if (tag.showLocation) location = snippet.locationDescription;

//  # Make sure every snippet shows the reader where it goes.
//  if (showLocation and len(location) <= 1
//      and beforeLines == 0 and afterLines == 0):
//    print("No location or context for {}".format(name))
//    errors.append("No location or context for {}".format(name))
//    contents += "**ERROR: No location or context for {}**\n".format(name)
//    return contents

  var buffer = StringBuffer();
  buffer.write('<div class="codehilite">');

  if (snippet.contextBefore.isNotEmpty) {
    _writeContextLines(buffer, snippet.contextBefore,
        snippet.added.isNotEmpty ? "insert-before" : null);
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
    _writeContextLines(buffer, snippet.contextAfter,
        snippet.added.isNotEmpty ? "insert-after" : null);
  }

  buffer.writeln('</div>');

  if (tag.showLocation) {
    var lines = location.join(", ");
    buffer.writeln('<div class="source-file-narrow">$lines</div>');
  }

  return buffer.toString();
}

String _writeContextLines(
    StringBuffer buffer, List<String> lines, String preClass) {
  buffer.write("<pre");
  if (preClass != null) buffer.write(' class="$preClass"');
  buffer.writeln(">");

  for (var line in lines) {
    buffer.writeln(escapeHtml(line));
  }

  buffer.write("</pre>");
}
