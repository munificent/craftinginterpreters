import 'dart:convert';
import 'dart:math' as math;

/// Punctuation characters removed from file names and anchors.
final _punctuation = RegExp(r'[,.?!:/"]');

final _whitespace = RegExp(r"\s+");

/// Converts [text] to a string suitable for use as a file or anchor name.
String toFileName(String text) {
  if (text == "Crafting Interpreters") return "index";
  if (text == "Table of Contents") return "contents";

  // Hack. The introduction has a *subheader* named "Challenges" distinct from
  // the challenges section. This function here is also used to generate the
  // anchor names for the links, so handle that one specially so it doesn't
  // collide with the real "Challenges" section.
  if (text == "Challenges") return "challenges_";

  return text.toLowerCase().replaceAll(" ", "-").replaceAll(_punctuation, "");
}

/// Returns the length of the longest line in lines, or [longest], whichever
/// is longer.
int longestLine(int longest, Iterable<String> lines) {
  for (var line in lines) {
    longest = math.max(longest, line.length);
  }
  return longest;
}

String pluralize<T>(Iterable<T> sequence) {
  if (sequence.length == 1) return "";
  return "s";
}

extension IntExtensions on int {
  /// Convert n to roman numerals.
  String get roman {
    if (this <= 3) return "I" * this;
    if (this == 4) return "IV";
    if (this < 10) return "V" + "I" * (this - 5);

    throw ArgumentError("Can't convert $this to Roman.");
  }

  /// Make a nicely formatted string.
  String get withCommas {
    if (this > 1000) return "${this ~/ 1000},${this % 1000}";
    return toString();
  }
}

extension StringExtensions on String {
  /// Use nicer HTML entities and special characters.
  String get pretty {
    return this
        .replaceAll("à", "&agrave;")
        .replaceAll("ï", "&iuml;")
        .replaceAll("ø", "&oslash;")
        .replaceAll("æ", "&aelig;");
  }

  String get escapeHtml =>
      const HtmlEscape(HtmlEscapeMode.attribute).convert(this);

  int get wordCount => split(_whitespace).length;
}