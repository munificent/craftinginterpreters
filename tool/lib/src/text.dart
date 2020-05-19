import 'dart:math' as math;

/// Use nicer HTML entities and special characters.
String pretty(String text) {
  return text
      .replaceAll("...", "&hellip;")
      // Foreign characters.
      .replaceAll("à", "&agrave;")
      .replaceAll("ï", "&iuml;")
      .replaceAll("ø", "&oslash;")
      .replaceAll("æ", "&aelig;");
}

/// Converts [text] to a string suitable for use as a file or anchor name.
String toFileName(String text) =>
    text.toLowerCase().replaceAll(" ", "-").replaceAll(r'[,.?!:/"]', "");

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
