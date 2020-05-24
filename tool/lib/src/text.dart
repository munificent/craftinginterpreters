import 'dart:convert';
import 'dart:math' as math;

/// Punctuation characters removed from file names and anchors.
final _punctuation = RegExp(r'[,.?!:/"]');

/// Use nicer HTML entities and special characters.
String pretty(String text) {
  return text
      .replaceAll("à", "&agrave;")
      .replaceAll("ï", "&iuml;")
      .replaceAll("ø", "&oslash;")
      .replaceAll("æ", "&aelig;");
}

/// Converts [text] to a string suitable for use as a file or anchor name.
String toFileName(String text) {
  if (text == "Crafting Interpreters") return "index";
  if (text == "Table of Contents") return "contents";

  // TODO: Is this still needed?
  // Hack. The introduction has a *subheader* named "Challenges" distinct from
  // the challenges section. This function here is also used to generate the
  // anchor names for the links, so handle that one specially so it doesn't
  // collide with the real "Challenges" section.
  if (text == "Challenges") return "challenges_";

  return text.toLowerCase().replaceAll(" ", "-").replaceAll(_punctuation, "");
}

/// Convert n to roman numerals.
String roman(int n) {
  if (n <= 3) return "I" * n;
  if (n == 4) return "IV";
  if (n < 10) return "V" + "I" * (n - 5);

  throw ArgumentError("Can't convert $n to Roman.");
}

/// Returns the length of the longest line in lines, or [longest], whichever
/// is longer.
int longestLine(int longest, Iterable<String> lines) {
  for (var line in lines) {
    longest = math.max(longest, line.length);
  }
  return longest;
}

String escapeHtml(String html) =>
    const HtmlEscape(HtmlEscapeMode.element).convert(html);

String pluralize<T>(Iterable<T> sequence) {
  if (sequence.length == 1) return "";
  return "s";
}
