import 'dart:io';

import 'code_tag.dart';
import 'page.dart';
import 'text.dart';

final _codeOptionsPattern = RegExp(r"([-a-z0-9]+) \(([^)]+)\)");
final _beforePattern = RegExp(r"(\d+) before");
final _afterPattern = RegExp(r"(\d+) after");

/// Parses the contents of the Markdown file for [page] to extract its metadata,
/// code tags, section headers, etc.
PageFile parsePage(Page page) {
  var template = 'page';
  var headers = <String, Header>{};
  var codeTags = <int, CodeTag>{};
  String designNote;
  var hasChallenges = false;

  var headerIndex = 0;
  var subheaderIndex = 0;

  var lines = File(page.markdownPath).readAsLinesSync();
  for (var i = 0; i < lines.length; i++) {
    var line = lines[i];
    if (line.startsWith("^")) {
      var commandLine = line.substring(1).trim();
      var space = commandLine.indexOf(" ");
      var command = commandLine.substring(0, space);
      var argument = commandLine.substring(space + 1).trim();

      switch (command) {
        case "code":
          codeTags[i] = createCodeTag(page, i, argument);
          break;
        case "template":
          template = argument;
          break;
        case "part":
        case "title":
          // TODO: No longer used. Remove from Markdown files.
          break;
        default:
          throw "Unknown command '$command'.";
      }
    } else if (line.startsWith("# ") ||
        line.startsWith("## ") ||
        line.startsWith("### ")) {
      // Keep track of the headers so we can add section navigation for them.
      var level = line.indexOf(" ");
      var headerType = line.substring(0, level);
      var name = pretty(line.substring(level).trim());

      if (headerType.length == 2) {
        headerIndex += 1;
        subheaderIndex = 0;
      } else if (headerType.length == 3) {
        subheaderIndex += 1;
      }

      var header = Header(headerType.length, headerIndex,
          headerType.length == 3 ? subheaderIndex : null, name);

      if (header.isChallenges) hasChallenges = true;
      if (header.isDesignNote) {
        designNote = header.name.substring("Design Note: ".length);
      }

      headers[line] = header;
    }
  }

//  # Validate that every snippet for the chapter is included.
//  for name, snippet in snippets.items():
//    if name != 'not-yet' and name != 'omit' and snippet != False:
//      errors.append("Unused snippet {}".format(name))
//
//  # Show any errors at the top of the file.
//  if errors:
//    error_markdown = ""
//    for error in errors:
//      error_markdown += "**Error: {}**\n\n".format(error)
//    contents = error_markdown + contents
//
  return PageFile(
      lines, template, headers, hasChallenges, designNote, codeTags);
}

CodeTag createCodeTag(Page chapter, int line, String argument) {
  var name = argument;

  // Parse the location annotations after the name, if present.
  var showLocation = true;
  var beforeCount = 0;
  var afterCount = 0;

  var match = _codeOptionsPattern.firstMatch(argument);
  if (match != null) {
    name = match.group(1);
    var options = match.group(2).split(", ");

    for (var option in options) {
      if (option == "no location") {
        showLocation = false;
      } else if ((match = _beforePattern.firstMatch(option)) != null) {
        beforeCount = int.parse(match.group(1));
      } else if ((match = _afterPattern.firstMatch(option)) != null) {
        afterCount = int.parse(match.group(1));
      }
    }
  }

  return CodeTag(chapter, name, line, beforeCount, afterCount, showLocation);
}
