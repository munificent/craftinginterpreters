import 'page.dart';

class CodeTag with Ordering<CodeTag> implements Comparable<CodeTag> {
  final Page chapter;
  final String name;

  /// Line in the Markdown file where this tag appears.
  final int line;

  /// Number of preceding lines of context to show.
  final int beforeCount;

  /// Number of trailing lines of context to show.
  final int afterCount;

  /// Whether to show location information.
  final bool showLocation;

  factory CodeTag(Page chapter, String name, int line, int beforeCount,
      int afterCount, bool showLocation) {
    // Hackish. Always want "not-yet" to be the last tag even if it appears
    // before a real tag. That ensures we can push it for other tags that have
    // been named.
    if (name == "not-yet") line = 9999;

    return CodeTag._(
        chapter, name, line, beforeCount, afterCount, showLocation);
  }

  CodeTag._(this.chapter, this.name, this.line, this.beforeCount,
      this.afterCount, this.showLocation);

  int compareTo(CodeTag other) {
    if (chapter.ordinal != other.chapter.ordinal) {
      return chapter.ordinal.compareTo(other.chapter.ordinal);
    }

    return line.compareTo(other.line);
  }

  String toString() => "Tag(${chapter.ordinal}|$line: $chapter $name)";
}

/// Implements the comparison operators in terms of [compareTo()].
mixin Ordering<T> implements Comparable<T> {
  bool operator <(T other) => compareTo(other) < 0;
  bool operator <=(T other) => compareTo(other) <= 0;
  bool operator >(T other) => compareTo(other) > 0;
  bool operator >=(T other) => compareTo(other) >= 0;
}
