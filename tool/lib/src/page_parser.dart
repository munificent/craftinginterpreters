import 'dart:io';

import 'code_tag.dart';
import 'page.dart';
import 'text.dart';

final _codePattern = RegExp(r"^\^code ([-a-z0-9]+)( \(([^)]+)\))?$");
final _headerPattern = RegExp(r"^(#{1,3}) ");
final _beforePattern = RegExp(r"(\d+) before");
final _afterPattern = RegExp(r"(\d+) after");

/// Parses the contents of the Markdown file for [page] to extract its metadata,
/// code tags, section headers, etc.
PageFile parsePage(Page page) {
  var headers = <String, Header>{};
  var codeTagsByName = <String, CodeTag>{};
  String designNote;
  var hasChallenges = false;

  var headerIndex = 0;
  var subheaderIndex = 0;

  var lines = File(page.markdownPath).readAsLinesSync();
  for (var i = 0; i < lines.length; i++) {
    var line = lines[i];

    var match = _codePattern.firstMatch(line);
    if (match != null) {
      var codeTag =
          _createCodeTag(page, codeTagsByName.length, match[1], match[3]);
      codeTagsByName[codeTag.name] = codeTag;
      continue;
    }

    match = _headerPattern.firstMatch(line);
    if (match != null) {
      // Keep track of the headers so we can add section navigation for them.
      var headerType = match[1];
      var level = headerType.length;
      var name = pretty(line.substring(level).trim());

      if (level == 2) {
        headerIndex += 1;
        subheaderIndex = 0;
      } else if (level == 3) {
        subheaderIndex += 1;
      }

      var header =
          Header(level, headerIndex, level == 3 ? subheaderIndex : null, name);

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
  return PageFile(lines, headers, hasChallenges, designNote, codeTagsByName);
}

CodeTag _createCodeTag(Page page, int index, String name, String options) {
  // Parse the location annotations after the name, if present.
  var showLocation = true;
  var beforeCount = 0;
  var afterCount = 0;

  if (options != null) {
    for (var option in options.split(", ")) {
      if (option == "no location") {
        showLocation = false;
        continue;
      }

      var match = _beforePattern.firstMatch(option);
      if (match != null) {
        beforeCount = int.parse(match.group(1));
        continue;
      }

      match = _afterPattern.firstMatch(option);
      if (match != null) {
        afterCount = int.parse(match.group(1));
        continue;
      }

      throw "Unknown code option '$option'";
    }
  }

  return CodeTag(page, name, index, beforeCount, afterCount, showLocation);
}
